#ifndef KDRIVER_SHARED_TYPES_H
#define KDRIVER_SHARED_TYPES_H


#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <sys/types.h>
#endif

#define MAP_OPC_COUNTER 1
#define MAP_COUNTER_SIZE (1024 * 1024)

#endif /*! KDRIVER_SHARED_TYPES_H */
