/*
    Speed control for DoS tests, 
    jesus.olmos@innevis.com

    USAGE:
        make
        LD_PRELOAD=./bwcontrol.so BW_LIMIT=80 hping3 -1 8.8.8.8  --fast

        this hping has been limited his max speed to 80 bytes per second.
*/

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#define _GNU_SOURCE 

static ssize_t (*__libc_sendto)(int , const void *, size_t, int, const struct sockaddr *, socklen_t);
static ssize_t (*__libc_send)(int, const void*, size_t, int);

static void *libc = NULL;
static int bw_bytes = 0;
static time_t bw_time = NULL;
static int bw_limit = 0;
static unsigned int bw_global_timeout = 0;


ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen);

void __attribute__ ((constructor)) init(void) {
    libc = RTLD_NEXT;
    /*libc = dlopen("/lib/x86_64-linux-gnu/libc.so.6", RTLD_LAZY);
    if (libc == NULL)
        printf("crash\n");
    */


    if (getenv("BW_LIMIT") == NULL || getenv("BW_TIMEOUT") == NULL)
        usage();

    bw_global_timeout = time(NULL) + atoi(getenv("BW_TIMEOUT"));


    __libc_sendto = dlsym(libc, "sendto");
    __libc_send = dlsym(libc, "send");

    printf("BW Control loaded.\n");

}


void __attribute__ ((destructor)) fini(void) {
    //srand = __libc_srand;
    //rand = __libc_rand;
    //dlclose(libc);
}

void usage() {
    printf("example of usage:\n");
    printf("LD_PRELOAD=./bwcontrol.so BW_LIMIT=10 BW_TIMEOUT=100  ./tool\n");
    printf("  this launches tool during 100 seconds, then finishes the execution.\n");
    printf("  the speed is limited to 100 bytes per second\n");
    exit(1);
}


int bw_end_timeout() {
    return (time(NULL) >= bw_global_timeout);
}

int bw_timeout() {
    return ((time(NULL)-bw_time) >= 1);
}

int bw_mustDrop() {

    if (bw_time == NULL) {
        //printf("crono start\n");
        bw_limit = atoi(getenv("BW_LIMIT"));
        bw_time = time(NULL);

    } else {
        if (bw_end_timeout()) {
            printf("BW_TIMEOUT reached!!\n");
            exit(1);
        }

        if (bw_timeout()) {
            //printf("timeout\n");
            bw_time = time(NULL);
            bw_bytes = 0;

        } else {
            if (bw_bytes >= bw_limit) {
                printf("%d bytes sent, Blocked\n",bw_bytes);
                return 1;
            } /*else {
                printf("limit not reached\n");
            }*/
        }
    }

    return 0;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen) {

    if (bw_mustDrop())
        return 0;

    bw_bytes += len;
    printf("bytes sent: %d\n",bw_bytes);
    return __libc_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    if (bw_mustDrop())
        return 0;

    bw_bytes += len;
    printf("bytes sent: %d\n",bw_bytes);
    return __libc_send(sockfd, buf, len, flags);
}
