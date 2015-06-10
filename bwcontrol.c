/*
    Speed control for DoS tests, 
    jesus.olmos@innevis.com

    USAGE:
        make
        LD_PRELOAD=./bwcontrol.so BW_LIMIT=b80 BW_TIMEOUT=100  hping3 -1 8.8.8.8  --fast

        this hping has been limited his max speed to 80 bytes per second, and in 100 seconds will be finished.
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
static ssize_t (*__libc_sendmsg)(int , const struct msghdr *, int);

static void *libc = NULL;
static int bw_bytes = 0;
static time_t bw_time = NULL;
static float bw_limit = 0;
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

    char *env_bt_limit = getenv("BW_LIMIT");
    char precission = env_bt_limit[0];
    env_bt_limit++;

    bw_global_timeout = time(NULL) + atoi(getenv("BW_TIMEOUT"));
    bw_limit = atof(env_bt_limit);
    bw_time = time(NULL);

    
    switch (precission) {
        case 'K':
        case 'k': bw_limit *= 1024; break;
        case 'M':
        case 'm': bw_limit *= 1024*1024; break;
        case 'G':
        case 'g': bw_limit *= 1024*1024*1024; break;
    }

    printf("limit: %f bytes\n", bw_limit);

    __libc_send = dlsym(libc, "send");
    __libc_sendto = dlsym(libc, "sendto");
    __libc_sendmsg = dlsym(libc, "sendmsg");

    printf("BW Control loaded.\n");
}


void __attribute__ ((destructor)) fini(void) {
    //srand = __libc_srand;
    //rand = __libc_rand;
    //dlclose(libc);
}

void usage() {
    printf("example of usage:\n");
    printf("LD_PRELOAD=./bwcontrol.so BW_LIMIT=b10 BW_TIMEOUT=100  ./tool\n");
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
    int mustDrop = 0;

    if (bw_end_timeout()) {
        printf("BW_TIMEOUT reached!!\n");
        exit(1);
    }

    if (bw_bytes > bw_limit) {
        printf("%d bytes, Blocked\n",bw_bytes);
        mustDrop = 1;
    } else 
        printf("%d bytes, send\n", bw_bytes);

    if (bw_timeout()) {
        bw_time = time(NULL);
        bw_bytes = 0;
    } 

    return mustDrop;
}



ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    bw_bytes += len;

    if (bw_mustDrop())
        return len;

    
    printf("bytes sent: %d\n",len);
    return __libc_send(sockfd, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen) {

    bw_bytes += len;

    if (bw_mustDrop())
        return len;

    printf("bytes sent: %d\n",len);
    return __libc_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    int len = 0;
    for (int i=0; i<msg->msg_iovlen; i++)
        len += msg->msg_iov[i].iov_len;

    bw_bytes += len;

    if (bw_mustDrop())
        return len;

    printf("sending %d bytes\n", len);
    return __libc_sendmsg(sockfd, msg, flags);
}
