/*
 * $Copyright: 2017-2024 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 */

/*
 * Middle-driver for communication between Linux KNET driver and
 * drivers support Generic Netlink channel.
 *
 * This code is used to integrate packet sampling from KNET Rx filter
 * call-back function to the psample infra (kernel/linux/net/psample)
 * for sending sampled packets to userspace sflow applications such as
 * Host Sflow (https://github.com/sflow/host-sflow) using Generic Netlink
 * interfaces.
 *
 * This driver is also built with the DCB library as the helper for parsing
 * the RX packet meta data from the Linux KNET driver filter call-back function.
 * The environment DCBDIR must be set to indicate the directroy of the DCB
 * library.
 */


#include <gmodule.h> /* Must be included first */
#include <linux-bde.h>
#include <kcom.h>
#include <bcm-knet.h>
#include <linux/if_vlan.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/netdevice.h>
#include <net/net_namespace.h>
#include "bcm-genl-psample.h"
#include "bcm-genl-dev.h"
#include "bcm-genl-netif.h"
#if BCMGENL_PSAMPLE_SUPPORT
#include <net/psample.h>
#include <uapi/linux/psample.h>

#define PSAMPLE_CB_DBG
#ifdef PSAMPLE_CB_DBG
static int debug;
#define PSAMPLE_CB_DBG_PRINT(...) \
    if (debug & 0x1) {         \
        gprintk(__VA_ARGS__);  \
    }
#else
#define PSAMPLE_CB_DBG_PRINT(...)
#endif

#define FCS_SZ 4
#define PSAMPLE_NLA_PADDING 4

#define PSAMPLE_RATE_DFLT 1
#define PSAMPLE_SIZE_DFLT 128
static int psample_size = PSAMPLE_SIZE_DFLT;
LKM_MOD_PARAM(psample_size, "i", int, 0);
MODULE_PARM_DESC(psample_size,
"psample pkt size (default 128 bytes)");

#define PSAMPLE_QLEN_DFLT 1024
static int psample_qlen = PSAMPLE_QLEN_DFLT;
LKM_MOD_PARAM(psample_qlen, "i", int, 0);
MODULE_PARM_DESC(psample_qlen,
"psample queue length (default 1024 buffers)");

/* driver proc entry root */
static struct proc_dir_entry *psample_proc_root = NULL;
static char psample_procfs_path[80];

/* psample general info */
typedef struct psample_info_s {
    struct net *netns;
    struct list_head group_list;
    uint64_t rx_reason_sample_source[LINUX_BDE_MAX_DEVICES];
} psample_info_t;
static psample_info_t g_psample_info;

typedef struct psample_group_data_s {
    struct list_head list;
    struct psample_group *group;
    uint32_t group_num;
} psample_group_data_t;

/* Maintain sampled pkt statistics */
typedef struct psample_stats_s {
    unsigned long pkts_f_psample_cb;
    unsigned long pkts_f_psample_mod;
    unsigned long pkts_f_handled;
    unsigned long pkts_f_pass_through;
    unsigned long pkts_f_dst_mc;
    unsigned long pkts_c_qlen_cur;
    unsigned long pkts_c_qlen_hi;
    unsigned long pkts_d_qlen_max;
    unsigned long pkts_d_no_mem;
    unsigned long pkts_d_no_group;
    unsigned long pkts_d_sampling_disabled;
    unsigned long pkts_d_not_ready;
    unsigned long pkts_d_metadata;
    unsigned long pkts_d_meta_srcport;
    unsigned long pkts_d_meta_dstport;
    unsigned long pkts_d_invalid_size;
} psample_stats_t;
static psample_stats_t g_psample_stats;

typedef struct psample_meta_s {
    int trunc_size;
    int src_ifindex;
    int dst_ifindex;
    int sample_rate;
} psample_meta_t;

typedef struct psample_pkt_s {
    struct list_head list;
    struct psample_group *group;
    psample_meta_t meta;
    struct sk_buff *skb;
} psample_pkt_t;

typedef struct psample_work_s {
    struct list_head pkt_list;
    struct work_struct wq;
    spinlock_t lock;
} psample_work_t;
static psample_work_t g_psample_work;

static struct psample_group *
psample_group_get_from_list(uint32_t grp_num)
{
    struct list_head *list_ptr;
    psample_group_data_t *grp;

    list_for_each(list_ptr, &g_psample_info.group_list) {
        grp = list_entry(list_ptr, psample_group_data_t, list);
        if (grp->group_num == grp_num) {
            return grp->group;
        }
    }

    if ((grp = kmalloc(sizeof(psample_group_data_t), GFP_ATOMIC)) == NULL) {
        return NULL;
    }
    grp->group = psample_group_get(g_psample_info.netns, grp_num);
    if (grp->group == NULL) {
        kfree(grp);
        return NULL;
    }
    grp->group_num = grp_num;
    list_add_tail(&grp->list, &g_psample_info.group_list);

    return grp->group;
}

static int
psample_meta_srcport_get(int dev_no, void *pkt_meta)
{
    uint32_t p;

    if (bcmgenl_dev_pktmeta_rx_srcport_get(dev_no, pkt_meta, &p) < 0) {
        return -1;
    }

    return p;
}

static int
psample_meta_dstport_get(int dev_no, void *pkt_meta, bool *is_mcast)
{
    bool mcast;
    uint32_t p;

    if (bcmgenl_dev_pktmeta_rx_dstport_get(dev_no, pkt_meta, &mcast, &p) < 0) {
        return -1;
    }
    if (is_mcast) {
        *is_mcast = mcast;
    }

    return p;
}

static int
psample_meta_sample_reason(int dev_no, void *pkt_meta)
{
    uint64_t rx_reason;
    uint64_t *exp_reason = &g_psample_info.rx_reason_sample_source[dev_no];

    if (bcmgenl_dev_pktmeta_rx_reason_get(dev_no, pkt_meta, &rx_reason) < 0) {
        return 0;
    }
    if (*exp_reason == 0) {
        if (bcmgenl_dev_rx_reason_sample_source_get(dev_no, exp_reason) < 0) {
            return 0;
        }
    }

    /* Check if only sample reason code is set.
     * If only sample reason code, then consume pkt.
     * If other reason codes exist, then pkt should be
     * passed through to Linux network stack.
     */
    if ((rx_reason & *exp_reason) == *exp_reason) {
        return 1;
    }
    return 0;
}

static int
psample_meta_get(int dev_no, kcom_filter_t *kf, void *pkt_meta,
                 psample_meta_t *sflow_meta)
{
    bool mcast = false;
    int srcport, dstport;
    int src_ifindex = 0;
    int dst_ifindex = 0;
    int sample_rate = 1;
    int sample_size = PSAMPLE_SIZE_DFLT;
    bcmgenl_netif_t bcmgenl_netif;

#ifdef PSAMPLE_CB_DBG
    if (debug & 0x1) {
        int i=0;
        uint8_t *meta = (uint8_t *)pkt_meta;
        PSAMPLE_CB_DBG_PRINT("%s: psample pkt metadata\n", __func__);
        for (i=0; i<64; i+=16) {
            PSAMPLE_CB_DBG_PRINT("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    meta[i+0], meta[i+1], meta[i+2], meta[i+3], meta[i+4], meta[i+5], meta[i+6], meta[i+7],
                    meta[i+8], meta[i+9], meta[i+10], meta[i+11], meta[i+12], meta[i+13], meta[i+14], meta[i+15]);
        }
    }
#endif

    /* parse pkt metadata for src and dst ports */
    srcport = psample_meta_srcport_get(dev_no, pkt_meta);
    dstport = psample_meta_dstport_get(dev_no, pkt_meta, &mcast);
    if ((srcport == -1) || (dstport == -1)) {
        gprintk("%s: invalid srcport %d or dstport %d\n", __func__, srcport, dstport);
        return -1;
    }

    /* find src port netif (no need to lookup CPU port) */
    if (srcport != 0) {
        if (bcmgenl_netif_get_by_port(srcport, &bcmgenl_netif) == 0) {
            src_ifindex = bcmgenl_netif.dev->ifindex;
            sample_rate = bcmgenl_netif.sample_rate;
            sample_size = bcmgenl_netif.sample_size;
        } else {
            g_psample_stats.pkts_d_meta_srcport++;
            PSAMPLE_CB_DBG_PRINT("%s: could not find srcport(%d)\n", __func__, srcport);
        }
    }

    /* set sFlow dst type for MC pkts */
    if (mcast) {
        g_psample_stats.pkts_f_dst_mc++;
    /* find dst port netif for UC pkts (no need to lookup CPU port) */
    } else if (dstport != 0) {
        if (bcmgenl_netif_get_by_port(dstport, &bcmgenl_netif) == 0) {
            dst_ifindex = bcmgenl_netif.dev->ifindex;
        } else {
            g_psample_stats.pkts_d_meta_dstport++;
            PSAMPLE_CB_DBG_PRINT("%s: could not find dstport(%d)\n", __func__, dstport);
        }
    }

    PSAMPLE_CB_DBG_PRINT("%s: dstport %d, src_ifindex 0x%x, dst_ifindex 0x%x\n",
                         __func__, dstport, src_ifindex, dst_ifindex);

    sflow_meta->src_ifindex = src_ifindex;
    sflow_meta->dst_ifindex = dst_ifindex;
    sflow_meta->trunc_size  = sample_size;
    sflow_meta->sample_rate = sample_rate;
    return 0;
}

static void
psample_task(struct work_struct *work)
{
    psample_work_t *psample_work = container_of(work, psample_work_t, wq);
    unsigned long flags;
    struct list_head *list_ptr, *list_next;
    psample_pkt_t *pkt;

    spin_lock_irqsave(&psample_work->lock, flags);
    list_for_each_safe(list_ptr, list_next, &psample_work->pkt_list) {
        /* dequeue pkt from list */
        pkt = list_entry(list_ptr, psample_pkt_t, list);
        list_del(list_ptr);
        g_psample_stats.pkts_c_qlen_cur--;
        spin_unlock_irqrestore(&psample_work->lock, flags);
 
        /* send to psample */
        if (pkt) {
            PSAMPLE_CB_DBG_PRINT("%s: group 0x%x, trunc_size %d, src_ifdx 0x%x, dst_ifdx 0x%x, sample_rate %d\n",
                    __func__, pkt->group->group_num, 
                    pkt->meta.trunc_size, pkt->meta.src_ifindex, 
                    pkt->meta.dst_ifindex, pkt->meta.sample_rate);

            psample_sample_packet(pkt->group, 
                                  pkt->skb, 
                                  pkt->meta.trunc_size,
                                  pkt->meta.src_ifindex,
                                  pkt->meta.dst_ifindex,
                                  pkt->meta.sample_rate);
            g_psample_stats.pkts_f_psample_mod++;
 
            dev_kfree_skb_any(pkt->skb);
            kfree(pkt);
        }
        spin_lock_irqsave(&psample_work->lock, flags);
    }
    spin_unlock_irqrestore(&psample_work->lock, flags);
}

static int
psample_filter_cb(uint8_t *pkt, int size, int dev_no, void *pkt_meta,
                  int chan, kcom_filter_t *kf)
{
    struct psample_group *group = NULL;
    psample_meta_t meta;   
    int rv = 0;

    PSAMPLE_CB_DBG_PRINT("%s: pkt size %d, kf->dest_id %d, kf->cb_user_data %d\n",
            __func__, size, kf->dest_id, kf->cb_user_data);
    g_psample_stats.pkts_f_psample_cb++;

    /* get psample group info. psample genetlink group ID passed in kf->dest_id */
    group = psample_group_get_from_list(kf->dest_id);
    if (!group) {
        gprintk("%s: Could not find psample genetlink group %d\n", __func__, kf->cb_user_data);
        g_psample_stats.pkts_d_no_group++;
        goto PSAMPLE_FILTER_CB_PKT_HANDLED;
    }

    /* get psample metadata */
    rv = psample_meta_get(dev_no, kf, pkt_meta, &meta);
    if (rv < 0) {
        gprintk("%s: Could not parse pkt metadata\n", __func__);
        g_psample_stats.pkts_d_metadata++;
        goto PSAMPLE_FILTER_CB_PKT_HANDLED;
    }

    /* Adjust original pkt size to remove 4B FCS */
    if (size < FCS_SZ) {
        g_psample_stats.pkts_d_invalid_size++;
        goto PSAMPLE_FILTER_CB_PKT_HANDLED;
    } else {
       size -= FCS_SZ; 
    }

    /* Account for padding in libnl used by psample */
    if (meta.trunc_size >= size) {
        meta.trunc_size = size - PSAMPLE_NLA_PADDING;
    }

    PSAMPLE_CB_DBG_PRINT("%s: group 0x%x, trunc_size %d, src_ifdx 0x%x, dst_ifdx 0x%x, sample_rate %d\n",
            __func__, group->group_num, meta.trunc_size, meta.src_ifindex, meta.dst_ifindex, meta.sample_rate);

    /* drop if configured sample rate is 0 */
    if (meta.sample_rate > 0) {
        unsigned long flags;
        psample_pkt_t *psample_pkt;
        struct sk_buff *skb;

        if (g_psample_stats.pkts_c_qlen_cur >= psample_qlen) {
            gprintk("%s: tail drop due to max qlen %d reached\n", __func__, psample_qlen);
            g_psample_stats.pkts_d_qlen_max++;
            goto PSAMPLE_FILTER_CB_PKT_HANDLED;
        }

        if ((psample_pkt = kmalloc(sizeof(psample_pkt_t), GFP_ATOMIC)) == NULL) {
            gprintk("%s: failed to alloc psample mem for pkt\n", __func__);
            g_psample_stats.pkts_d_no_mem++;
            goto PSAMPLE_FILTER_CB_PKT_HANDLED;
        }
        memcpy(&psample_pkt->meta, &meta, sizeof(psample_meta_t));
        psample_pkt->group = group;

        if ((skb = dev_alloc_skb(meta.trunc_size)) == NULL) {
            gprintk("%s: failed to alloc psample mem for pkt skb\n", __func__);
            g_psample_stats.pkts_d_no_mem++;
            goto PSAMPLE_FILTER_CB_PKT_HANDLED;
        }

        /* setup skb to point to pkt */
        memcpy(skb->data, pkt, meta.trunc_size);
        skb_put(skb, meta.trunc_size);
        skb->len = meta.trunc_size;
        psample_pkt->skb = skb;

        spin_lock_irqsave(&g_psample_work.lock, flags);
        list_add_tail(&psample_pkt->list, &g_psample_work.pkt_list); 
        
        g_psample_stats.pkts_c_qlen_cur++;
        if (g_psample_stats.pkts_c_qlen_cur > g_psample_stats.pkts_c_qlen_hi) {
            g_psample_stats.pkts_c_qlen_hi = g_psample_stats.pkts_c_qlen_cur;
        }

        schedule_work(&g_psample_work.wq);
        spin_unlock_irqrestore(&g_psample_work.lock, flags);
    } else {
        g_psample_stats.pkts_d_sampling_disabled++;
    }    

PSAMPLE_FILTER_CB_PKT_HANDLED:
    /* if sample reason only, consume pkt. else pass through */
    rv = psample_meta_sample_reason(dev_no, pkt_meta);
    if (rv) {
        g_psample_stats.pkts_f_handled++;
    } else {
        g_psample_stats.pkts_f_pass_through++;
    }
    return rv;
}

/*
 * psample rate Proc Read Entry
 */
static int
proc_rate_show(void *cb_data, bcmgenl_netif_t *netif)
{
    struct seq_file *m = (struct seq_file *)cb_data;
    
    seq_printf(m, "  %-14s %d\n",
               netif->dev->name, netif->sample_rate);
    return 0;
}

static int
psample_proc_rate_show(struct seq_file *m, void *v)
{
    bcmgenl_netif_search(NULL, proc_rate_show, (void *)m);
    return 0;
}

static int
psample_proc_rate_open(struct inode * inode, struct file * file)
{
    return single_open(file, psample_proc_rate_show, NULL);
}

/*
 * psample rate Proc Write Entry
 *
 *   Syntax:
 *   <netif>=<pkt sample rate>
 *
 *   Where <netif> is a virtual network interface name.
 *
 *   Examples:
 *   eth4=1000
 */
static int
proc_rate_write(void *cb_data, bcmgenl_netif_t *netif)
{
    uint32 sample_rate = (uint32)(uintptr_t)cb_data;

    netif->sample_rate = sample_rate;
    return 0;
}

static ssize_t
psample_proc_rate_write(struct file *file, const char *buf,
                        size_t count, loff_t *loff)
{
    int netif_cnt;
    char sample_str[40], *ptr, *newline;

    if (count > sizeof(sample_str)) {
        count = sizeof(sample_str) - 1;
        sample_str[count] = '\0';
    }
    if (copy_from_user(sample_str, buf, count)) {
        return -EFAULT;
    }
    sample_str[count] = 0;
    newline = strchr(sample_str, '\n');
    if (newline) {
        /* Chop off the trailing newline */
        *newline = '\0';
    }

    if ((ptr = strchr(sample_str, '=')) == NULL &&
        (ptr = strchr(sample_str, ':')) == NULL) {
        gprintk("Error: Pkt sample rate syntax not recognized: '%s'\n", sample_str);
        return count;
    }
    *ptr++ = 0;

    netif_cnt = bcmgenl_netif_search(sample_str, proc_rate_write,
                                     (void *)(uintptr_t)simple_strtol(ptr, NULL, 10));
    if (netif_cnt <= 0) {
        gprintk("Warning: Failed setting psample rate on "
                "unknown network interface: '%s'\n", sample_str);
    }

    return count;
}

struct proc_ops psample_proc_rate_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =      psample_proc_rate_open,
    .proc_read =      seq_read,
    .proc_lseek =     seq_lseek,
    .proc_write =     psample_proc_rate_write,
    .proc_release =   single_release,
};

/*
 * psample size Proc Read Entry
 */
static int
proc_size_show(void *cb_data, bcmgenl_netif_t *netif)
{
    struct seq_file *m = (struct seq_file *)cb_data;
    
    seq_printf(m, "  %-14s %d\n", netif->dev->name, netif->sample_size);
    return 0;
}

static int
psample_proc_size_show(struct seq_file *m, void *v)
{
    bcmgenl_netif_search(NULL, proc_size_show, (void *)m);
    return 0;
}

static int
psample_proc_size_open(struct inode * inode, struct file * file)
{
    return single_open(file, psample_proc_size_show, NULL);
}

/*
 * psample size Proc Write Entry
 *
 *   Syntax:
 *   <netif>=<pkt sample size in bytes>
 *
 *   Where <netif> is a virtual network interface name.
 *
 *   Examples:
 *   eth4=128
 */
static int
proc_size_write(void *cb_data, bcmgenl_netif_t *netif)
{
    uint32 sample_size = (uint32)(uintptr_t)cb_data;

    netif->sample_size = sample_size;
    return 0;
}

static ssize_t
psample_proc_size_write(struct file *file, const char *buf,
                    size_t count, loff_t *loff)
{
    int netif_cnt;
    char sample_str[40], *ptr, *newline;

    if (count > sizeof(sample_str)) {
        count = sizeof(sample_str) - 1;
        sample_str[count] = '\0';
    }
    if (copy_from_user(sample_str, buf, count)) {
        return -EFAULT;
    }
    sample_str[count] = 0;
    newline = strchr(sample_str, '\n');
    if (newline) {
        /* Chop off the trailing newline */
        *newline = '\0';
    }

    if ((ptr = strchr(sample_str, '=')) == NULL &&
        (ptr = strchr(sample_str, ':')) == NULL) {
        gprintk("Error: Pkt sample size syntax not recognized: '%s'\n", sample_str);
        return count;
    }
    *ptr++ = 0;

    netif_cnt = bcmgenl_netif_search(sample_str, proc_size_write,
                                     (void *)(uintptr_t)simple_strtol(ptr, NULL, 10));
    if (netif_cnt <= 0) {
        gprintk("Warning: Failed setting psample size on "
                "unknown network interface: '%s'\n", sample_str);
    }
    
    return count;
}

struct proc_ops psample_proc_size_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =      psample_proc_size_open,
    .proc_read =      seq_read,
    .proc_lseek =     seq_lseek,
    .proc_write =     psample_proc_size_write,
    .proc_release =   single_release,
};

/*
 * psample debug Proc Read Entry
 */
static int
psample_proc_debug_show(struct seq_file *m, void *v)
{
    seq_printf(m, "BCM KNET %s Callback Config\n", PSAMPLE_GENL_NAME);
    seq_printf(m, "  debug:           0x%x\n", debug);
    seq_printf(m, "  netif_count:     %d\n",   bcmgenl_netif_num_get());
    seq_printf(m, "  queue length:    %d\n",   psample_qlen);

    return 0;
}

static int
psample_proc_debug_open(struct inode * inode, struct file * file)
{
    return single_open(file, psample_proc_debug_show, NULL);
}

/*
 * psample debug Proc Write Entry
 *
 *   Syntax:
 *   debug=<mask>
 *
 *   Where <mask> corresponds to the debug module parameter.
 *
 *   Examples:
 *   debug=0x1
 */
static ssize_t
psample_proc_debug_write(struct file *file, const char *buf,
                    size_t count, loff_t *loff)
{
    char debug_str[40];
    char *ptr;

    if (count > sizeof(debug_str)) {
        count = sizeof(debug_str) - 1;
        debug_str[count] = '\0';
    }
    if (copy_from_user(debug_str, buf, count)) {
        return -EFAULT;
    }

    if ((ptr = strstr(debug_str, "debug=")) != NULL) {
        ptr += 6;
        debug = simple_strtol(ptr, NULL, 0);
    } else {
        gprintk("Warning: unknown configuration setting\n");
    }

    return count;
}

struct proc_ops psample_proc_debug_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       psample_proc_debug_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =      psample_proc_debug_write,
    .proc_release =    single_release,
};

static int
psample_proc_stats_show(struct seq_file *m, void *v)
{
    seq_printf(m, "BCM KNET %s Callback Stats\n", PSAMPLE_GENL_NAME);
    seq_printf(m, "  pkts filter psample cb         %10lu\n", g_psample_stats.pkts_f_psample_cb);
    seq_printf(m, "  pkts sent to psample module    %10lu\n", g_psample_stats.pkts_f_psample_mod);
    seq_printf(m, "  pkts handled by psample        %10lu\n", g_psample_stats.pkts_f_handled);
    seq_printf(m, "  pkts pass through              %10lu\n", g_psample_stats.pkts_f_pass_through);
    seq_printf(m, "  pkts with mc destination       %10lu\n", g_psample_stats.pkts_f_dst_mc);
    seq_printf(m, "  pkts current queue length      %10lu\n", g_psample_stats.pkts_c_qlen_cur);
    seq_printf(m, "  pkts high queue length         %10lu\n", g_psample_stats.pkts_c_qlen_hi);
    seq_printf(m, "  pkts drop max queue length     %10lu\n", g_psample_stats.pkts_d_qlen_max);
    seq_printf(m, "  pkts drop no memory            %10lu\n", g_psample_stats.pkts_d_no_mem);
    seq_printf(m, "  pkts drop no psample group     %10lu\n", g_psample_stats.pkts_d_no_group);
    seq_printf(m, "  pkts drop sampling disabled    %10lu\n", g_psample_stats.pkts_d_sampling_disabled);
    seq_printf(m, "  pkts drop psample not ready    %10lu\n", g_psample_stats.pkts_d_not_ready);
    seq_printf(m, "  pkts drop metadata parse error %10lu\n", g_psample_stats.pkts_d_metadata);
    seq_printf(m, "  pkts with invalid src port     %10lu\n", g_psample_stats.pkts_d_meta_srcport);
    seq_printf(m, "  pkts with invalid dst port     %10lu\n", g_psample_stats.pkts_d_meta_dstport);
    seq_printf(m, "  pkts with invalid orig pkt sz  %10lu\n", g_psample_stats.pkts_d_invalid_size);
    return 0;
}

static int
psample_proc_stats_open(struct inode * inode, struct file * file)
{
    return single_open(file, psample_proc_stats_show, NULL);
}

/*
 * psample stats Proc Write Entry
 *
 *   Syntax:
 *   write any value to clear stats
 */
static ssize_t
psample_proc_stats_write(struct file *file, const char *buf,
                    size_t count, loff_t *loff)
{
    int qlen_cur = 0;
    unsigned long flags;

    spin_lock_irqsave(&g_psample_work.lock, flags);
    qlen_cur = g_psample_stats.pkts_c_qlen_cur;
    memset(&g_psample_stats, 0, sizeof(psample_stats_t));
    g_psample_stats.pkts_c_qlen_cur = qlen_cur;
    spin_unlock_irqrestore(&g_psample_work.lock, flags);

    return count;
}

struct proc_ops psample_proc_stats_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =      psample_proc_stats_open,
    .proc_read =      seq_read,
    .proc_lseek =     seq_lseek,
    .proc_write =     psample_proc_stats_write,
    .proc_release =   single_release,
};

static int
psample_proc_init(char *procfs_path)
{
    struct proc_dir_entry *entry;

    if (procfs_path == NULL || procfs_path[0] == '\0') {
        return 0;
    }

    /* Initialize proc files for psample */
    snprintf(psample_procfs_path, sizeof(psample_procfs_path) - 1,
             "%s/%s", procfs_path, PSAMPLE_GENL_NAME);
    psample_proc_root = proc_mkdir(psample_procfs_path, NULL);

    /* create procfs for psample stats */
    PROC_CREATE(entry, "stats", 0666, psample_proc_root,
                &psample_proc_stats_file_ops);
    if (entry == NULL) {
        return -1;
    }

    /* create procfs for setting sample rates */
    PROC_CREATE(entry, "rate", 0666, psample_proc_root,
                &psample_proc_rate_file_ops);
    if (entry == NULL) {
        return -1;
    }

    /* create procfs for setting sample size */
    PROC_CREATE(entry, "size", 0666, psample_proc_root,
                &psample_proc_size_file_ops);
    if (entry == NULL) {
        return -1;
    }

    /* create procfs for debug log */
    PROC_CREATE(entry, "debug", 0666, psample_proc_root,
                &psample_proc_debug_file_ops);
    if (entry == NULL) {
        return -1;
    }

    return 0;
}

static int
psample_proc_cleanup(void)
{
    if (psample_proc_root) {
        remove_proc_entry("stats", psample_proc_root);
        remove_proc_entry("rate",  psample_proc_root);
        remove_proc_entry("size",  psample_proc_root);
        remove_proc_entry("debug", psample_proc_root);
        remove_proc_entry(psample_procfs_path, NULL);
        psample_proc_root = NULL;
    }
    return 0;
}

static int
psample_cleanup(void)
{
    psample_pkt_t *pkt;
    psample_group_data_t *grp;

    cancel_work_sync(&g_psample_work.wq);

    while (!list_empty(&g_psample_work.pkt_list)) {
        pkt = list_entry(g_psample_work.pkt_list.next, psample_pkt_t, list);
        list_del(&pkt->list);
        dev_kfree_skb_any(pkt->skb);
        kfree(pkt);
    }

    while (!list_empty(&g_psample_info.group_list)) {
        grp = list_entry(g_psample_info.group_list.next,
                         psample_group_data_t, list);
        list_del(&grp->list);
        psample_group_put(grp->group);
        kfree(grp);
    }

    return 0;
}

static int
psample_init(void)
{
    /* clear data structs */
    memset(&g_psample_stats, 0, sizeof(psample_stats_t));
    memset(&g_psample_info, 0, sizeof(psample_info_t));
    memset(&g_psample_work, 0, sizeof(psample_work_t));

    /* setup psample_info struct */
    INIT_LIST_HEAD(&g_psample_info.group_list);

    /* setup psample work queue */
    spin_lock_init(&g_psample_work.lock);
    INIT_LIST_HEAD(&g_psample_work.pkt_list);
    INIT_WORK(&g_psample_work.wq, psample_task);

    /* get net namespace */
    g_psample_info.netns = get_net_ns_by_pid(current->pid);
    if (!g_psample_info.netns) {
        gprintk("%s: Could not get network namespace for pid %d\n",
                __func__, current->pid);
        return -1;
    }
    PSAMPLE_CB_DBG_PRINT("%s: current->pid %d, netns 0x%p, sample_size %d\n",
                         __func__,
                         current->pid, g_psample_info.netns, psample_size);

    return 0;
}

int bcmgenl_psample_cleanup(void)
{
    psample_cleanup();
    psample_proc_cleanup();
    bkn_filter_cb_unregister(psample_filter_cb);
    return 0;
}

int
bcmgenl_psample_init(char *procfs_path)
{
    bkn_filter_cb_register_by_name(psample_filter_cb, PSAMPLE_GENL_NAME);
    bcmgenl_netif_default_sample_set(PSAMPLE_RATE_DFLT, PSAMPLE_SIZE_DFLT);
    psample_proc_init(procfs_path);
    return psample_init();
}

#else

int
bcmgenl_psample_init(char *procfs_path)
{
    return 0;
}

int
bcmgenl_psample_cleanup(void)
{
    return 0;
}

#endif /* BCMGENL_PSAMPLE_SUPPORT */
