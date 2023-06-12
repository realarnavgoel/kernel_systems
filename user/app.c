#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#define FOR_EACH(index, size) for(int index = 1; index < size; i++)
#define APP_LOG(l, t, fmt, ...) printf("["#l"]["#t"]: "fmt, ##__VA_ARGS__)

int run_openclose_test_one(void) {
    int fd = open("/dev/kdriver", O_RDWR);
    if (fd < 0) {
        APP_LOG(ERR, TEST, "Unable to open kdriver file. Reason: %s\n", strerror(errno));
	return -1;
    }

    usleep(5*1000000);
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
    return 0;
}
