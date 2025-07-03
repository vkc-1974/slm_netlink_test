// File		:slm_netlink_kprobe.c
// Author	:Victor Kovalevich
// Created	:Thu Jul  3 19:49:25 2025
#include "slm_netlink.h"

// Process of incoming messages fro user-space client (process)
// Kprobe pre_handler
int slm_netlink_kprobe_handler_pre(struct kprobe* probe, struct pt_regs *regs) {
    char buf[MAX_PAYLOAD];

    // Prepare a message body
    snprintf(buf,
             sizeof(buf),
             "[%s] is called",
             probe->symbol_name);

    // If there is no a client registered just put appropriate message
    // into the log
    if (slm_netlink_context_get_client_pid() > 0) {
        slm_netlink_socket_send_msg(buf,
                                    strlen(buf) + 1);
    } else {
        pr_info("%s: (no client, skipping notify): %s\n",
                MODULE_NAME,
                buf);
    }
    return 0;
}

bool slm_netlink_kprobe_setup(struct kprobe* probe,
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

