// File		:slm_netlink_main.c
// Author	:Victor Kovalevich
// Created	:Wed Jul  2 12:50:38 2025
#include "slm_netlink.h"

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

static struct slm_netlink_context_data slm_netlink_context /*slm_unit*/ = {
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

static void slm_netlink_kprobe_release_all(void) {
    slm_netlink_kprobe_release(&slm_netlink_context.kp_create);
    slm_netlink_kprobe_release(&slm_netlink_context.kp_open);
    slm_netlink_kprobe_release(&slm_netlink_context.kp_read);
    slm_netlink_kprobe_release(&slm_netlink_context.kp_write);
    slm_netlink_kprobe_release(&slm_netlink_context.kp_lseek);
    slm_netlink_kprobe_release(&slm_netlink_context.kp_close);
}

int slm_netlink_context_get_client_pid(void) {
    return slm_netlink_context.client_pid;
}

int slm_netlink_context_set_client_pid(const int new_client_pid) {
    int result = slm_netlink_context.client_pid;
    slm_netlink_context.client_pid = new_client_pid;
    return result;
}

struct sock* slm_netlink_context_get_nl_socket(void) {
    return slm_netlink_context.nl_socket;
}

///// struct sock* slm_netlink_context_set_nl_socket(struct sock* new_nl_socket) {
/////     struct sock* result = slm_netlink_context.nl_socket;
/////     slm_netlink_context.nl_socket = new_nl_socket;
/////     return result;
///// }

static int __init slm_init(void) {
    bool init_result = false;
    struct sock* nl_socket = NULL;
    struct proc_dir_entry* proc_dir = NULL;

    pr_info("%s: Initializing\n",
            MODULE_NAME);

    do {
        // Open NETLINK communication channel
        if ((nl_socket = slm_netlink_socket_init(NETLINK_USER)) == NULL) {
            break;
        }

        if (slm_netlink_kprobe_setup(&slm_netlink_context.kp_create,
                                     KP_VFS_CREATE,
                                     slm_netlink_kprobe_handler_pre,
                                     NULL) == false) {
            break;
        }
        if (slm_netlink_kprobe_setup(&slm_netlink_context.kp_open,
                                     KP_VFS_OPEN,
                                     slm_netlink_kprobe_handler_pre,
                                     NULL) == false) {
            break;
        }
        /*
        if (slm_netlink_kprobe_setup(&slm_netlink_context.kp_read,
                                     KP_VFS_READ,
                                     slm_netlink_kprobe_handler_pre,
                                     NULL) == false) {
            break;
        }
        if (slm_netlink_kprobe_setup(&slm_netlink_context.kp_write,
                                     KP_VFS_WRITE,
                                     slm_netlink_kprobe_handler_pre,
                                     NULL) == false) {
            break;
        }
        if (slm_netlink_kprobe_setup(&slm_netlink_context.kp_lseek,
                                     KP_VFS_LSEEK,
                                     slm_netlink_kprobe_handler_pre,
                                     NULL) == false) {
            break;
        }
        */
        if (slm_netlink_kprobe_setup(&slm_netlink_context.kp_close,
                                     KP_VFS_CLOSE,
                                     slm_netlink_kprobe_handler_pre,
                                     NULL) == false) {
            break;
        }

        if ((proc_dir = slm_netlink_procfs_setup()) == NULL) {
            break;
        }

        pr_info("%s: init Ok\n",
                MODULE_NAME);

        init_result = true;
    } while (false);

    if (init_result == false) {
        slm_netlink_procfs_release();
        slm_netlink_kprobe_release_all();
        slm_netlink_socket_release(nl_socket);
        return -1;
    }

    slm_netlink_context.nl_socket = nl_socket;
    slm_netlink_context.proc_dir = proc_dir;

    return 0;
}

static void __exit slm_exit(void) {
    slm_netlink_procfs_release();
    slm_netlink_kprobe_release_all();
    slm_netlink_socket_release(slm_netlink_context_get_nl_socket());

    pr_info("%s: Module exited\n",
            MODULE_NAME);
}

module_init(slm_init);
module_exit(slm_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.3");
MODULE_AUTHOR("Victor Kovalevich");
MODULE_DESCRIPTION("SLM kernel module with Netlink, waits for user-space registration before sending notifications");
