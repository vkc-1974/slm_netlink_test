// File		:slm_netlink.h
// Author	:Victor Kovalevich
// Created	:Thu Jul  3 18:01:22 2025
#ifndef __SLM_NETLINK_H__
#define __SLM_NETLINK_H__

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
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define NETLINK_USER 25
#define MAX_PAYLOAD 128
#define MODULE_NAME                "slm_netlink"
#define PROCFS_SLM_NETLINK_DIR     (MODULE_NAME)
#define PROCFS_SLM_NETLINK_UNIT_ID "unit_id"
#define PROCFS_SLM_NETLINK_FILTERS "filters"
#define PROCFS_SLM_NETLINK_PIDS    "pids"

struct slm_netlink_context_data {
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
    //
    // Procfs directory with module specific entries
    struct proc_dir_entry* proc_dir;
};

//
// Module context specific routines
int slm_netlink_context_get_client_pid(void);
int slm_netlink_context_set_client_pid(const int new_client_pid);
struct sock* slm_netlink_context_get_nl_socket(void);
///// struct sock* slm_netlink_context_set_nl_socket(struct sock* new_nl_socket);

//
// NETLINK communication specific routines
struct sock* slm_netlink_socket_init(const int nl_unit_id);
void slm_netlink_socket_release(struct sock* nl_socket);
void slm_netlink_socket_send_msg(const char* msg_body,
                                 size_t msg_len);

//
// Kprobe specific routines
int slm_netlink_kprobe_handler_pre(struct kprobe* probe,
                                   struct pt_regs *regs);
bool slm_netlink_kprobe_setup(struct kprobe* probe,
                              const char* symbol_name,
                              kprobe_pre_handler_t pre_handler,
                              kprobe_post_handler_t post_handler);
void slm_netlink_kprobe_release(struct kprobe* probe);

//
// Procfs specific routines
struct proc_dir_entry* slm_netlink_procfs_setup(void);
void slm_netlink_procfs_release(void);



#endif //  !__SLM_NETLINK_H__
