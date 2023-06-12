#include <linux/init.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include "kdriver_log.h"
#include "kdriver.h"

kdriver_t *priv = NULL;

#define KDRIVER_FILE_NAME "kdriver"
#define KDRIVER_CLASS_NAME "kdriver_class"

static int __init kdriver_init(void) {
    int rc = -1;

    // Create kdriver priv object
    priv = vmalloc(sizeof(kdriver_t));
    if (!priv) {
        KDRV_LOG(ALERT, DRV, "Unable to allocate kdriver priv\n");
	return -1;
    }

    // Create device class object
    priv->cls = class_create(THIS_MODULE, KDRIVER_CLASS_NAME);
    if (!(priv->cls)) {
        KDRV_LOG(ALERT, DRV, "Unable to allocate class object\n");
	goto cleanup_kdriver_priv;
    }

    // Allocate device major.minor number
    rc = alloc_chrdev_region(&(priv->devt), 0, 1, KDRIVER_FILE_NAME);
    if (rc < 0) {
	KDRV_LOG(ALERT, DRV, "Unable to allocate device major/minor number\n");
	goto cleanup_kdriver_class;
    }

    // Allocate char device
    priv->fops.open = &kdriver_open;
    priv->fops.release = &kdriver_close;
    priv->fops.mmap = &kdriver_mmap;
    cdev_init(&(priv->cdev), &(priv->fops));
    rc = cdev_add(&(priv->cdev), priv->devt, 1);
    if (rc < 0) {
	KDRV_LOG(ALERT, DRV, "Unable to allocate char device\n");
	goto cleanup_kdriver_devt;
    }

    // Create device node using device number & class
    priv->dev = device_create(priv->cls, NULL, priv->devt, priv, "kdriver");
    if (!(priv->dev)) {
        KDRV_LOG(ALERT, DRV, "Unable to create device node\n");
	goto cleanup_kdriver_cdev;
    }

    // Allocate kdriver global uctx
    priv->ctx = vmalloc(sizeof(kdriver_context_t));
    if (!(priv->ctx)) {
        KDRV_LOG(ALERT, DRV, "Unable to allocate kdriver global context\n");
	goto cleanup_kdriver_device;
    }

    mutex_init(&(priv->lock));

    return 0;

cleanup_kdriver_device:
    device_destroy(priv->cls, priv->devt);

cleanup_kdriver_cdev:
    cdev_del(&(priv->cdev));

cleanup_kdriver_devt:
    unregister_chrdev_region(priv->devt, 1);

cleanup_kdriver_class:
    class_destroy(priv->cls);

cleanup_kdriver_priv:
    vfree(priv);
    return -1;
}

static void __exit kdriver_fini(void) {
    // Mutex destroy
    mutex_destroy(&priv->lock);

    // Free kdriver uctx
    vfree(priv->ctx);

    // Destroy device node
    device_destroy(priv->cls, priv->devt);

    // Free char device 
    cdev_del(&(priv->cdev));

    // Free device numbers
    unregister_chrdev_region(priv->devt, 1);

    // Destroy device class
    class_destroy(priv->cls);

    // Free private kdriver object
    vfree(priv);

    priv = NULL;
}

module_init(kdriver_init);
module_exit(kdriver_fini);
MODULE_LICENSE("GPL");
