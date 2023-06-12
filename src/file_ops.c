#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/random.h>
#include "kdriver.h"
#include "kdriver_log.h"

/* 1MB page size */
#define KDRIVER_KV_SIZE_BYTES (1024 * 1024)
#define KV_OFFSET(b, t, o) (t)((u8 *)(b) + (o))

/**
 * @struct kdriver_ctx_rsc_t
 * @brief KDriver File Context Resource Object
 */
typedef struct kdriver_ctx_rsc_s {
    void *region;
    struct task_struct *task;
} kdriver_ctx_rsc_t;

int kdriver_rng_cb(void *data) {
    size_t off; u8 *target;
    kdriver_ctx_rsc_t *arg = (kdriver_ctx_rsc_t *)(data);
    void *region = arg->region;
    u8 rand = 0;

    while (!kthread_should_stop()) {
	// Find RNG offset b/w (size/2, size)
	off = (KDRIVER_KV_SIZE_BYTES/2) + (get_random_u32() % (KDRIVER_KV_SIZE_BYTES/2));
	if (off >= KDRIVER_KV_SIZE_BYTES) {
	   off = (KDRIVER_KV_SIZE_BYTES/2); /* Reset to middle */
	}

	// Calculate target location from region, offset
	target = KV_OFFSET(region, u8 *, off);

	// Assign target to RNG value
	get_random_bytes(&rand, 1);
	*(u8 *)(target) = (u8)rand;

	KDRV_LOG(DEBUG, FOPS, "Simulating Counter[%zx]: [%u]\n",
		 off, (u8)rand);

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
    // Increment the refcount
    ctx->refcount++;

    // Allocate a physically contiguous region of size 1MB and assign to filep private
    uctx->region = kzalloc(GFP_KERNEL, KDRIVER_KV_SIZE_BYTES);
    if (!uctx->region) {
	KDRV_LOG(ERR, FOPS, "Unable to allocate file %p kpage\n", fp);
	goto unlock_free_uctx;
    }

    // Allocate a kthread and assign to filep private
    uctx->task = kthread_create(&kdriver_rng_cb, uctx, "RNG Counter");
    if (!uctx->task) {
        KDRV_LOG(ERR, FOPS, "Unable to allocate file %p kthread\n", fp);
	goto unlock_free_kpage;
    }

    // Start a kthread with a callback to write to 512KB-1MB with RNG every 1msec
    wake_up_process(uctx->task);
    fp->private_data = (void *)(uctx);
    KDRV_LOG(INFO, FOPS, "Opened filep: %p refcount: %u task: %p region: %llx\n", fp,
	     ctx->refcount, uctx->task, (u64)uctx->region);
    // TODO: Append page and kthread to the global driver context
    mutex_unlock(&priv->lock);
    return 0;

unlock_free_kpage:
    kfree(uctx->region);
unlock_free_uctx:
    vfree(uctx);
    mutex_unlock(&priv->lock);
    fp->private_data = NULL;
    return -1;
}

int kdriver_close(struct inode *inode, struct file *fp) {
    int rc = 0;
    struct kdriver_context_s *ctx = priv->ctx;
    kdriver_ctx_rsc_t *uctx = (kdriver_ctx_rsc_t *)fp->private_data;
    mutex_lock(&priv->lock);

    // Decrement the refcount
    // TODO: Remove page and kthread from the global driver context
    ctx->refcount--;

    // Join and Free a kthread associated to this filep
    rc = kthread_stop(uctx->task); 
    if (rc < 0) {
        KDRV_LOG(ERR, FOPS, "Unable to close filep: %p and stop task: %p. Reason: %d\n", fp, uctx->task, rc);
	rc = -EFAULT;
    }

    // Free a physically contiguous region of size 1MB associated to this filep priv
    kfree(uctx->region);
    KDRV_LOG(INFO, FOPS, "Closed filep: %p refcount: %u", fp,
             ctx->refcount);

    // Free filep private context
    vfree(uctx);
    fp->private_data = NULL;
    mutex_unlock(&priv->lock);
    return (rc); 
}

int kdriver_mmap(struct file *fp, struct vm_area_struct *vma) {

    // Use vma->offset as a opcode: MAP_COUNTER, etc
    // Enforce vma->flags to be VM_LOCKED and VM_DONTFORK
    // Assuming backend is kzalloc: Get physical address from kvaddr
    // remap_pfn_range() on a single PA
    //
    // Assuming backend is vmalloc: Get physical address list from kvaddr
    // remap_pfn_range() on a list of PA
    return -1;
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
