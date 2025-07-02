// File		:slm_user.c
// Author	:Victor Kovalevich
// Created	:Wed Jul  2 12:53:13 2025
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define NETLINK_USER 25

int main() {
    struct sockaddr_nl src_addr;
    int sock_fd;
    struct nlmsghdr *nlh = NULL;
    struct iovec iov;
    struct msghdr msg;

    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); // our PID

    if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(1024));
    while (1) {
        memset(nlh, 0, NLMSG_SPACE(1024));
        iov.iov_base = (void *)nlh;
        iov.iov_len = NLMSG_SPACE(1024);
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        int len = recvmsg(sock_fd, &msg, 0);
        if (len > 0) {
            printf("Received: %s\n", (char *)NLMSG_DATA(nlh));
        }
    }
    free(nlh);
    close(sock_fd);
    return 0;
}
