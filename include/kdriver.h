#ifndef KDRIVER_H
#define KDRIVER_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

/**
 * @struct kdriver_context_t
 * @brief KDriver Context Structure
 */
typedef struct kdriver_context_s {
	u8 refcount;
	//kthread
	//kpage
} kdriver_context_t;

/**
 * @struct kdriver_t
 * @brief KDriver Private Structrue
 */
typedef struct kdriver_s {
	struct class *cls;
	dev_t devt;
	struct cdev cdev;
	struct file_operations fops;
	struct device *dev;
	kdriver_context_t *ctx;
	struct mutex lock;
} kdriver_t;

/**
 * @def priv
 * @brief KDriver Private Object Exportable
 */
extern kdriver_t *priv;

/**
 * @brief Opens a kdriver device file per open() call
 * @param inode
 * @param file
 * @return 0 or -EFAULT
 */
int kdriver_open(struct inode *, struct file *);

/**
 * @brief Closes a kdriver device file per close() call
 * @param inode
 * @param file
 * @return 0 or -EFAULT
 */
int kdriver_close(struct inode *, struct file *);

/**
 * @brief Memory map kdriver device file region per mmap() call
 *        based on the MAP_OPC specified in offset field
 * @param file
 * @param vma
 * @return 0 or -EINVAL, -EFAULT, -ENOMEM
 */
int kdriver_mmap(struct file *, struct vm_area_struct *vma);

#endif /*! KDRIVER_H */
