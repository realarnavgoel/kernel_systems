#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include "kdriver_shared_types.h"

#define FOR_EACH(index, size) for(int index = 1; index < size; i++)
#define APP_LOG(l, t, fmt, ...) printf("["#l"]["#t"]: "fmt, ##__VA_ARGS__)
#define MAP_PAGE_SIZE_UNIT(o) (o * sysconf(_SC_PAGESIZE))
#define ADDR_OFFSET(b, t, o) ((t)((uint8_t *)(b) + o))

int run_openclose_test_one(void) {
    int fd = -1;

    fd = open("/dev/kdriver", O_RDWR);
    if (fd < 0) {
        APP_LOG(ERR, TEST, "Unable to open kdriver file. Reason: %s\n", strerror(errno));
	return -1;
    }

    /* Insert delay to allow threads to start - this is workaround !!! */
    usleep(1000);

    return (close(fd));
}

int run_openmmapclose_test_one(void) {
    void *addr = NULL;
    int fd = -1, i = 0;
    size_t count = 0;
    uint8_t *target = NULL;

    fd = open("/dev/kdriver", O_RDWR);
    if (fd < 0) {
        APP_LOG(ERR, TEST, "Unable to open kdriver file. Reason: %s\n", strerror(errno));
	return -1;
    }

    addr = mmap(NULL, MAP_COUNTER_SIZE, PROT_READ,
		MAP_PRIVATE, fd,
		MAP_PAGE_SIZE_UNIT(MAP_OPC_COUNTER));
    if (addr == MAP_FAILED) {
	APP_LOG(ERR, TEST, "Unable to mmap kdriver counter rsc. Reason: %s\n",
		strerror(errno));
	(void)close(fd);
	return -1;
    }

    APP_LOG(INFO, TEST, "Reading out kdriver performance counter\n");
    APP_LOG(INFO, TEST, "=========================================\n");
    do {
	 target = ADDR_OFFSET(addr, uint8_t *, i);
	 APP_LOG(INFO, TEST, "Offset[%2x]: %x\n",
		 i, *(uint8_t *)(target));
	 count += ((target[i] != 0) ? 1 : 0);
	 i += 1;
    } while (i < MAP_COUNTER_SIZE);
    APP_LOG(INFO, TEST, "Number of non-zero counter entries: %zu\n", count);
    APP_LOG(INFO, TEST, "=========================================\n");

    if (munmap(addr, MAP_COUNTER_SIZE) < 0) {
        APP_LOG(ERR, TEST, "Unable to munmap kdriver counter rsc. Reason: %s\n",
		strerror(errno));
	(void)close(fd);
	return -1;
    }

    return (close(fd));
}

int run_openclose_test(int iterations) {
    int i = 0;
    for (i = 0; i < iterations; i++) {
        if (run_openclose_test_one() < 0) {
	   APP_LOG(ERR, TEST, "Unable to run open/close test iter %d\n", i);
	   return -1;
	}
    }

    return 0;
}

int run_openmmapclose_test(int iterations) {
    int i = 0;
    for (i = 0; i < iterations; i++) {
        if (run_openmmapclose_test_one() < 0) {
	   APP_LOG(ERR, TEST, "Unable to run open/mmap/close test iter %d\n", i);
	   return -1;
	}
    }

    return 0;
} 

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("Usage: ./app [OPTIONS]\n");
	printf("\t\t\tOptions: -o Open/Close for user-specified iterations\n");
	printf("\t\t\tOptions: -m Memory Map for user-specified iterations\n");
	return 1;
    }

    // TODO: getopts implement
    int iter = atoi(argv[2]);
    assert(run_openclose_test(iter) == 0);
    assert(run_openmmapclose_test(iter) == 0);
    return 0;
}
