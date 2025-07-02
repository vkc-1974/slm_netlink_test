// File		:slm_netlink.c
// Author	:Victor Kovalevich
// Created	:Wed Jul  2 12:50:38 2025
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/skbuff.h>
#include <linux/fs.h>
#include <linux/kprobes.h>
#include <linux/version.h>
#include <linux/uaccess.h>

#define NETLINK_USER 25
#define MAX_PAYLOAD 128

struct slm_module_data {
    //
    // NETLINK socket (communication with a userspace process (client)
    struct sock* nl_socket;
    //
    // Userspace client PID
    int client_pid;
    //
    // Kprobes
    struct kprobe kp_create;
    struct kprobe kp_open;
    struct kprobe kp_read;
    struct kprobe kp_write;
    struct kprobe kp_lseek;
    struct kprobe kp_close;
};

static const char* MODULE_NAME = "slm_netlink";

#if 0
#define KP_VFS_CREATE "vfs_create"
#define KP_VFS_OPEN   "vfs_open"
#define KP_VFS_READ   "vfs_read"
#define KP_VFS_WRITE  "vfs_write"
#define KP_VFS_LSEEK  "vfs_llseek"
#define KP_VFS_CLOSE  "filp_close"
#else
#define KP_VFS_CREATE "__x64_sys_creat"
#define KP_VFS_OPEN   "__x64_sys_openat"
#define KP_VFS_READ   "__x64_sys_read"
#define KP_VFS_WRITE  "__x64_sys_write"
#define KP_VFS_LSEEK  "__x64_sys_lseek"
#define KP_VFS_CLOSE  "__x64_sys_close"
#endif

static struct slm_module_data slm_unit = {
    .nl_socket = NULL,
    .client_pid = 0,
    .kp_create = {
        .symbol_name = NULL,
        .pre_handler = NULL,
        .post_handler = NULL
    },
    .kp_open = {
        .symbol_name = NULL,
        .pre_handler = NULL,
        .post_handler = NULL
    },
    .kp_read = {
        .symbol_name = NULL,
        .pre_handler = NULL,
        .post_handler = NULL
    },
    .kp_write = {
        .symbol_name = NULL,
        .pre_handler = NULL,
        .post_handler = NULL
    },
    .kp_lseek = {
        .symbol_name = NULL,
        .pre_handler = NULL,
        .post_handler = NULL
    },
    .kp_close = {
        .symbol_name = NULL,
        .pre_handler = NULL,
        .post_handler = NULL
    },
};


// Sending of a message to the client (if it is registered)
static void slm_netlink_send_msg(const char* msg_body, size_t msg_len) {
    struct sk_buff* sk_buffer_out;
    struct nlmsghdr* nl_header;
    int res;

    if (!slm_unit.nl_socket || !slm_unit.client_pid) {
        return;
    }

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
    if ((res = nlmsg_unicast(slm_unit.nl_socket, sk_buffer_out, slm_unit.client_pid)) < 0) {
        pr_err("SLM: Error sending to user: %d\n", res);
    }
}

// Process of incoming messages fro user-space client (process)
static void slm_netlink_recv_msg(struct sk_buff *sk_buffer) {
    struct nlmsghdr* nl_header;

    nl_header = (struct nlmsghdr*)sk_buffer->data;
    slm_unit.client_pid = nl_header->nlmsg_pid;
    pr_info("%s: Registered client PID=%d\n",
            MODULE_NAME,
            slm_unit.client_pid);
    if (nl_header->nlmsg_len > 0 && nlmsg_data(nl_header)) {
        pr_info("%s: Registration message: %s\n",
                MODULE_NAME,
                (char*)nlmsg_data(nl_header));
    }
}

// Kprobe pre_handler
static int slm_handler_pre(struct kprobe* probe, struct pt_regs *regs) {
    char buf[MAX_PAYLOAD];

    // Prepare a message body
    snprintf(buf,
             sizeof(buf),
             "[%s] is called",
             probe->symbol_name);

    // If there is no a client registered just put appropriate message
    // into the log
    if (slm_unit.client_pid) {
        slm_netlink_send_msg(buf, strlen(buf) + 1);
    } else {
        pr_info("%s: (no client, skipping notify): %s\n",
                MODULE_NAME,
                buf);
    }
    return 0;
}

static struct sock* slm_netlink_init(const int nl_unit_id) {
    struct netlink_kernel_cfg nl_cfg = {
        .input = slm_netlink_recv_msg
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

static void slm_netlink_release(struct sock* nl_socket) {
    if (nl_socket == NULL) {
        return;
    }

    netlink_kernel_release(nl_socket);

    pr_info("%s: netlink_kernel_release() Ok\n",
            MODULE_NAME);
}

static bool slm_kprobe_setup(struct kprobe* probe,
                             const char* symbol_name,
                             kprobe_pre_handler_t pre_handler,
                             kprobe_post_handler_t post_handler) {
    if (!probe) {
        return false;
    }
    
    probe->nmissed = 0;
    probe->symbol_name = symbol_name;
    probe->offset = 0;
    probe->pre_handler = pre_handler;
    probe->post_handler = post_handler;

    if (register_kprobe(probe) == 0) {
        pr_info("%s: register_kprobe(\"%s\"): Ok\n",
                MODULE_NAME,
                symbol_name);
        return true;
    }

    pr_err("%s: register_kprobe(\"%s\") FAILED\n",
           MODULE_NAME,
           symbol_name);
    return false;
}

static void slm_kprobe_release(struct kprobe* probe) {
    if (probe == NULL ||
        probe->symbol_name == NULL) {
        return;
    }
    pr_info("%s: unregister_kprobe(\"%s\")\n",
            MODULE_NAME,
            probe->symbol_name + probe->offset);
    unregister_kprobe(probe);
}

static void slm_kprobe_release_all(void) {
    slm_kprobe_release(&slm_unit.kp_create);
    slm_kprobe_release(&slm_unit.kp_open);
    slm_kprobe_release(&slm_unit.kp_read);
    slm_kprobe_release(&slm_unit.kp_write);
    slm_kprobe_release(&slm_unit.kp_lseek);
    slm_kprobe_release(&slm_unit.kp_close);
}

static int __init slm_init(void) {
    bool init_result = false;
    struct sock* nl_socket = NULL;

    pr_info("%s: Initializing\n",
            MODULE_NAME);

    do {
        // Open NETLINK communication channel
        if ((nl_socket = slm_netlink_init(NETLINK_USER)) == NULL) {
            break;
        }

        if (slm_kprobe_setup(&slm_unit.kp_create,
                             KP_VFS_CREATE,
                             slm_handler_pre,
                             NULL) == false) {
            break;
        }
        if (slm_kprobe_setup(&slm_unit.kp_open,
                             KP_VFS_OPEN,
                             slm_handler_pre,
                             NULL) == false) {
            break;
        }
        /*
        if (slm_kprobe_setup(&slm_unit.kp_read,
                             KP_VFS_READ,
                             slm_handler_pre,
                             NULL) == false) {
            break;
        }
        if (slm_kprobe_setup(&slm_unit.kp_write,
                             KP_VFS_WRITE,
                             slm_handler_pre,
                             NULL) == false) {
            break;
        }
        if (slm_kprobe_setup(&slm_unit.kp_lseek,
                             KP_VFS_LSEEK,
                             slm_handler_pre,
                             NULL) == false) {
            break;
        }
        */
        if (slm_kprobe_setup(&slm_unit.kp_close,
                             KP_VFS_CLOSE,
                             slm_handler_pre,
                             NULL) == false) {
            break;
        }

        pr_info("%s: init Ok\n",
                MODULE_NAME);
       
        init_result = true;
    } while (false);

    if (init_result == false) {
        slm_netlink_release(nl_socket);
        slm_kprobe_release_all();
        return -1;
    }

    slm_unit.nl_socket = nl_socket;

    return 0;
}

static void __exit slm_exit(void) {
    slm_kprobe_release_all();
    slm_netlink_release(slm_unit.nl_socket);

    pr_info("%s: Module exited\n",
            MODULE_NAME);
}

module_init(slm_init);
module_exit(slm_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");
MODULE_AUTHOR("Victor Kovalevich");
MODULE_DESCRIPTION("SLM kernel module with Netlink, waits for user-space registration before sending notifications");
