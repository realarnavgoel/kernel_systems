#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/random.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include "kdriver_shared_types.h"
#include "kdriver.h"
#include "kdriver_log.h"

#define KV_OFFSET(b, t, o) (t)((u8 *)(b) + (o))
#define VMA_OFFSET_UNIT(off) ((off)/PAGE_SIZE)

/**
 * @struct kdriver_ctx_rsc_t
 * @brief KDriver File Context Resource Object
 */
typedef struct kdriver_ctx_rsc_s {
    void *region;
    struct task_struct *task;
    struct mutex *mm_lock;
} kdriver_ctx_rsc_t;

int kdriver_rng_cb(void *data) {
    size_t off; u8 *target;
    kdriver_ctx_rsc_t *arg = (kdriver_ctx_rsc_t *)(data);
    void *region = arg->region;
    u8 rand = 0;

    while (!kthread_should_stop()) {
	// Find RNG offset b/w (size/2, size)
	off = (MAP_COUNTER_SIZE/2) + (get_random_u32() % (MAP_COUNTER_SIZE/2));
	if (off >= MAP_COUNTER_SIZE) {
	   off = (MAP_COUNTER_SIZE/2); /* Reset to middle */
	}

	mutex_lock(arg->mm_lock);
	// Calculate target location from region, offset
	target = KV_OFFSET(region, u8 *, off);

	// Assign target to RNG value
	get_random_bytes(&rand, 1);
	*(u8 *)(target) = (u8)rand;

	KDRV_LOG(DEBUG, FOPS, "Task[%p]: Simulating Counter[%zx]: [%2x]\n",
                 arg->task, off, (u8)rand);

	mutex_unlock(arg->mm_lock);

	// Wait for 1msec before next simulated counter write 
        msleep_interruptible(1);
    }

    return 0;
}


int kdriver_open(struct inode *inode, struct file *fp) {
    struct kdriver_context_s *ctx = priv->ctx;
    // Allocate fp private context
    kdriver_ctx_rsc_t *uctx = vmalloc(sizeof(kdriver_ctx_rsc_t));
    if (!uctx) {
        KDRV_LOG(ERR, FOPS, "Unable to allocate file %p open context\n",
	         fp);
	return -1;
    }

    mutex_lock(&priv->lock);
    // Allocate a physically contiguous region to simulate HW rsc
    uctx->region = kzalloc(MAP_COUNTER_SIZE, GFP_KERNEL);
    if (!(uctx->region)) {
	KDRV_LOG(ERR, FOPS, "Invalid kdriver counter rsc on file %p\n",
		 fp);
	goto unlock_return;
    }

    uctx->mm_lock = vmalloc(sizeof(struct mutex));
    if (!(uctx->mm_lock)) {
	KDRV_LOG(ERR, FOPS, "Unable to allocate file %p mm lock\n",
		 fp);
	goto unlock_free_region;
    }

    mutex_init(uctx->mm_lock);
    // Allocate a kthread and assign to filep private
    uctx->task = kthread_create(&kdriver_rng_cb, uctx, "RNG Counter");
    if (!uctx->task) {
        KDRV_LOG(ERR, FOPS, "Unable to allocate file %p kthread\n", fp);
	goto unlock_free_mmlock_uctx;
    }

    // Start a kthread with a callback to write to 512KB-1MB with RNG every 1msec
    wake_up_process(uctx->task);
    fp->private_data = (void *)(uctx);
    // Increment the refcount
    ctx->refcount++;
    KDRV_LOG(INFO, FOPS, "Opened filep: %p refcount: %u task: %p region: %llx\n", fp,
	     ctx->refcount, uctx->task, (u64)uctx->region);
    // TODO: Append kthread to the global driver context
    mutex_unlock(&priv->lock);
    return 0;

unlock_free_mmlock_uctx:
    mutex_destroy(uctx->mm_lock);
    vfree(uctx->mm_lock);
    vfree(uctx);
unlock_free_region:
    kfree(uctx->region);
unlock_return:
    mutex_unlock(&priv->lock);
    fp->private_data = NULL;
    return -1;
}

int kdriver_close(struct inode *inode, struct file *fp) {
    int rc = 0;
    struct kdriver_context_s *ctx = priv->ctx;
    kdriver_ctx_rsc_t *uctx = (kdriver_ctx_rsc_t *)fp->private_data;
    mutex_lock(&priv->lock);

    // Join and Free a kthread associated to this filep
    rc = kthread_stop(uctx->task); 
    if (rc < 0) {
        KDRV_LOG(ERR, FOPS, "Unable to close filep: %p and stop task: %p. Reason: %d\n", fp, uctx->task, rc);
	rc = -EFAULT;
    }

    // Free physically contiguous region to simulate HW rsc
    kfree(uctx->region);

    // Free filep private context
    mutex_destroy(uctx->mm_lock);
    vfree(uctx->mm_lock);
    vfree(uctx);

    // Decrement the refcount
    // TODO: Remove kthread from the global driver context
    ctx->refcount--;
    KDRV_LOG(INFO, FOPS, "Closed filep: %p refcount: %u", fp,
             ctx->refcount);
    fp->private_data = NULL;
    mutex_unlock(&priv->lock);
    return (rc); 
}

static int kdriver_mmap_counter_page(kdriver_ctx_rsc_t *uctx,
				     struct vm_area_struct *vma) {
    // Assuming backend is kzalloc: Get physical address from kvaddr
    // remap_pfn_range() on a single PA
    unsigned long pfn = virt_to_pfn(uctx->region);
    unsigned long size = (vma->vm_end - vma->vm_start);
    // Enforce vm->flags to be VM_LOCKED and VM_DONTFORK
    vma->vm_flags |= (VM_READ | VM_WRITE | VM_LOCKED | VM_DONTCOPY);
    // Since do_mmap holds mm_sem, this is safe to call
    KDRV_LOG(DEBUG, FOPS, "Mapping vma_start %lx pfn %lx size %ld\n",
		    vma->vm_start, pfn, size);
    return (remap_pfn_range(vma, vma->vm_start, pfn, size, vm_get_page_prot(vma->vm_flags)));
}

int kdriver_mmap(struct file *fp, struct vm_area_struct *vma) {

    int rc = -ENOMEM;
    kdriver_ctx_rsc_t *uctx = (kdriver_ctx_rsc_t *)(fp->private_data);
    // Use vma->offset as a opcode: MAP_COUNTER, etc
    switch(vma->vm_pgoff) {
        case MAP_OPC_COUNTER: {
		mutex_lock(uctx->mm_lock);
		rc = kdriver_mmap_counter_page(uctx, vma);
		mutex_unlock(uctx->mm_lock);
	} break;
	default: KDRV_LOG(ERR, FOPS, "Invalid opc: %lu for filep: %p\n",
		          vma->vm_pgoff, fp);
		 return (-EINVAL);
    }

    // TODO: Assuming backend is vmalloc: Get physical address list from kvaddr
    // remap_pfn_range() on a list of PA
    return (rc);
}

int kdriver_ioctl(struct inode *inode, struct file *fp,
		  unsigned int code, unsigned long arg) {


    // ALLOC_MONITOR: fp->private_data
    // pages = get_user_pages_fast(uvaddr)
    // kvaddr = vmap(pages)
    // mon = kthread_create(MONITOR, fn)
    // wake_up_process(mon)

    // FREE_MONITOR: fp->private_data
    // vunmap(kvaddr)
    // kthread_stop(mon)

    // SET_MONITOR_INFO: fp->private_data
    // SET_COUNTER_INFO: fp->private_data
    return -1;
}
