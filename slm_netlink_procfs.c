// File		:slm_netlink_procfs.c
// Author	:Victor Kovalevich
// Created	:Thu Jul  3 21:43:54 2025
#include "slm_netlink.h"

static int slm_netlink_procfs_unitid_show(struct seq_file* m, void* v) {
    seq_printf(m, "%d\n", NETLINK_USER);
    return 0;
}

static int slm_netlink_procfs_filters_show(struct seq_file* m, void* v) {
    seq_printf(m, "%s\n", "filters - NOT IMPLEMENTED YET");
    return 0;
}

static int slm_netlink_procfs_pids_show(struct seq_file* m, void* v) {
    seq_printf(m, "%s\n", "pids - NOT IMPLEMENTED YET");
    return 0;
}

static int slm_netlink_procfs_unitid_open(struct inode* inode, struct file* file) {
    return single_open(file, slm_netlink_procfs_unitid_show, NULL);
}

static int slm_netlink_procfs_filters_open(struct inode* inode, struct file* file) {
    return single_open(file, slm_netlink_procfs_filters_show, NULL);
}

static int slm_netlink_procfs_pids_open(struct inode* inode, struct file* file) {
    return single_open(file, slm_netlink_procfs_pids_show, NULL);
}

static const struct proc_ops slm_netlink_procfs_unitid_ops = {
    .proc_open = slm_netlink_procfs_unitid_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static const struct proc_ops slm_netlink_procfs_filters_ops = {
    .proc_open = slm_netlink_procfs_filters_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static const struct proc_ops slm_netlink_procfs_pids_ops = {
    .proc_open = slm_netlink_procfs_pids_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

struct proc_dir_entry* slm_netlink_procfs_setup(void) {
    struct proc_dir_entry* slm_proc_dir;

    if ((slm_proc_dir = proc_mkdir(PROCFS_SLM_NETLINK_DIR, NULL)) == NULL) {
        pr_err("%s: unable to create /proc/%s\n",
               MODULE_NAME,
               PROCFS_SLM_NETLINK_DIR);
        return NULL;
    }

    proc_create(PROCFS_SLM_NETLINK_UNIT_ID, 0444, slm_proc_dir, &slm_netlink_procfs_unitid_ops);
    proc_create(PROCFS_SLM_NETLINK_FILTERS, 0444, slm_proc_dir, &slm_netlink_procfs_filters_ops);
    proc_create(PROCFS_SLM_NETLINK_PIDS,    0444, slm_proc_dir, &slm_netlink_procfs_pids_ops);

    return slm_proc_dir;
}

void slm_netlink_procfs_release(void) {
    remove_proc_subtree(PROCFS_SLM_NETLINK_DIR, NULL);
}
