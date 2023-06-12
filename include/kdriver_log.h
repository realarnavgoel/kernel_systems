#ifndef KDRIVER_LOG_H
#define KDRIVER_LOG_H

#include <linux/printk.h>

typedef enum kdrv_log_lvl_e {
	EMERG = 0,
	ALERT,
	CRIT,
	ERR,
	WARNING,
	NOTICE,
	INFO,
	DEBUG
} kdrv_log_lvl_t;

typedef enum kdrv_log_type_e {
	DRV = 0,
	FOPS,
	MMAP,
	IOCTL
} kdrv_log_type_t;

#define KDRV_LOG(l, t, fmt, ...) printk(KERN_##l "["#t"]: " pr_fmt(fmt), ##__VA_ARGS__)

#endif /*! KDRIVER_LOG_H */
