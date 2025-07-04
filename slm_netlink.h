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
#include <linux/spinlock.h>
#include <linux/bitmap.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/errno.h>

#define NETLINK_USER 25
#define MAX_PAYLOAD 1024
#define MODULE_NAME                "slm_netlink"
#define PROCFS_SLM_NETLINK_DIR     (MODULE_NAME)
#define PROCFS_SLM_NETLINK_UNIT_ID "unit_id"
#define PROCFS_SLM_NETLINK_FILTERS "filters"
#define PROCFS_SLM_NETLINK_PIDS    "pids"

struct slm_netlink_buffer_pool {
    void **bufs;
    unsigned long *bitmap;
    unsigned int size;
    unsigned int count;
    spinlock_t lock;
    atomic_t in_use;
};

static inline int slm_netlink_buffer_pool_in_use(struct slm_netlink_buffer_pool* pool)
{
    return atomic_read(&pool->in_use);
}

struct slm_netlink_context_data {
    //
    // NETLINK socket (communication with a userspace process (client)
    struct sock* nl_socket;
    //
    // Userspace client PID
    int client_pid;
    //
    // Kprobes
    struct kprobe kp_vfs_creat;
    struct kprobe kp_vfs_open;
    struct kprobe kp_vfs_openat;
    struct kprobe kp_vfs_openat2;
    struct kprobe kp_vfs_close;
    struct kprobe kp_vfs_close_range;
    //
    // Procfs directory with module specific entries
    struct proc_dir_entry* proc_dir;
    //
    // Pool of the buffers used to prepare ans send the messages
    // via NETLINK channel
    struct slm_netlink_buffer_pool buffer_pool;
};

//
// Module context specific routines
int slm_netlink_context_get_client_pid(void);
int slm_netlink_context_set_client_pid(const int new_client_pid);
struct sock* slm_netlink_context_get_nl_socket(void);
bool slm_netlink_context_check_uid(const int uid);
bool slm_netlink_context_buffer_pool_init(struct slm_netlink_buffer_pool* pool,
                                          unsigned int count,
                                          unsigned int size);
void slm_netlink_context_buffer_pool_destroy(struct slm_netlink_buffer_pool* pool);
void* slm_netlink_context_buffer_pool_alloc(void);
unsigned int slm_netlink_context_buffer_pool_get_buffer_size(void);
bool slm_netlink_context_buffer_pool_free(void* buf);

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

int slm_netlink_kprobe_handler_vfs_creat_pre(struct kprobe* probe,
                                             struct pt_regs *regs);
int slm_netlink_kprobe_handler_vfs_open_pre(struct kprobe* probe,
                                            struct pt_regs *regs);
int slm_netlink_kprobe_handler_vfs_openat_pre(struct kprobe* probe,
                                            struct pt_regs *regs);
int slm_netlink_kprobe_handler_vfs_openat2_pre(struct kprobe* probe,
                                               struct pt_regs *regs);
int slm_netlink_kprobe_handler_vfs_close_pre(struct kprobe* probe,
                                             struct pt_regs *regs);
int slm_netlink_kprobe_handler_vfs_close_range_pre(struct kprobe* probe,
                                                   struct pt_regs *regs);

bool slm_netlink_kprobe_setup(struct kprobe* probe);
void slm_netlink_kprobe_release(struct kprobe* probe);

//
// Procfs specific routines
struct proc_dir_entry* slm_netlink_procfs_setup(void);
void slm_netlink_procfs_release(void);



#endif //  !__SLM_NETLINK_H__
