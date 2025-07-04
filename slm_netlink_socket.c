// File		:slm_netlink_socket.c
// Author	:Victor Kovalevich
// Created	:Thu Jul  3 19:17:16 2025
#include "slm_netlink.h"

//
// NETLINK receive message callback
static void slm_netlink_socket_recv_msg(struct sk_buff *sk_buffer) {
    struct nlmsghdr* nl_header;

    nl_header = (struct nlmsghdr*)sk_buffer->data;
    slm_netlink_context_set_client_pid(nl_header->nlmsg_pid);
    pr_info("%s: Registered client PID=%d\n",
            MODULE_NAME,
            slm_netlink_context_get_client_pid());
    if (nl_header->nlmsg_len > 0 && nlmsg_data(nl_header)) {
        pr_info("%s: Registration message: %s\n",
                MODULE_NAME,
                (char*)nlmsg_data(nl_header));
    }
}

struct sock* slm_netlink_socket_init(const int nl_unit_id) {
    struct netlink_kernel_cfg nl_cfg = {
        .input = slm_netlink_socket_recv_msg,
        .flags = 0,
        .groups = 0,
    };
    struct sock* nl_socket = NULL;
    if ((nl_socket = netlink_kernel_create(&init_net, nl_unit_id, &nl_cfg)) == NULL) {
        pr_err("%s: unable to create a Netlink socket with [%d] unit\n",
               MODULE_NAME,
               nl_unit_id);
    } else {
        pr_info("%s: netlink_kernel_create() with [%d] unit Ok\n",
                MODULE_NAME,
                nl_unit_id);
    }

    return nl_socket;
}

void slm_netlink_socket_release(struct sock* nl_socket) {
    if (nl_socket == NULL) {
        return;
    }

    netlink_kernel_release(nl_socket);

    pr_info("%s: netlink_kernel_release() Ok\n",
            MODULE_NAME);
}

// Sending of a message to the client (if it is registered)
void slm_netlink_socket_send_msg(const char* msg_body, size_t msg_len) {
    static const unsigned int max_attempt_number = 5;
    struct sk_buff* sk_buffer_out;
    struct nlmsghdr* nl_header;
    int res;
    unsigned int i;

    if (slm_netlink_context_get_nl_socket() == NULL ||
        slm_netlink_context_get_client_pid() <= 0) {
        return;
    }

    //
    // NOTE: message should be released within `nlmsg_unicast` even
    //       an issue occurs
    if ((sk_buffer_out = nlmsg_new(msg_len, GFP_KERNEL)) == NULL) {
        pr_err("%s: Failed to allocate sk_buffer_out\n",
               MODULE_NAME);
        return;
    }
    nl_header = nlmsg_put(sk_buffer_out, 0, 0, NLMSG_DONE, msg_len, 0);
    memcpy(nlmsg_data(nl_header), msg_body, msg_len);

    // Unicast the message to the client by using its PID (client_pid)
    i = max_attempt_number;

    while(i > 0) {
        if ((res = nlmsg_unicast(slm_netlink_context_get_nl_socket(),
                                 sk_buffer_out,
                                 slm_netlink_context_get_client_pid())) < 0) {
            pr_err("%s: Error sending to user %d due to the issue: %d",
                   MODULE_NAME,
                   slm_netlink_context_get_client_pid(),
                   res);
            i--;
        } else {
            break;
        }
    }

    if (i == 0) {
        pr_err("%s: Connection to user %d is closed "
               "as number of message sending attempts %u has been expired\n",
               MODULE_NAME,
               slm_netlink_context_get_client_pid(),
               max_attempt_number);
        slm_netlink_context_set_client_pid(0);
    }
}
