// File		:slm_netlink_kprobe.c
// Author	:Victor Kovalevich
// Created	:Thu Jul  3 19:49:25 2025
#include "slm_netlink.h"

#define SILENT_MODE

// Process of incoming messages fro user-space client (process)
// Kprobe pre_handler
int slm_netlink_kprobe_handler_pre(struct kprobe* probe, struct pt_regs *regs) {
    char buf[MAX_PAYLOAD];

    // Prepare a message body
    snprintf(buf,
             sizeof(buf),
             "[%s] is called by PID: %d (%s), UID: %d",
             probe->symbol_name,
             current->pid,
             current->comm,
             current_uid().val);

    // If there is no a client registered just put appropriate message
    // into the log
    if (slm_netlink_context_get_client_pid() > 0) {
        if (slm_netlink_context_check_uid(current_uid().val) == true) {
            slm_netlink_socket_send_msg(buf,
                                        strlen(buf) + 1);
        }
    } else {
#if !defined(SILENT_MODE)
        pr_info("%s: (no client, skipping notify): %s\n",
                MODULE_NAME,
                buf);
#endif //  !SILENT_MODE
    }
    return 0;
}

//
// KProbe pre handler to __x64_sys_creat routine
int slm_netlink_kprobe_handler_vfs_creat_pre(struct kprobe* probe,
                                             struct pt_regs *regs) {
    char buf[MAX_PAYLOAD];

    // Prepare a message body
    snprintf(buf,
             sizeof(buf),
             "[%s] is called by PID: %d (%s), UID: %d",
             probe->symbol_name,
             current->pid,
             current->comm,
             current_uid().val);

    // If there is no a client registered just put appropriate message
    // into the log
    if (slm_netlink_context_get_client_pid() > 0) {
        if (slm_netlink_context_check_uid(current_uid().val) == true) {
            slm_netlink_socket_send_msg(buf,
                                        strlen(buf) + 1);
        }
    } else {
#if !defined(SILENT_MODE)
        pr_info("%s: (no client, skipping notify): %s\n",
                MODULE_NAME,
                buf);
#endif //  !SILENT_MODE
    }
    return 0;
}

//
// KProbe pre handler to __x64_sys_open routine
int slm_netlink_kprobe_handler_vfs_open_pre(struct kprobe* probe,
                                            struct pt_regs *regs) {
    char buf[MAX_PAYLOAD];

    // Prepare a message body
    snprintf(buf,
             sizeof(buf),
             "[%s] is called by PID: %d (%s), UID: %d",
             probe->symbol_name,
             current->pid,
             current->comm,
             current_uid().val);

    // If there is no a client registered just put appropriate message
    // into the log
    if (slm_netlink_context_get_client_pid() > 0) {
        if (slm_netlink_context_check_uid(current_uid().val) == true) {
            slm_netlink_socket_send_msg(buf,
                                        strlen(buf) + 1);
        }
    } else {
#if !defined(SILENT_MODE)
     pr_info("%s: (no client, skipping notify): %s\n",
                MODULE_NAME,
                buf);
#endif //  !SILENT_MODE
    }
    return 0;
}

//
// KProbe pre handler to __x64_sys_openat routine
int slm_netlink_kprobe_handler_vfs_openat_pre(struct kprobe* probe,
                                              struct pt_regs *regs) {
    char* buf;

    if ((buf = slm_netlink_context_buffer_pool_alloc()) == NULL) {
        return 0;
    }

    // Prepare a message body
    snprintf(buf,
             slm_netlink_context_buffer_pool_get_buffer_size(),
             "[%s] is called by PID: %d (%s), UID: %d",
             probe->symbol_name,
             current->pid,
             current->comm,
             current_uid().val);

    // If there is no a client registered just put appropriate message
    // into the log
    if (slm_netlink_context_get_client_pid() > 0) {
        if (slm_netlink_context_check_uid(current_uid().val) == true) {
            slm_netlink_socket_send_msg(buf,
                                        strlen(buf) + 1);
        }
    } else {
#if !defined(SILENT_MODE)
        pr_info("%s: (no client, skipping notify): %s\n",
                MODULE_NAME,
                buf);
#endif //  !SILENT_MODE
    }
    slm_netlink_context_buffer_pool_free(buf);
    return 0;
}

//
// KProbe pre handler to __x64_sys_openat2 routine
int slm_netlink_kprobe_handler_vfs_openat2_pre(struct kprobe* probe,
                                               struct pt_regs *regs) {
    char* buf;

    if ((buf = slm_netlink_context_buffer_pool_alloc()) == NULL) {
        pr_err("%s: slm_netlink_context_buffer_pool_alloc() -> NULL\n",
               MODULE_NAME);
        return 0;
    }

    // Prepare a message body
    snprintf(buf,
             slm_netlink_context_buffer_pool_get_buffer_size(),
             "[%s] is called by PID: %d (%s), UID: %d",
             probe->symbol_name,
             current->pid,
             current->comm,
             current_uid().val);

    // If there is no a client registered just put appropriate message
    // into the log
    if (slm_netlink_context_get_client_pid() > 0) {
        if (slm_netlink_context_check_uid(current_uid().val) == true) {
            slm_netlink_socket_send_msg(buf,
                                        strlen(buf) + 1);
        }
    } else {
#if !defined(SILENT_MODE)
        pr_info("%s: (no client, skipping notify): %s\n",
                MODULE_NAME,
                buf);
#endif //  !SILENT_MODE
    }
    slm_netlink_context_buffer_pool_free(buf);
    return 0;
}

//
// KProbe pre handler to __x64_sys_close routine
int slm_netlink_kprobe_handler_vfs_close_pre(struct kprobe* probe,
                                             struct pt_regs *regs) {
    char* buf;

    if ((buf = slm_netlink_context_buffer_pool_alloc()) == NULL) {
        return 0;
    }

    // Prepare a message body
    snprintf(buf,
             slm_netlink_context_buffer_pool_get_buffer_size(),
             "[%s] is called by PID: %d (%s), UID: %d",
             probe->symbol_name,
             current->pid,
             current->comm,
             current_uid().val);

    // If there is no a client registered just put appropriate message
    // into the log
    if (slm_netlink_context_get_client_pid() > 0) {
        if (slm_netlink_context_check_uid(current_uid().val) == true) {
            slm_netlink_socket_send_msg(buf,
                                        strlen(buf) + 1);
        }
    } else {
#if !defined(SILENT_MODE)
        pr_info("%s: (no client, skipping notify): %s\n",
                MODULE_NAME,
                buf);
#endif //  !SILENT_MODE
    }
    slm_netlink_context_buffer_pool_free(buf);
    return 0;
}

//
// KProbe pre handler to __x64_sys_close_range routine
int slm_netlink_kprobe_handler_vfs_close_range_pre(struct kprobe* probe,
                                                   struct pt_regs *regs) {
    char* buf;

    if ((buf = slm_netlink_context_buffer_pool_alloc()) == NULL) {
        return 0;
    }

    // Prepare a message body
    snprintf(buf,
             slm_netlink_context_buffer_pool_get_buffer_size(),
             "[%s] is called by PID: %d (%s), UID: %d",
             probe->symbol_name,
             current->pid,
             current->comm,
             current_uid().val);

    // If there is no a client registered just put appropriate message
    // into the log
    if (slm_netlink_context_get_client_pid() > 0) {
        if (slm_netlink_context_check_uid(current_uid().val) == true) {
            slm_netlink_socket_send_msg(buf,
                                        strlen(buf) + 1);
        }
    } else {
#if !defined(SILENT_MODE)
        pr_info("%s: (no client, skipping notify): %s\n",
                MODULE_NAME,
                buf);
#endif //  !SILENT_MODE
    }
    slm_netlink_context_buffer_pool_free(buf);
    return 0;
}

bool slm_netlink_kprobe_setup(struct kprobe* probe) {
    if (probe == NULL ||
        probe->symbol_name == NULL ||
        (probe->pre_handler == NULL && probe->post_handler == NULL)) {
        return false;
    }

    if (register_kprobe(probe) == 0) {
        pr_info("%s: register_kprobe(\"%s\"): Ok\n",
                MODULE_NAME,
                probe->symbol_name + probe->offset);

        return true;
    }

    pr_err("%s: register_kprobe(\"%s\") FAILED\n",
           MODULE_NAME,
           probe->symbol_name + probe->offset);
    return false;
}

void slm_netlink_kprobe_release(struct kprobe* probe) {
    if (probe == NULL ||
        probe->symbol_name == NULL) {
        return;
    }
    pr_info("%s: unregister_kprobe(\"%s\")\n",
            MODULE_NAME,
            probe->symbol_name + probe->offset);
    unregister_kprobe(probe);
}

