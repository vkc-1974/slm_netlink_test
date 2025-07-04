// File		:slm_netlink_main.c
// Author	:Victor Kovalevich
// Created	:Wed Jul  2 12:50:38 2025
#include "slm_netlink.h"

#define KP_VFS_CREATE "__x64_sys_creat"

#define KP_VFS_OPEN     "__x64_sys_open"
#define KP_VFS_OPENAT   "__x64_sys_openat"
#define KP_VFS_OPENAT2  "__x64_sys_openat2"

#define KP_VFS_READ   "__x64_sys_read"
#define KP_VFS_WRITE  "__x64_sys_write"
#define KP_VFS_LSEEK  "__x64_sys_lseek"

#define KP_VFS_CLOSE        "__x64_sys_close"
#define KP_VFS_CLOSE_RANGE  "__x64_sys_close_range"

#define BUFFER_SIZE      3072
#define BUFFER_POOL_SIZE  256

static struct slm_netlink_context_data slm_netlink_context = {
    .nl_socket = NULL,
    .client_pid = 0,
    .kp_vfs_creat = {
        .nmissed = 0,
        .symbol_name = "__x64_sys_creat",
        .offset = 0,
        .pre_handler = slm_netlink_kprobe_handler_vfs_creat_pre,
        .post_handler = NULL
    },
    .kp_vfs_open = {
        .nmissed = 0,
        .symbol_name = "__x64_sys_open",
        .offset = 0,
        .pre_handler = slm_netlink_kprobe_handler_vfs_open_pre,
        .post_handler = NULL
    },
    .kp_vfs_openat = {
        .nmissed = 0,
        .symbol_name = "__x64_sys_openat",
        .offset = 0,
        .pre_handler = slm_netlink_kprobe_handler_vfs_openat_pre,
        .post_handler = NULL
    },
    .kp_vfs_openat2 = {
        .nmissed = 0,
        .symbol_name = "__x64_sys_openat2",
        .offset = 0,
        .pre_handler = slm_netlink_kprobe_handler_vfs_openat2_pre,
        .post_handler = NULL
    },
    .kp_vfs_close = {
        .nmissed = 0,
        .symbol_name = "__x64_sys_close",
        .offset = 0,
        .pre_handler = slm_netlink_kprobe_handler_vfs_close_pre,
        .post_handler = NULL
    },
    .kp_vfs_close_range = {
        .nmissed = 0,
        .symbol_name = "__x64_sys_close_range",
        .offset = 0,
        .pre_handler = slm_netlink_kprobe_handler_vfs_close_range_pre,
        .post_handler = NULL
    },
    .buffer_pool = {
        .bufs = NULL,
        .bitmap = NULL,
        .size = 0,
        .count = 0
    }
};

static bool slm_netlink_kprobe_setup_all(void) {
    return (slm_netlink_kprobe_setup(&slm_netlink_context.kp_vfs_creat) &&
            slm_netlink_kprobe_setup(&slm_netlink_context.kp_vfs_open) &&
            slm_netlink_kprobe_setup(&slm_netlink_context.kp_vfs_openat) &&
            slm_netlink_kprobe_setup(&slm_netlink_context.kp_vfs_openat2) &&
            slm_netlink_kprobe_setup(&slm_netlink_context.kp_vfs_close) &&
            slm_netlink_kprobe_setup(&slm_netlink_context.kp_vfs_close_range));
}

static void slm_netlink_kprobe_release_all(void) {
    slm_netlink_kprobe_release(&slm_netlink_context.kp_vfs_creat);
    slm_netlink_kprobe_release(&slm_netlink_context.kp_vfs_open);
    slm_netlink_kprobe_release(&slm_netlink_context.kp_vfs_openat);
    slm_netlink_kprobe_release(&slm_netlink_context.kp_vfs_openat2);
    slm_netlink_kprobe_release(&slm_netlink_context.kp_vfs_close);
    slm_netlink_kprobe_release(&slm_netlink_context.kp_vfs_close_range);
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

bool slm_netlink_context_check_uid(const int uid) {
    return true;
}

bool slm_netlink_context_buffer_pool_init(struct slm_netlink_buffer_pool* pool,
                                          unsigned int count,
                                          unsigned int size) {
    if (pool == NULL ||
        count == 0 ||
        size == 0) {
        return false;
    }
    unsigned int i;
    void** bufs = NULL;
    unsigned long* bitmap = NULL;
    bool init_res = false;

    do {
        if ((bufs = kcalloc(count, sizeof(void *), GFP_KERNEL)) == NULL) {
            pr_err("%s: unable to allocate memory segement for memory buffer pool",
                   MODULE_NAME);
            break;
        }
        if ((bitmap = kcalloc(BITS_TO_LONGS(count), sizeof(unsigned long), GFP_KERNEL)) == NULL) {
            pr_err("%s: unable to allocate bitmap structure for memory buffer pool",
                   MODULE_NAME);
            break;
        }
        for (i = 0; i < count; i++) {
            if ((bufs[i] = kmalloc(size, GFP_KERNEL)) == NULL) {
                pr_err("%s: unable to allocate %d buffer for memory buffer pool",
                       MODULE_NAME,
                       i);
                break;
            }
        }
        if (i < count) {
            break;
        }
        init_res = true;
    } while (false);

    if (init_res == true) {
        pool->bufs = bufs;
        pool->bitmap = bitmap;
        pool->size = size;
        pool->count = count;
        bitmap_zero(pool->bitmap, count);
        spin_lock_init(&pool->lock);
        atomic_set(&pool->in_use, 0);
        pr_info("%s: memory buffer pool with %u buffers %u bytes length is ready for use",
                MODULE_NAME,
                pool->count,
                pool->size);
        return true;
    }
    while (i--) {
        kfree(bufs[i]);
    }
    if (bitmap) {
        kfree(bitmap);
    }
    if (bufs) {
         kfree(bufs);
    }
    return false;
}

void slm_netlink_context_buffer_pool_destroy(struct slm_netlink_buffer_pool* pool) {
    unsigned int i;
    if (!pool ||
        !pool->bufs ||
        !pool->bitmap) {
        return;
    }
    for (i = 0; i < pool->count; i++) {
        kfree(pool->bufs[i]);
    }
    kfree(pool->bitmap);
    kfree(pool->bufs);
    pool->bufs = NULL;
    pool->bitmap = NULL;
    pool->count = 0;
    pool->size = 0;
}

void* slm_netlink_context_buffer_pool_alloc(void) {
    unsigned long flags;
    unsigned int i;
    void *buf = NULL;

    spin_lock_irqsave(&slm_netlink_context.buffer_pool.lock, flags);
    i = find_first_zero_bit(slm_netlink_context.buffer_pool.bitmap,
                            slm_netlink_context.buffer_pool.count);
    if (i < slm_netlink_context.buffer_pool.count) {
        set_bit(i, slm_netlink_context.buffer_pool.bitmap);
        buf = slm_netlink_context.buffer_pool.bufs[i];
        atomic_inc(&slm_netlink_context.buffer_pool.in_use);
    }
    spin_unlock_irqrestore(&slm_netlink_context.buffer_pool.lock, flags);

    return buf;
}

unsigned int slm_netlink_context_buffer_pool_get_buffer_size(void) {
    return slm_netlink_context.buffer_pool.size;
}

bool slm_netlink_context_buffer_pool_free(void* buf) {
    unsigned long flags;
    unsigned int i;
    bool found = false;

    spin_lock_irqsave(&slm_netlink_context.buffer_pool.lock, flags);
    for (i = 0; i < slm_netlink_context.buffer_pool.count; i++) {
        if (slm_netlink_context.buffer_pool.bufs[i] == buf) {
            if (test_and_clear_bit(i, slm_netlink_context.buffer_pool.bitmap)) {
                atomic_dec(&slm_netlink_context.buffer_pool.in_use);
                found = true;
            }
            break;
        }
    }
    spin_unlock_irqrestore(&slm_netlink_context.buffer_pool.lock, flags);
    return found;
}

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

        if (slm_netlink_kprobe_setup_all() == false) {
            break;
        }

        if ((proc_dir = slm_netlink_procfs_setup()) == NULL) {
            break;
        }

        if (slm_netlink_context_buffer_pool_init(&slm_netlink_context.buffer_pool,
                                                 BUFFER_POOL_SIZE,
                                                 BUFFER_SIZE) == false) {
            break;
        }

        pr_info("%s: init Ok\n",
                MODULE_NAME);

        init_result = true;
    } while (false);

    if (init_result == false) {
        slm_netlink_context_buffer_pool_free(&slm_netlink_context.buffer_pool);
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
    slm_netlink_context_buffer_pool_free(&slm_netlink_context.buffer_pool);
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
