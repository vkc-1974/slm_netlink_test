// File		:slm_user.c
// Author	:Victor Kovalevich
// Adapted for Netlink registration handshake with kernel module
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define NETLINK_USER 25

#define BUFFER_SIZE 1024
    

int main() {
    struct sockaddr_nl src_addr, dest_addr;
    int sock_fd;
    struct nlmsghdr *nlh = NULL;
    struct iovec iov;
    struct msghdr msg;

    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0) {
        perror("socket(AF_NETLINK, SOCK_RAW, NETLINK_USER)");
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

    /* Prepare destination address (kernel) */
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; // Kernel
    dest_addr.nl_groups = 0; // unicast

    /* Allocate and prepare registration message */
    const char *register_msg = "register";
    size_t reg_msg_len = strlen(register_msg) + 1;
    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(BUFFER_SIZE)); //// NLMSG_SPACE(reg_msg_len));
    memset(nlh, 0, NLMSG_SPACE(reg_msg_len));
    nlh->nlmsg_len = NLMSG_SPACE(reg_msg_len);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;
    memcpy(NLMSG_DATA(nlh), register_msg, reg_msg_len);

    /* Set up IO vector and message header */
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    /* Send registration message to kernel */
    if (sendmsg(sock_fd, &msg, 0) < 0) {
        perror("sendmsg");
        free(nlh);
        close(sock_fd);
        return -1;
    }

    /* Ready to receive notifications from kernel */
    /* Reuse the same nlh buffer for receiving */
    while (1) {
        memset(nlh, 0, NLMSG_SPACE(1024));
        iov.iov_base = (void *)nlh;
        iov.iov_len = NLMSG_SPACE(1024);
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = (void *)&src_addr;
        msg.msg_namelen = sizeof(src_addr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        int len = recvmsg(sock_fd, &msg, 0);
        if (len > 0) {
            const char* msg_data;
            msg_data = (char *)NLMSG_DATA(nlh);
            printf("Received: %s\n", msg_data);
        }
        //// break;
    }
    free(nlh);
    close(sock_fd);
    return 0;
}
