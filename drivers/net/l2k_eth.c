// jungwook.lim@lge.com
// 0014986: [LTE] l2k virtual ethernet driver update

/* linux/drivers/net/l2k_eth.c
 *
 * Virtual Ethernet Interface for L2000 LTE modem networking
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/if_arp.h>
#include <linux/ip.h>

#include "lte/lte_him.h"

/*-------------------------------------------------------------------

--------------------------------------------------------------------*/
#define L2K_ETH_NAME                               "l2k_eth"
#define L2K_ETH_VERSION                            "0.05"
#define L2K_ETH_NUMBER_OF_DEVICES_DEFAULT          8
#define L2K_ETH_NUMBER_OF_DEVICES_MAX              10
#define L2K_ETH_DEBUG_LEVEL                        10
/* enables IPv6 support */
#define L2K_ETH_IPV6_SUPPORT
/* allows to send DAD (duplicate address discovery) */
//#define L2K_ETH_IPV6_DAD
/* additional sanity checks level */
#define L2K_ETH_PARANOID                           2
/* sysfs support */
#define L2K_ETH_SYSFS
/* whether TX path is asynchronous (via workqueue) */
#define L2K_ETH_TX_ASYNC
/* whether RX path is asynchronous (via workqueue) */
#define L2K_ETH_RX_ASYNC
/**/
#if defined(L2K_ETH_TX_ASYNC) || defined(L2K_ETH_RX_ASYNC)
/* size for fixed work items allocation pool        */
/* if not defined - items are dynamically allocated */
#define L2K_ETH_WORK_STRUCT_POOL_SIZE              350

#ifdef  L2K_ETH_WORK_STRUCT_POOL_SIZE
/* Maximum queue size for async TX and RX */
#define L2K_ETH_QUEUE_MAX                          L2K_ETH_WORK_STRUCT_POOL_SIZE
#else
#define L2K_ETH_QUEUE_MAX                          350
#endif
#ifndef L2K_ETH_QUEUE_LOW_THR
/* low queue threshold */
#define L2K_ETH_QUEUE_LOW_THR                      (L2K_ETH_QUEUE_MAX*15/100)
#endif
#ifndef L2K_ETH_QUEUE_HIGH_THR
/* high queue threshold */
#define L2K_ETH_QUEUE_HIGH_THR                     (L2K_ETH_QUEUE_MAX*85/100)
#endif
#if (L2K_ETH_QUEUE_LOW_THR < 0) || (L2K_ETH_QUEUE_LOW_THR > L2K_ETH_QUEUE_MAX)
#error "Incorrect L2K_ETH_QUEUE_LOW_THR value"
#endif
#if (L2K_ETH_QUEUE_HIGH_THR < 0) || (L2K_ETH_QUEUE_HIGH_THR > L2K_ETH_QUEUE_MAX)
#error "Incorrect L2K_ETH_QUEUE_HIGH_THR value"
#endif
#if (L2K_ETH_QUEUE_HIGH_THR < L2K_ETH_QUEUE_LOW_THR)
#error "Incorrect L2K_ETH_QUEUE_HIGH_THR value (should not be less than L2K_ETH_QUEUE_LOW_THR)"
#endif
#endif
#define L2K_ETH_IPV4_ADDR_LEN                      4
#ifdef L2K_ETH_IPV6_SUPPORT
#include <linux/in6.h>
#include <linux/ipv6.h>
#include <linux/icmpv6.h>
#endif


#ifdef L2K_ETH_IPV6_SUPPORT
#ifndef ICMPV6_ND_SOLICITATION
#define ICMPV6_ND_SOLICITATION                     135
#endif
#ifndef ICMPV6_ND_ADVERTISEMENT
#define ICMPV6_ND_ADVERTISEMENT                    136
#endif
#endif
/*------------------------------------------------------------------*/
/* Debug macros                                                     */
/*------------------------------------------------------------------*/
#define L2K_ERR(msg)                           {printk(KERN_ERR L2K_ETH_NAME ": " msg "\n");}
#define L2K_ERR1(msg,v1)                       {printk(KERN_ERR L2K_ETH_NAME ": " msg "\n",(v1));}
#define L2K_ERR2(msg,v1,v2)                    {printk(KERN_ERR L2K_ETH_NAME ": " msg "\n",(v1),(v2));}
#define L2K_ERR3(msg,v1,v2,v3)                 {printk(KERN_ERR L2K_ETH_NAME ": " msg "\n",(v1),(v2),(v3));}
#define L2K_ERR4(msg,v1,v2,v3,v4)              {printk(KERN_ERR L2K_ETH_NAME ": " msg "\n",(v1),(v2),(v3),(v4));}
#define L2K_ERR5(msg,v1,v2,v3,v4,v5)           {printk(KERN_ERR L2K_ETH_NAME ": " msg "\n",(v1),(v2),(v3),(v4),(v5));}
#ifdef L2K_ETH_DEBUG_LEVEL
#define L2K_ISDBG(level)                       (((level) <= L2K_ETH_DEBUG_LEVEL) && ((level) <= debug_level))
#define L2K_DBG(level,msg)                     { if (L2K_ISDBG(level)) {printk(KERN_INFO L2K_ETH_NAME ": " msg "\n");}; }
#define L2K_DBG1(level,msg,v1)                 { if (L2K_ISDBG(level)) {printk(KERN_INFO L2K_ETH_NAME ": " msg "\n",(v1));}; }
#define L2K_DBG2(level,msg,v1,v2)              { if (L2K_ISDBG(level)) {printk(KERN_INFO L2K_ETH_NAME ": " msg "\n",(v1),(v2));}; }
#define L2K_DBG3(level,msg,v1,v2,v3)           { if (L2K_ISDBG(level)) {printk(KERN_INFO L2K_ETH_NAME ": " msg "\n",(v1),(v2),(v3));}; }
#define L2K_DBG4(level,msg,v1,v2,v3,v4)        { if (L2K_ISDBG(level)) {printk(KERN_INFO L2K_ETH_NAME ": " msg "\n",(v1),(v2),(v3),(v4));}; }
#define L2K_DBG5(level,msg,v1,v2,v3,v4,v5)     { if (L2K_ISDBG(level)) {printk(KERN_INFO L2K_ETH_NAME ": " msg "\n",(v1),(v2),(v3),(v4),(v5));}; }
#define L2K_DBG6(level,msg,v1,v2,v3,v4,v5,v6)  { if (L2K_ISDBG(level)) {printk(KERN_INFO L2K_ETH_NAME ": " msg "\n",(v1),(v2),(v3),(v4),(v5),(v6));}; }
#else
#define L2K_ISDBG(level)                       0
#define L2K_DBG(level,msg)
#define L2K_DBG1(level,msg,v1)
#define L2K_DBG2(level,msg,v1,v2)
#define L2K_DBG3(level,msg,v1,v2,v3)
#define L2K_DBG4(level,msg,v1,v2,v3,v4)
#define L2K_DBG5(level,msg,v1,v2,v3,v4,v5)
#define L2K_DBG6(level,msg,v1,v2,v3,v4,v5,v6)
#endif
/*------------------------------------------------------------------*/
#ifdef L2K_ETH_PARANOID
#define L2K_ETH_IS_PARANOID(level)             (level <= L2K_ETH_PARANOID)
#define L2K_ETH_PARANOID_WARN_ON(level,cond)   {if (L2K_ETH_IS_PARANOID(level)) {WARN_ON(cond);};}
#else
#define L2K_ETH_PARANOID                       0
#define L2K_ETH_IS_PARANOID(level)             0
#define L2K_ETH_PARANOID_WARN_ON(level,cond)   {}
#endif
/*------------------------------------------------------------------*/


static const char l2k_eth_driver_name[] = L2K_ETH_NAME;
static const char l2k_eth_driver_version[] = 
   L2K_ETH_VERSION
#ifdef L2K_ETH_IPV6_SUPPORT
   " IPv6"
#endif
   ;
static unsigned int num_of_devices = L2K_ETH_NUMBER_OF_DEVICES_DEFAULT;
#ifdef L2K_ETH_DEBUG_LEVEL
static unsigned int debug_level = 0;
#endif
#if defined(L2K_ETH_TX_ASYNC) || defined(L2K_ETH_RX_ASYNC)
static unsigned int queue_max = L2K_ETH_QUEUE_MAX;
static unsigned int queue_low_thr = L2K_ETH_QUEUE_LOW_THR;
static unsigned int queue_high_thr = L2K_ETH_QUEUE_HIGH_THR;
#endif
#ifdef L2K_ETH_IPV6_SUPPORT
/** l2k_icmpv6_neighbour_discovery
 *  ICMPv6 neighbour discovery message structure
 */
struct l2k_icmpv6_neighbour_discovery {
   struct icmp6hdr icmph;
   struct in6_addr target;
   __u8            opt[0];
};


struct l2k_icmpv6_neighbour_advertisement_options {
   __u8     type;
   __u8     len;
   __u8     hwaddr[6];
};


#ifdef L2K_ETH_IPV6_DAD
static unsigned int ipv6_dad = 1;
#else
static unsigned int ipv6_dad = 0;
#endif
#endif

static struct net_device* devices[L2K_ETH_NUMBER_OF_DEVICES_MAX];
static volatile int device_open_count = 0;
static spinlock_t  device_open_lock;

#if defined(L2K_ETH_TX_ASYNC) || defined(L2K_ETH_RX_ASYNC)
struct l2k_eth_work {
   struct work_struct            work;
   struct net_device*            dev;
   struct sk_buff*               skb;
#ifdef L2K_ETH_WORK_STRUCT_POOL_SIZE
   struct l2k_eth_work* next_avail;
#if L2K_ETH_PARANOID >= 7
   int    signature;
   int    used;
#endif
#endif
};
#if defined(L2K_ETH_WORK_STRUCT_POOL_SIZE) && L2K_ETH_PARANOID >= 7
#define L2K_ETH_WORK_TX_SIGNATURE            0x54585751
#define L2K_ETH_WORK_RX_SIGNATURE            0x52585157
#define L2K_ETH_WORK_SET_TX(work)            {(work)->signature = L2K_ETH_WORK_TX_SIGNATURE;}
#define L2K_ETH_WORK_SET_RX(work)            {(work)->signature = L2K_ETH_WORK_RX_SIGNATURE;}
#define L2K_ETH_WARN_IF_WORK_NOT_TX(work)    {WARN_ON((work)->signature != L2K_ETH_WORK_TX_SIGNATURE);}
#define L2K_ETH_WARN_IF_WORK_NOT_RX(work)    {WARN_ON((work)->signature != L2K_ETH_WORK_RX_SIGNATURE);}
#define L2K_ETH_SET_WORK_USED(work)          {(work)->used = 1;}
#define L2K_ETH_SET_WORK_UNUSED(work)        {(work)->used = 0;}
#define L2K_ETH_WARN_IF_WORK_USED(work)      {WARN_ON((work)->used == 1);}
#define L2K_ETH_WARN_IF_WORK_UNUSED(work)    {WARN_ON((work)->used == 0);}
#else
#define L2K_ETH_WORK_SET_TX(work)            {}
#define L2K_ETH_WORK_SET_RX(work)            {}
#define L2K_ETH_WARN_IF_WORK_NOT_TX(work)    {}
#define L2K_ETH_WARN_IF_WORK_NOT_RX(work)    {}
#define L2K_ETH_SET_WORK_USED(work)          {}
#define L2K_ETH_SET_WORK_UNUSED(work)        {}
#define L2K_ETH_WARN_IF_WORK_USED(work)      {}
#define L2K_ETH_WARN_IF_WORK_UNUSED(work)    {}
#endif
#endif

#ifdef L2K_ETH_TX_ASYNC
/* shared TX workqueue */
static struct workqueue_struct *l2k_eth_tx_wq;
static volatile unsigned int  l2k_eth_tx_work_queued = 0;
static volatile unsigned int  l2k_eth_tx_work_queued_peak = 0;
static volatile unsigned int  l2k_eth_tx_work_queued_overruns = 0;
static volatile unsigned int  l2k_eth_tx_work_queued_high = 0;
static spinlock_t    l2k_eth_tx_work_lock;
#ifdef L2K_ETH_WORK_STRUCT_POOL_SIZE
static struct l2k_eth_work  l2k_eth_tx_work_pool[L2K_ETH_WORK_STRUCT_POOL_SIZE + 1];
static volatile struct l2k_eth_work* l2k_eth_tx_work_avail;
#endif
#endif

#ifdef L2K_ETH_RX_ASYNC
/* shared RX workqueue */
static struct workqueue_struct *l2k_eth_rx_wq;
static volatile unsigned int  l2k_eth_rx_work_queued = 0;
static volatile unsigned int  l2k_eth_rx_work_queued_peak = 0;
static volatile unsigned int  l2k_eth_rx_work_queued_overruns = 0;
static volatile unsigned int  l2k_eth_rx_work_queued_high = 0;
static spinlock_t    l2k_eth_rx_work_lock;
#ifdef L2K_ETH_WORK_STRUCT_POOL_SIZE
static struct l2k_eth_work  l2k_eth_rx_work_pool[L2K_ETH_WORK_STRUCT_POOL_SIZE + 1];
static volatile struct l2k_eth_work* l2k_eth_rx_work_avail;
#endif
#endif
/**/

struct l2k_eth_private
{
   struct net_device_stats stats;
   spinlock_t lock;
   int    pdn;                                  /* TODO: PDN id */
   unsigned char l2k_eth_addr[MAX_ADDR_LEN];    /* remote MAC address - L2K modem */
#ifdef L2K_ETH_IPV6_SUPPORT
   int    ipv6_dad;                             /* allow sending IPv6 DAD */
#endif
   /*
   struct wake_lock wake_lock;
   struct sk_buff *skb;
   spinlock_t lock;
   */
};

#ifdef L2K_ETH_DEBUG_LEVEL
/** l2k_eth_dump_mac
 *  Dump ethernet MAC address
 */
static void l2k_eth_dump_mac(const char* msg, unsigned char* mac) {
   printk(KERN_INFO L2K_ETH_NAME ": " "%s %02x:%02x:%02x:%02x:%02x:%02x\n",msg,
          mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}
/** l2k_eth_dump_ipv4
 *  Dump IPv4 address
 */
static void l2k_eth_dump_ipv4(const char* msg, unsigned int ip) {
   printk(KERN_INFO L2K_ETH_NAME ": " "%s %d.%d.%d.%d\n",msg,
                                                         ip & 0xff,
                                                         (ip >>  8) & 0xff,
                                                         (ip >> 16) & 0xff,
                                                         (ip >> 24) & 0xff);
}
#ifdef L2K_ETH_IPV6_SUPPORT
/** l2k_eth_dump_ipv6
 *  Dump IPv6 address
 */
static void l2k_eth_dump_ipv6(const char* msg, struct in6_addr ip) {
   printk(KERN_INFO L2K_ETH_NAME ": " "%s %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:"
                                         "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
                          msg,
                          ip.s6_addr[0],ip.s6_addr[1],ip.s6_addr[2],ip.s6_addr[3],
                          ip.s6_addr[4],ip.s6_addr[5],ip.s6_addr[6],ip.s6_addr[7],
                          ip.s6_addr[8],ip.s6_addr[9],ip.s6_addr[10],ip.s6_addr[11],
                          ip.s6_addr[12],ip.s6_addr[13],ip.s6_addr[14],ip.s6_addr[15]);
}
#endif
/**/
#define L2K_DMAC(level,msg,mac)                { if (L2K_ISDBG(level)) {l2k_eth_dump_mac((msg),(mac));}; }
#define L2K_DIP4(level,msg,ip)                 { if (L2K_ISDBG(level)) {l2k_eth_dump_ipv4((msg),(ip));}; }
#ifdef L2K_ETH_IPV6_SUPPORT
#define L2K_DIP6(level,msg,ip)                 { if (L2K_ISDBG(level)) {l2k_eth_dump_ipv6((msg),(ip));}; }
#endif
#endif

#ifdef L2K_ETH_SYSFS
static ssize_t sysfs_pdn_id_store(struct device *d, struct device_attribute *attr, const char *buf, size_t n)
{
   struct l2k_eth_private *p = netdev_priv(to_net_dev(d));
   p->pdn = simple_strtoul(buf, NULL, 10);
   return n;
}

static ssize_t sysfs_pdn_id_show(struct device *d,
                  struct device_attribute *attr,
             char *buf)
{
   struct l2k_eth_private *p = netdev_priv(to_net_dev(d));
   return sprintf(buf, "%lu\n", (unsigned long) (p->pdn));
}

static DEVICE_ATTR(pdn, 0664, sysfs_pdn_id_show, sysfs_pdn_id_store);
#ifdef L2K_ETH_DEBUG_LEVEL
static ssize_t sysfs_debug_level_store(struct device *d, struct device_attribute *attr, const char *buf, size_t n)
{
   debug_level = simple_strtoul(buf, NULL, 10);
   return n;
}

static ssize_t sysfs_debug_level_show(struct device *d,
                  struct device_attribute *attr,
             char *buf)
{
   return sprintf(buf, "%lu\n", (unsigned long) (debug_level));
}

static DEVICE_ATTR(debug_level, 0664, sysfs_debug_level_show, sysfs_debug_level_store);
#ifdef L2K_ETH_IPV6_SUPPORT
static ssize_t sysfs_ipv6_dad_store(struct device *d, struct device_attribute *attr, const char *buf, size_t n)
{unsigned long tmp;
 struct l2k_eth_private *p = netdev_priv(to_net_dev(d));
   tmp = simple_strtoul(buf, NULL, 10);
   p->ipv6_dad = tmp ? 1 : 0;
   return n;
}

static ssize_t sysfs_ipv6_dad_show(struct device *d,
                  struct device_attribute *attr,
             char *buf)
{struct l2k_eth_private *p = netdev_priv(to_net_dev(d));
   return sprintf(buf, "%lu\n", (unsigned long) (p->ipv6_dad));
}

static DEVICE_ATTR(ipv6_dad, 0664, sysfs_ipv6_dad_show, sysfs_ipv6_dad_store);
#endif
#endif

#ifdef L2K_ETH_TX_ASYNC
static ssize_t sysfs_txqueue_show(struct device *d,
                                  struct device_attribute *attr,
                                  char *buf)
{
    return sprintf(buf, "Current:  %u\n"
                   "Peak:     %u\n"
                   "Limit:    %u\n"
                   "Low thr:  %u\n"
                   "High thr: %u\n"
                   "Overruns: %u\n",
               l2k_eth_tx_work_queued,
               l2k_eth_tx_work_queued_peak,
               queue_max,
               queue_low_thr,
               queue_high_thr,
               l2k_eth_tx_work_queued_overruns);
}
static DEVICE_ATTR(txqueue, 0444, sysfs_txqueue_show, NULL);
#endif
#ifdef L2K_ETH_RX_ASYNC
static ssize_t sysfs_rxqueue_show(struct device *d,
                                  struct device_attribute *attr,
                                  char *buf)
{
    return sprintf(buf, "Current:  %u\n"
                   "Peak:     %u\n"
                   "Limit:    %u\n"
                   "Low thr:  %u\n"
                   "High thr: %u\n"
                   "Overruns: %u\n",
               l2k_eth_rx_work_queued,
               l2k_eth_rx_work_queued_peak,
               queue_max,
               queue_low_thr,
               queue_high_thr,
               l2k_eth_rx_work_queued_overruns);
}
static DEVICE_ATTR(rxqueue, 0444, sysfs_rxqueue_show, NULL);
#endif
#endif

#ifdef L2K_ETH_TX_ASYNC
/** l2k_eth_tx_work_release
 *   Releases TX work queueu item (request processed)
 */
static void l2k_eth_tx_work_release(struct l2k_eth_work *work)
{
   spin_lock_bh(&l2k_eth_tx_work_lock);
   L2K_ETH_PARANOID_WARN_ON(4,work == NULL);
   L2K_ETH_PARANOID_WARN_ON(5,l2k_eth_tx_work_queued <= 0);
   l2k_eth_tx_work_queued--;
#ifdef L2K_ETH_WORK_STRUCT_POOL_SIZE
   L2K_ETH_WARN_IF_WORK_NOT_TX(work);
   L2K_ETH_WARN_IF_WORK_UNUSED(work);
   L2K_ETH_SET_WORK_UNUSED(work);
   work->next_avail = (struct l2k_eth_work*)l2k_eth_tx_work_avail;
   l2k_eth_tx_work_avail = work;
#else
   kfree(work);
#endif
   spin_unlock_bh(&l2k_eth_tx_work_lock);
}


/** l2k_eth_tx_work_alloc
 *  Allocates TX work queue item to put request into queue
 */
static struct l2k_eth_work* l2k_eth_tx_work_alloc(void) {
   struct l2k_eth_work* work = NULL;
   spin_lock_bh(&l2k_eth_tx_work_lock);
   if (l2k_eth_tx_work_queued >= queue_max) {
      L2K_DBG(2,"l2k_eth_tx_work_alloc(): exceeded limit of TX work queue size");
      l2k_eth_tx_work_queued_overruns++;
   } else {
#ifdef L2K_ETH_WORK_STRUCT_POOL_SIZE
      work = NULL;
      if (l2k_eth_tx_work_avail) {
         L2K_ETH_PARANOID_WARN_ON(5,l2k_eth_tx_work_queued >= L2K_ETH_WORK_STRUCT_POOL_SIZE);
         work = (struct l2k_eth_work*)l2k_eth_tx_work_avail;
         L2K_ETH_WARN_IF_WORK_NOT_TX(work);
         L2K_ETH_WARN_IF_WORK_USED(work);
         L2K_ETH_SET_WORK_USED(work);
         l2k_eth_tx_work_avail = l2k_eth_tx_work_avail->next_avail;
      } else {
         L2K_DBG(2,"l2k_eth_tx_work_alloc(): TX work items pool exhausted");
         l2k_eth_tx_work_queued_overruns++;
      }
#else
      work = (struct l2k_eth_work*)kmalloc(sizeof(struct l2k_eth_work),GFP_KERNEL);
      if (!work) {
         L2K_DBG(2,"l2k_eth_tx_work_alloc(): failed to allocate TX work item");
         l2k_eth_tx_work_queued_overruns++;
      }
#endif
      if (work) {
         l2k_eth_tx_work_queued++;
         if (l2k_eth_tx_work_queued > l2k_eth_tx_work_queued_peak) {
            l2k_eth_tx_work_queued_peak = l2k_eth_tx_work_queued;
            }
      }
   }
   spin_unlock_bh(&l2k_eth_tx_work_lock);
   return work;
}
#endif

#ifdef L2K_ETH_RX_ASYNC
/** l2k_eth_rx_work_release
 *   Releases RX work queueu item (request processed)
 */
static void l2k_eth_rx_work_release(struct l2k_eth_work *work)
{
   spin_lock_bh(&l2k_eth_rx_work_lock);
   L2K_ETH_PARANOID_WARN_ON(4,work == NULL);
   L2K_ETH_PARANOID_WARN_ON(5,l2k_eth_rx_work_queued <= 0);
   l2k_eth_rx_work_queued--;
#ifdef L2K_ETH_WORK_STRUCT_POOL_SIZE
   L2K_ETH_WARN_IF_WORK_NOT_RX(work);
   L2K_ETH_WARN_IF_WORK_UNUSED(work);
   L2K_ETH_SET_WORK_UNUSED(work);
   work->next_avail = (struct l2k_eth_work*)l2k_eth_rx_work_avail;
   l2k_eth_rx_work_avail = work;
#else
   kfree(work);
#endif
   spin_unlock_bh(&l2k_eth_rx_work_lock);
}


/** l2k_eth_rx_work_alloc
 *  Allocates RX work queue item to put request into queue
 */
static struct l2k_eth_work* l2k_eth_rx_work_alloc(void) {
   struct l2k_eth_work* work = NULL;
   spin_lock_bh(&l2k_eth_rx_work_lock);
   if (l2k_eth_rx_work_queued >= queue_max) {
      L2K_DBG(2,"l2k_eth_rx_work_alloc(): exceeded limit of RX work queue size");
      l2k_eth_rx_work_queued_overruns++;
   } else {
#ifdef L2K_ETH_WORK_STRUCT_POOL_SIZE
      work = NULL;
      if (l2k_eth_rx_work_avail) {
         L2K_ETH_PARANOID_WARN_ON(5,l2k_eth_rx_work_queued >= L2K_ETH_WORK_STRUCT_POOL_SIZE);
         work = (struct l2k_eth_work*)l2k_eth_rx_work_avail;
         L2K_ETH_WARN_IF_WORK_NOT_RX(work);
         L2K_ETH_WARN_IF_WORK_USED(work);
         L2K_ETH_SET_WORK_USED(work);
         l2k_eth_rx_work_avail = l2k_eth_rx_work_avail->next_avail;
      } else {
         L2K_DBG(2,"l2k_eth_rx_work_alloc(): RX work items pool exhausted");
         l2k_eth_rx_work_queued_overruns++;
      }
#else
      work = (struct l2k_eth_work*)kmalloc(sizeof(struct l2k_eth_work),GFP_KERNEL);
      if (!work) {
         L2K_DBG(2,"l2k_eth_rx_work_alloc(): failed to allocate RX work item");
         l2k_eth_rx_work_queued_overruns++;
      }
#endif
      if (work) {
         l2k_eth_rx_work_queued++;
         if (l2k_eth_rx_work_queued > l2k_eth_rx_work_queued_peak) {
            l2k_eth_rx_work_queued_peak = l2k_eth_rx_work_queued;
         }
      }
   }
   spin_unlock_bh(&l2k_eth_rx_work_lock);
   return work;
}
#endif


/** l2k_eth_handle_rx_packet
 *  Handles received IP packet
 *  Adds 802.3 framing and passes it to the kernel
 */
static void l2k_eth_handle_rx_packet(struct net_device* dev, struct sk_buff* skb)
{struct ethhdr *eth;
 struct iphdr* ip;
#ifdef L2K_ETH_IPV6_SUPPORT
 struct ipv6hdr* ipv6;
#endif
 struct l2k_eth_private *p;
   L2K_DBG1(4,"l2k_eth_handle_rx_packet() called, len = %d",skb->len);
   p = netdev_priv(dev);
   ip = (struct iphdr*)(skb->data);
   if (ip->version == 4) {
      /**/
      if (dev_hard_header(skb, dev, ETH_P_IP, dev->dev_addr, p->l2k_eth_addr, skb->len) < 0) {
         L2K_DBG(2,"l2k_eth_handle_rx_packet() failed to create ethernet header");
         /**/
         spin_lock_bh(&(p->lock));
         p->stats.rx_dropped++;
         spin_unlock_bh(&(p->lock));
         /**/
         dev_kfree_skb_irq(skb);
         L2K_DBG(4,"l2k_eth_handle_rx_packet() returns");
         return;
      }
      skb->protocol = eth_type_trans(skb,dev);
      eth = eth_hdr(skb);
      ip = ip_hdr(skb);
      /**/
      L2K_DBG1(5,"l2k_eth_handle_rx_packet(): eth protocol = 0x%x",ntohs(skb->protocol));
      L2K_DBG1(5,"l2k_eth_handle_rx_packet(): length       = %d",skb->len);
      L2K_DMAC(6,"l2k_eth_handle_rx_packet(): source hw=",eth->h_source);
      L2K_DMAC(6,"l2k_eth_handle_rx_packet(): target hw=",eth->h_dest);
      /**/
      L2K_DBG1(5,"l2k_eth_handle_rx_packet(): version   = %d",ip->version);
      L2K_DBG1(5,"l2k_eth_handle_rx_packet(): length    = %d",ntohs(ip->tot_len));
      L2K_DBG1(5,"l2k_eth_handle_rx_packet(): protocol  = %d",ip->protocol);
      L2K_DIP4(5,"l2k_eth_handle_rx_packet(): source IP =",ip->saddr);
      L2K_DIP4(5,"l2k_eth_handle_rx_packet(): target IP =",ip->daddr);
      /**/
      if (likely(netif_rx(skb) != NET_RX_SUCCESS)) {
         spin_lock_bh(&(p->lock));
         p->stats.rx_dropped++;
         spin_unlock_bh(&(p->lock));
     }
   } else if (ip->version == 6) {
#ifdef L2K_ETH_IPV6_SUPPORT
      ipv6 = (struct ipv6hdr*)(skb->data);
      /**/
      if (dev_hard_header(skb, dev, ETH_P_IPV6, dev->dev_addr, p->l2k_eth_addr, skb->len) < 0) {
         L2K_DBG(2,"l2k_eth_handle_rx_packet() failed to create ethernet header");
         /**/
         spin_lock_bh(&(p->lock));
         p->stats.rx_dropped++;
         spin_unlock_bh(&(p->lock));
         /**/
         dev_kfree_skb_irq(skb);
         L2K_DBG(4,"l2k_eth_handle_rx_packet() returns");
         return;
      }
      skb->protocol = eth_type_trans(skb,dev);
      eth = eth_hdr(skb);
      ipv6 = ipv6_hdr(skb);
      /**/
      L2K_DBG1(5,"l2k_eth_handle_rx_packet(): eth protocol = 0x%x",ntohs(skb->protocol));
      L2K_DBG1(5,"l2k_eth_handle_rx_packet(): length       = %d",skb->len);
      L2K_DMAC(6,"l2k_eth_handle_rx_packet(): source hw=",eth->h_source);
      L2K_DMAC(6,"l2k_eth_handle_rx_packet(): target hw=",eth->h_dest);
      /**/
      L2K_DBG1(5,"l2k_eth_handle_rx_packet(): version          = %d",ipv6->version);
      L2K_DBG1(5,"l2k_eth_handle_rx_packet(): payloadlength    = %d",ntohs(ipv6->payload_len));
      L2K_DBG1(5,"l2k_eth_handle_rx_packet(): nexthdr          = %d",ipv6->nexthdr);
      L2K_DIP6(5,"l2k_eth_handle_rx_packet(): source IP        =",ipv6->saddr);
      L2K_DIP6(5,"l2k_eth_handle_rx_packet(): target IP        =",ipv6->daddr);
      /**/
      if (likely(netif_rx(skb) != NET_RX_SUCCESS)) {
         spin_lock_bh(&(p->lock));
         p->stats.rx_dropped++;
         spin_unlock_bh(&(p->lock));
     }
      /**/
#else
      L2K_DBG(2,"l2k_eth_handle_rx_packet()(): IPV6 is not supported");
      spin_lock_bh(&(p->lock));
      p->stats.rx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      dev_kfree_skb_irq(skb);
#endif
   } else {
      L2K_DBG(2,"l2k_eth_handle_rx_packet()(): unknown IP version");
      spin_lock_bh(&(p->lock));
      p->stats.rx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      dev_kfree_skb_irq(skb);
   }
   L2K_DBG(4,"l2k_eth_handle_rx_packet() returns");
}

#if defined(L2K_ETH_RX_ASYNC)
/** l2k_eth_rx_task
 *  Asynchronous RX task handler
 */
static void l2k_eth_rx_task(struct work_struct *pwork)
{
 struct l2k_eth_work *work;
 struct sk_buff* skb;
 struct net_device *dev;

   L2K_DBG(5,"l2k_eth_rx_task() called");
   L2K_ETH_PARANOID_WARN_ON(4,pwork == NULL);
   work = container_of(pwork, struct l2k_eth_work, work);
   skb = work->skb;
   dev = work->dev;
   L2K_ETH_PARANOID_WARN_ON(5,skb == NULL);
   L2K_ETH_PARANOID_WARN_ON(5,dev == NULL);
   L2K_ETH_PARANOID_WARN_ON(6,atomic_read(&skb->users) <= 0);
   /**/
   l2k_eth_rx_work_release(work);
   /* handle packet */
   l2k_eth_handle_rx_packet(dev,skb);
   /**/
   spin_lock_bh(&l2k_eth_rx_work_lock);
   if (l2k_eth_rx_work_queued < queue_low_thr &&
       l2k_eth_rx_work_queued_high) {
      L2K_DBG(2,"l2k_eth_rx_task(): RX queue below low threshold");
      l2k_eth_rx_work_queued_high = 0;
      /* TODO: flow control ? */
   }        
   spin_unlock_bh(&l2k_eth_rx_work_lock);
   /* finished */
   L2K_DBG(5,"l2k_eth_rx_task() returns");
}
#endif

/** l2k_eth_lte_him_callback
 *  IP packet receive entry from LTE HIM driver
 */
static int l2k_eth_lte_him_callback(void *packet, unsigned int size, int pdn) {
 int i;
 struct net_device* dev;
 struct l2k_eth_private *p;
 struct sk_buff *skb;
#if defined(L2K_ETH_RX_ASYNC)
 struct l2k_eth_work* work;
#endif
   L2K_DBG1(3,"l2k_eth_lte_him_callback() called, pdn == %d",pdn);
   /**/
   if (packet == NULL || size == 0) {
      L2K_ERR("l2k_eth_lte_him_callback(): called with NULL packet");
      L2K_DBG(3,"l2k_eth_lte_him_callback() returns");
      return 0;
   }
   /* find VED by pdn */
   dev = NULL;
   for(i = 0; dev == NULL && i < num_of_devices; i++) {
      if (devices[i] != NULL) {
         p = netdev_priv(devices[i]);
         if (p != NULL && p->pdn == pdn) {
            dev = devices[i];
            break;
         }
      }
   }
   /**/
   if (dev) {
      /**/
      p = netdev_priv(dev);
      spin_lock_bh(&(p->lock));
      p->stats.rx_packets++;
      p->stats.rx_bytes += size + sizeof(struct ethhdr);
      spin_unlock_bh(&(p->lock));
      /**/
      /* allocate skb */
      skb = alloc_skb(size + LL_ALLOCATED_SPACE(dev), GFP_ATOMIC);
      if (skb == NULL)
      {
         L2K_DBG(2,"l2k_eth_lte_him_callback(): failed to allocate skb");
         /**/
         spin_lock_bh(&(p->lock));
         p->stats.rx_dropped++;
         spin_unlock_bh(&(p->lock));
         /**/
         L2K_DBG(3,"l2k_eth_lte_him_callback() returns");
         return 1;
      }
      /**/
      /* fill ethernet header */
      skb_reserve(skb, LL_RESERVED_SPACE(dev));
      skb_reset_network_header(skb);
      skb_put(skb, size);
      memcpy(skb->data,packet,size);
      /**/
#if defined(L2K_ETH_RX_ASYNC)
      work = l2k_eth_rx_work_alloc();
      if (work) {
         work->dev = dev;
         work->skb = skb;
         INIT_WORK(&(work->work), l2k_eth_rx_task);
         queue_work(l2k_eth_rx_wq, &(work->work));
      } else {
         L2K_DBG(2,"l2k_eth_lte_him_callback(): failed to allocate RX work");
         dev_kfree_skb_irq(skb);
         /**/
         spin_lock_bh(&(p->lock));
         p->stats.rx_dropped++;
         spin_unlock_bh(&(p->lock));
         /**/
      }
      if (l2k_eth_rx_work_queued > queue_high_thr &&
         !l2k_eth_rx_work_queued_high) {
         L2K_DBG(2,"l2k_eth_lte_him_callback(): RX queue above high threshold");
         l2k_eth_rx_work_queued_high = 1;
         /* TODO: flow control ? */
      }
#else
      l2k_eth_handle_rx_packet(dev,skb);
#endif
   } else {
      L2K_DBG1(2,"l2k_eth_lte_him_callback(): pdn %d is not known to driver",pdn);
   }
   L2K_DBG(3,"l2k_eth_lte_him_callback() returns");
   return 1;
}



/** l2k_eth_handle_tx_ip
 *  Transmits IP packet to LTE HIM driver
 */
static void l2k_eth_handle_tx_ip(struct net_device* dev, struct sk_buff* skb)
{struct l2k_eth_private *p = netdev_priv(dev);
 struct iphdr* ip;
   L2K_DBG(3,"l2k_eth_handle_tx_ip() called");
   ip = ip_hdr(skb);
   L2K_ETH_PARANOID_WARN_ON(4,ip == NULL);
   /**/
   L2K_DBG1(5,"l2k_eth_handle_tx_ip(): version   = %d",ip->version);
   L2K_DBG1(5,"l2k_eth_handle_tx_ip(): length    = %d",ntohs(ip->tot_len));
   L2K_DBG1(5,"l2k_eth_handle_tx_ip(): protocol  = %d",ip->protocol);
   L2K_DIP4(5,"l2k_eth_handle_tx_ip(): source IP =",ip->saddr);
   L2K_DIP4(5,"l2k_eth_handle_tx_ip(): target IP =",ip->daddr);
   L2K_DBG1(5,"l2k_eth_handle_tx_ip(): him  = %d",p->pdn);
   /**/
   if (lte_him_enqueue_ip_packet(ip, ntohs(ip->tot_len), p->pdn) != 0) {
      L2K_DBG(2,"l2k_eth_handle_tx_ip(): failed to enqueue packet to HIM");
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
   }
   dev_kfree_skb_irq(skb);
   /**/
   L2K_DBG(3,"l2k_eth_handle_tx_ip() returns");
}

/** l2k_eth_handle_arp
 *  Handles ARP request from kernel
 *  responds with 'modem' MAC address (fake)
 */
static void l2k_eth_handle_arp(struct net_device* dev, struct sk_buff* skb_in)
{struct l2k_eth_private *p = netdev_priv(dev);
 struct sk_buff *skb;
 struct arphdr *arph;
 unsigned char *arp_ptr, *arp_ptr_in, *sha_ptr_in, *spa_ptr_in, *tpa_ptr_in;
   L2K_DBG(3,"l2k_eth_handle_arp() called");
#if L2K_ETH_PARANOID > 0
   if (pskb_may_pull(skb_in, arp_hdr_len(dev))) {
      arph = arp_hdr(skb_in);
      if (arph == NULL ||
          arph->ar_op != htons(ARPOP_REQUEST) ||
          arph->ar_hln != dev->addr_len ||
          skb_in->pkt_type == PACKET_LOOPBACK ||
          arph->ar_pln != L2K_ETH_IPV4_ADDR_LEN) {
         L2K_DBG(2,"l2k_eth_handle_arp(): ARP request is not valid");
         /**/
         spin_lock_bh(&(p->lock));
         p->stats.tx_dropped++;
         spin_unlock_bh(&(p->lock));
         /**/
         dev_kfree_skb_irq(skb_in);
         L2K_DBG(3,"l2k_eth_handle_arp() returns");
         /**/
         return;
      }
   } else {
      L2K_DBG(2,"l2k_eth_handle_arp(): ARP request is not valid");
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      dev_kfree_skb_irq(skb_in);
      L2K_DBG(3,"l2k_eth_handle_arp() returns");
      return;
   }
#endif
   /* Get pointer to ARP packet from received sb_buff */
   arph = arp_hdr(skb_in);
   L2K_ETH_PARANOID_WARN_ON(3,arph == NULL);
   arp_ptr_in = (unsigned char*)arph + sizeof(struct arphdr); // store pointer to SHA
   sha_ptr_in = arp_ptr_in;
   L2K_DMAC(5,"l2k_eth_handle_arp(): ARP REQ source hw =",sha_ptr_in);
   arp_ptr_in += dev->addr_len;
   spa_ptr_in = arp_ptr_in; // store SPA
   L2K_DBG4(5,"l2k_eth_handle_arp(): ARP REQ source IP=%d.%d.%d.%d",spa_ptr_in[0],spa_ptr_in[1],spa_ptr_in[2],spa_ptr_in[3]);
   arp_ptr_in += L2K_ETH_IPV4_ADDR_LEN;
   arp_ptr_in += dev->addr_len;
   tpa_ptr_in = arp_ptr_in; // store TPA
   L2K_DBG4(5,"l2k_eth_handle_arp(): ARP REQ target IP=%d.%d.%d.%d",tpa_ptr_in[0],tpa_ptr_in[1],tpa_ptr_in[2],tpa_ptr_in[3]);

   /* allocate buffer */
   skb = alloc_skb(arp_hdr_len(dev) + LL_ALLOCATED_SPACE(dev), GFP_ATOMIC);
   if (skb == NULL)
   {
      L2K_DBG(2,"l2k_eth_handle_arp: failed to allocate ARP response skb");
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      dev_kfree_skb_irq(skb_in);
      L2K_DBG(3,"l2k_eth_handle_arp() returns");
      return;
   }
   /**/
   /* fill ethernet header */
   skb_reserve(skb, LL_RESERVED_SPACE(dev));
   skb_reset_network_header(skb);
   arph = (struct arphdr *) skb_put(skb, arp_hdr_len(dev));
   skb->dev = dev;
   /**/
   if (dev_hard_header(skb, dev, ETH_P_ARP, dev->dev_addr, p->l2k_eth_addr, skb->len) < 0) {
      L2K_DBG(2,"l2k_eth_handle_arp() failed to create ethernet header");
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      dev_kfree_skb_irq(skb);
      dev_kfree_skb_irq(skb_in);
      L2K_DBG(3,"l2k_eth_handle_arp() returns");
      return;
   }
   /**/
   skb->protocol = eth_type_trans(skb,dev);
   /* Fill up ARP header */
   arph->ar_hrd = htons(dev->type);
   arph->ar_pro = htons(ETH_P_IP);
   /**/
   arph->ar_hln = dev->addr_len;
   arph->ar_pln = L2K_ETH_IPV4_ADDR_LEN;
   arph->ar_op = htons(ARPOP_REPLY);
   /**/
   /* Fill up addresses */
   arp_ptr = (unsigned char*)arph + sizeof(struct arphdr); // get pointer to the next byte following ARP header (SHA)
   memcpy(arp_ptr, p->l2k_eth_addr, dev->addr_len);      // store 'modem hw' address to sender field
   L2K_DMAC(5,"l2k_eth_handle_arp(): ARP RESP source hw =",arp_ptr);
   arp_ptr += dev->addr_len;                             // move to SPA field
   memcpy(arp_ptr, tpa_ptr_in, L2K_ETH_IPV4_ADDR_LEN);   // copy TPA from request, it should be SPA now
   L2K_DBG4(5,"l2k_eth_handle_arp(): ARP RESP source IP=%d.%d.%d.%d",arp_ptr[0],arp_ptr[1],arp_ptr[2],arp_ptr[3]);
   /**/
   arp_ptr += L2K_ETH_IPV4_ADDR_LEN;                     // move to THA field
   memcpy(arp_ptr, sha_ptr_in, dev->addr_len);           // copy SHA from request, it should be THA now
   L2K_DMAC(5,"l2k_eth_handle_arp(): ARP RESP target hw =",arp_ptr);
   arp_ptr += dev->addr_len;                             // move to TPA field
   memcpy(arp_ptr, spa_ptr_in, L2K_ETH_IPV4_ADDR_LEN);   // copy SPA from request, it should be TPA now
   L2K_DBG4(5,"l2k_eth_handle_arp(): ARP RESP target IP=%d.%d.%d.%d",arp_ptr[0],arp_ptr[1],arp_ptr[2],arp_ptr[3]);
   /**/
   /* send reply */
    if (likely(netif_rx(skb) == NET_RX_SUCCESS)) {
        spin_lock_bh(&(p->lock));
        p->stats.rx_bytes += skb->len;
        p->stats.rx_packets++;
        spin_unlock_bh(&(p->lock));
    } else {
        spin_lock_bh(&(p->lock));
        p->stats.rx_dropped++;
        spin_unlock_bh(&(p->lock));
    }
   dev_kfree_skb_irq(skb_in);
   L2K_DBG(3,"l2k_eth_handle_arp() returns");
}

#ifdef L2K_ETH_IPV6_SUPPORT
/** l2k_eth_handle_tx_ipv6_down
 *  Handles sending IPv6 packet to LTE
 */
static void l2k_eth_handle_tx_ipv6_down(struct net_device* dev, struct sk_buff* skb)
{struct l2k_eth_private *p = netdev_priv(dev);
 struct ipv6hdr* ipv6;
   L2K_DBG(3,"l2k_eth_handle_tx_ipv6_down() called");
   ipv6 = ipv6_hdr(skb);
   L2K_ETH_PARANOID_WARN_ON(3,ipv6 == NULL);
   /**/
   L2K_DBG1(5,"l2k_eth_handle_tx_ipv6_down(): version           = %d",ipv6->version);
   L2K_DBG1(5,"l2k_eth_handle_tx_ipv6_down(): payload length    = %d",ntohs(ipv6->payload_len));
   L2K_DBG1(5,"l2k_eth_handle_tx_ipv6_down(): nexthdr           = %d",ipv6->nexthdr);
   L2K_DIP6(5,"l2k_eth_handle_tx_ipv6_down(): source IP         =",ipv6->saddr);
   L2K_DIP6(5,"l2k_eth_handle_tx_ipv6_down(): target IP         =",ipv6->daddr);
   L2K_DBG1(5,"l2k_eth_handle_tx_ipv6_down(): him               = %d",p->pdn);
   /**/
   if (lte_him_enqueue_ip_packet(ipv6, skb->len, p->pdn) != 0) {
      L2K_DBG(2,"l2k_eth_handle_tx_ip(): failed to enqueue packet to HIM");
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
   }
   dev_kfree_skb_irq(skb);
   /**/
   L2K_DBG(3,"l2k_eth_handle_tx_ipv6_down() returns");
}


/** l2k_eth_is_ipv6_null
 *  Checks if IPv6 address is 0 (unassigned)
 */
static int l2k_eth_is_ipv6_null(struct in6_addr a) {
   return (a.s6_addr32[0] == 0) && (a.s6_addr32[1] == 0) &&
          (a.s6_addr32[2] == 0) && (a.s6_addr32[3] == 0);
}


/** l2k_eth_handle_tx_ipv6_neighbour_solicitation
 *  Handles IPv6 neighbour solicitation from kernel
 *  responds with 'modem' MAC address (fake)
 */
static void l2k_eth_handle_tx_ipv6_neighbour_solicitation(struct net_device* dev, struct sk_buff* skb)
{struct l2k_eth_private *p = netdev_priv(dev);
 struct ipv6hdr* ipv6;
 struct icmp6hdr* icmp6;
 struct l2k_icmpv6_neighbour_discovery* nd;
 struct sk_buff* skb_out;
 struct ipv6hdr* ipv6_out;
 struct l2k_icmpv6_neighbour_discovery* nd_out;
 struct l2k_icmpv6_neighbour_advertisement_options* nd_opts;

   ipv6 = ipv6_hdr(skb);
   icmp6 = icmp6_hdr(skb);
   L2K_ETH_PARANOID_WARN_ON(3,ipv6 == NULL || icmp6 == NULL);
   nd = (struct l2k_icmpv6_neighbour_discovery*)icmp6;
   /* check if duplicate address discovery */
   if (l2k_eth_is_ipv6_null(ipv6->saddr)) {
      L2K_DBG(5,"l2k_eth_handle_tx_ipv6(): ICMPv6 Neighbour Solicitation/Duplicate Address discovery");
      L2K_DIP6(5,"l2k_eth_handle_tx_ipv6(): DAD for IPv6:",nd->target);
      if (p->ipv6_dad) {
         L2K_DBG(5,"l2k_eth_handle_tx_ipv6():  sending DAD packet");
         l2k_eth_handle_tx_ipv6_down(dev,skb);
      } else {
         L2K_DBG(5,"l2k_eth_handle_tx_ipv6():  dropping DAD packet");
         spin_lock_bh(&(p->lock));
         p->stats.tx_dropped++;
         spin_unlock_bh(&(p->lock));
         dev_kfree_skb_irq(skb);
      }
   } else {
      L2K_DBG(5,"l2k_eth_handle_tx_ipv6(): ICMPv6 Neighbour Solicitation/Address resolution");
      L2K_DIP6(5,"l2k_eth_handle_tx_ipv6(): REQ for IPv6:",nd->target);
      /* allocate response buffer */
      skb_out = alloc_skb(sizeof(struct ipv6hdr) +
                          sizeof(struct l2k_icmpv6_neighbour_discovery) +
                          sizeof(struct l2k_icmpv6_neighbour_advertisement_options) +
                          LL_ALLOCATED_SPACE(dev), GFP_ATOMIC);
      if (skb_out == NULL)
      {
         L2K_DBG(2,"l2k_eth_handle_tx_ipv6_neighbour_solicitation(): failed to allocate neighbour advertisement skb");
         /**/
         spin_lock_bh(&(p->lock));
         p->stats.tx_dropped++;
         spin_unlock_bh(&(p->lock));
         /**/
         dev_kfree_skb_irq(skb);
         L2K_DBG(3,"l2k_eth_handle_tx_ipv6_neighbour_solicitation() returns");
         return;
      }
      /**/
      /* build response skb */
      skb_reserve(skb_out, LL_RESERVED_SPACE(dev));
      skb_reset_network_header(skb_out);
      /**/
      ipv6_out = (struct ipv6hdr *)skb_put(skb_out, sizeof(struct ipv6hdr));
      memset(ipv6_out,0,sizeof(struct ipv6hdr));
      nd_out = (struct l2k_icmpv6_neighbour_discovery *)skb_put(skb_out, sizeof(struct l2k_icmpv6_neighbour_discovery) +
                                                                sizeof(struct l2k_icmpv6_neighbour_advertisement_options));
      memset(nd_out,0,sizeof(struct l2k_icmpv6_neighbour_discovery) + sizeof(struct l2k_icmpv6_neighbour_advertisement_options));
      /**/
      skb_out->dev = dev;
      /**/
      if (dev_hard_header(skb_out, dev, ETH_P_IPV6, dev->dev_addr, p->l2k_eth_addr, skb_out->len) < 0) {
         L2K_DBG(2,"l2k_eth_handle_arp() failed to create ethernet header");
         /**/
         spin_lock_bh(&(p->lock));
         p->stats.tx_dropped++;
         spin_unlock_bh(&(p->lock));
         /**/
         dev_kfree_skb_irq(skb);
         dev_kfree_skb_irq(skb_out);
         L2K_DBG(3,"l2k_eth_handle_tx_ipv6_neighbour_solicitation() returns");
         return;
      }
      /**/
      skb_out->protocol = eth_type_trans(skb_out,dev);
      /**/
      /**/
      ipv6_out->version = 6;
      ipv6_out->payload_len = htons(sizeof(struct l2k_icmpv6_neighbour_discovery) + sizeof(struct l2k_icmpv6_neighbour_advertisement_options));
      ipv6_out->nexthdr = IPPROTO_ICMPV6;
      ipv6_out->hop_limit = 255;
      memcpy(&(ipv6_out->saddr),&(nd->target),sizeof(struct in6_addr));
      memcpy(&(ipv6_out->daddr),&(ipv6->saddr),sizeof(struct in6_addr));
      /**/
      nd_out->icmph.icmp6_type = ICMPV6_ND_ADVERTISEMENT;
      nd_out->icmph.icmp6_code = 0;
      nd_out->icmph.icmp6_cksum = 0;
      nd_out->icmph.icmp6_dataun.u_nd_advt.router = 1;
      nd_out->icmph.icmp6_dataun.u_nd_advt.solicited = 1;
      nd_out->icmph.icmp6_dataun.u_nd_advt.override = 1;
      memcpy(&(nd_out->target),&(nd->target),sizeof(struct in6_addr));
      /* put options */
      nd_opts = (struct l2k_icmpv6_neighbour_advertisement_options*)&(nd_out->opt);
      nd_opts->type = 2;
      nd_opts->len = 1;
      memcpy(nd_opts->hwaddr,p->l2k_eth_addr,6);
      /* calculate icmpv6 checksum */
      {unsigned int csum;
         csum = csum_partial(nd_out,
                             sizeof(struct l2k_icmpv6_neighbour_discovery) +
                             sizeof(struct l2k_icmpv6_neighbour_advertisement_options), 0);
         nd_out->icmph.icmp6_cksum = csum_ipv6_magic(&(ipv6_out->saddr),
                                                     &(ipv6_out->daddr),
                                                     ntohs(ipv6_out->payload_len),
                                                     ipv6_out->nexthdr,
                                                     csum);
      }
      /**/
      L2K_DBG(5,"l2k_eth_handle_tx_ipv6(): 'Fake' ICMPv6 Neighbour Advertisement/Address resolution");
      L2K_DIP6(5,"l2k_eth_handle_tx_ipv6(): RESP for IPv6:     ",nd->target);
      L2K_DMAC(6,"l2k_eth_handle_tx_ipv6(): source hw         =",dev->dev_addr);
      L2K_DMAC(6,"l2k_eth_handle_tx_ipv6(): target hw         =",p->l2k_eth_addr);
      L2K_DBG1(5,"l2k_eth_handle_tx_ipv6(): version           = %d",ipv6_out->version);
      L2K_DBG1(5,"l2k_eth_handle_tx_ipv6(): payload length    = %d",ntohs(ipv6_out->payload_len));
      L2K_DBG1(5,"l2k_eth_handle_tx_ipv6(): nexthdr           = %d",ipv6_out->nexthdr);
      L2K_DIP6(5,"l2k_eth_handle_tx_ipv6(): source IP         =",ipv6_out->saddr);
      L2K_DIP6(5,"l2k_eth_handle_tx_ipv6(): target IP         =",ipv6_out->daddr);
      /**/
        if (likely(netif_rx(skb_out) == NET_RX_SUCCESS)) {
          /**/
          spin_lock_bh(&(p->lock));
          p->stats.rx_packets++;
          p->stats.rx_bytes += skb->len;
          spin_unlock_bh(&(p->lock));
        } else {
          spin_lock_bh(&(p->lock));
          p->stats.rx_dropped++;
          spin_unlock_bh(&(p->lock));
        }
      dev_kfree_skb_irq(skb);
   }
   L2K_DBG(3,"l2k_eth_handle_tx_ipv6_neighbour_solicitation() returns");
}

/** l2k_eth_handle_tx_ipv6
 *  Hanles sending IPv6 packet from kernel
 */
static void l2k_eth_handle_tx_ipv6(struct net_device* dev, struct sk_buff* skb)
{struct ipv6hdr* ipv6;
 struct icmp6hdr* icmp6;
   L2K_DBG(3,"l2k_eth_handle_tx_ipv6() called");
   ipv6 = ipv6_hdr(skb);
   L2K_ETH_PARANOID_WARN_ON(3,ipv6 == NULL);
   /**/
   L2K_DBG1(5,"l2k_eth_handle_tx_ipv6(): version           = %d",ipv6->version);
   L2K_DBG1(5,"l2k_eth_handle_tx_ipv6(): payload length    = %d",ntohs(ipv6->payload_len));
   L2K_DBG1(5,"l2k_eth_handle_tx_ipv6(): nexthdr           = %d",ipv6->nexthdr);
   L2K_DIP6(5,"l2k_eth_handle_tx_ipv6(): source IP         =",ipv6->saddr);
   L2K_DIP6(5,"l2k_eth_handle_tx_ipv6(): target IP         =",ipv6->daddr);

   if (ipv6->nexthdr == IPPROTO_ICMPV6) {
      icmp6 = icmp6_hdr(skb);
      L2K_ETH_PARANOID_WARN_ON(3,icmp6 == NULL);
      switch(icmp6->icmp6_type) {
      case ICMPV6_ND_SOLICITATION:
         L2K_DBG(5,"l2k_eth_handle_tx_ipv6(): ICMPv6 Neighbour solicitation");
         l2k_eth_handle_tx_ipv6_neighbour_solicitation(dev,skb);
         break;
      default:
         L2K_DBG1(5,"l2k_eth_handle_tx_ipv6(): ICMPv6 type %d, sending down",icmp6->icmp6_type);
         l2k_eth_handle_tx_ipv6_down(dev,skb);
         break;
      }
   } else {
      L2K_DBG1(5,"l2k_eth_handle_tx_ipv6(): nexthdr=%d, sending down",ipv6->nexthdr);
      l2k_eth_handle_tx_ipv6_down(dev,skb);
   }
   L2K_DBG(3,"l2k_eth_handle_tx_ipv6() returns");
}
#endif

/** l2k_eth_handle_tx_packet
 *  Handles outgoing 802.3 packet from kernel
 */
static void l2k_eth_handle_tx_packet(struct net_device* dev, struct sk_buff* skb)
{struct ethhdr *eth;
 struct l2k_eth_private *p;
   L2K_DBG(4,"l2k_eth_handle_tx_packet() called");
   p = netdev_priv(dev);
   L2K_ETH_PARANOID_WARN_ON(2,p==NULL);
#if L2K_ETH_PARANOID > 0
   if (skb == NULL) {
      L2K_DBG(2,"l2k_eth_handle_tx_packet(): is called with skb == NULL");
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      return;
   }
   if (skb->data == NULL) {
      L2K_DBG(2,"l2k_eth_handle_tx_packet(): is called with skb->data == NULL");
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      dev_kfree_skb_irq(skb);
      return;
   }
#endif
   skb->protocol = eth_type_trans(skb,dev);
   eth = eth_hdr(skb);
#if L2K_ETH_PARANOID > 0
   if (eth == NULL) {
      L2K_DBG(2,"l2k_eth_handle_tx_packet(): could not get skb eth header");
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      dev_kfree_skb_irq(skb);
      return;
   }
#endif
   L2K_DMAC(6,"l2k_eth_handle_tx_packet(): source hw =",eth->h_source);
   L2K_DMAC(6,"l2k_eth_handle_tx_packet(): target hw =",eth->h_dest);
#if L2K_ETH_PARANOID > 0
   if(memcmp(eth->h_source,dev->dev_addr, ETH_ALEN) != 0) {
      L2K_DBG(2,"l2k_eth_handle_tx_packet(): packet source address is different from interface address ");
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      dev_kfree_skb_irq(skb);
      return;
   }
   if(((*eth->h_dest&1) == 0) &&
      (memcmp(eth->h_dest,p->l2k_eth_addr,ETH_ALEN) != 0)) {
      L2K_DBG(2,"l2k_eth_handle_tx_packet(): packet destination address is unicast and different from modem address");
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      dev_kfree_skb_irq(skb);
      return;
   }
#endif
   L2K_DBG1(5,"l2k_eth_handle_tx_packet(): eth protocol = 0x%x",ntohs(skb->protocol));
   L2K_DBG1(5,"l2k_eth_handle_tx_packet(): length       = %d",skb->len);
   switch(ntohs(skb->protocol)) {
   case ETH_P_ARP:
      l2k_eth_handle_arp(dev,skb);
      break;
   case ETH_P_IP:
      l2k_eth_handle_tx_ip(dev,skb);
      break;
   case ETH_P_IPV6:
#ifdef L2K_ETH_IPV6_SUPPORT
      l2k_eth_handle_tx_ipv6(dev,skb);
#else
      L2K_DBG(2,"l2k_eth_handle_tx_packet(): IPV6 is not supported");
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      dev_kfree_skb_irq(skb);
#endif
      break;
   default:
      L2K_DBG1(2,"l2k_eth_handle_tx_packet(): unknown protocol = 0x%x",ntohs(skb->protocol));
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_dropped++;
      spin_unlock_bh(&(p->lock));
      /**/
      dev_kfree_skb_irq(skb);
      break;
   }
   L2K_DBG(4,"l2k_eth_handle_tx_packet() returns");
}


#if defined(L2K_ETH_TX_ASYNC)
/** l2k_eth_tx_task
 *  Asynchronous TX queue handler
 */
static void l2k_eth_tx_task(struct work_struct *pwork)
{struct l2k_eth_work *work;
 struct sk_buff* skb;
 struct net_device *dev;
   L2K_DBG(5,"l2k_eth_tx_task() called");
   L2K_ETH_PARANOID_WARN_ON(4,pwork == NULL);
   work = container_of(pwork, struct l2k_eth_work, work);
   skb = work->skb;
   dev = work->dev;
   L2K_ETH_PARANOID_WARN_ON(5,skb == NULL);
   L2K_ETH_PARANOID_WARN_ON(5,dev == NULL);
   L2K_ETH_PARANOID_WARN_ON(6,atomic_read(&skb->users) <= 0);
   /**/
   l2k_eth_tx_work_release(work);
   /* handle packet */
   l2k_eth_handle_tx_packet(dev,skb);
   /* finished */
   spin_lock_bh(&l2k_eth_tx_work_lock);
   if (l2k_eth_tx_work_queued_high &&
       l2k_eth_tx_work_queued < queue_low_thr) {
      L2K_DBG(2,"l2k_eth_tx_task(): TX queue below low threshold, resuming net queue");
      netif_wake_queue(dev);
      l2k_eth_tx_work_queued_high = 0;
   }
   spin_unlock_bh(&l2k_eth_tx_work_lock);
   L2K_DBG(5,"l2k_eth_tx_task() returns");
}
#endif

/** l2k_eth_open
 *  opens network device. Usually it is called as a result of ifconfig up
 */
static int l2k_eth_open(struct net_device *dev)
{int err = 0;
 struct l2k_eth_private *p = netdev_priv(dev);
   L2K_DBG(5,"l2k_eth_open() called");
   L2K_ETH_PARANOID_WARN_ON(2,p == NULL);
   spin_lock_bh(&device_open_lock);
   if (device_open_count == 0) {
      L2K_DBG(4,"registering HIM callback");
      err = lte_him_register_cb_from_ved(l2k_eth_lte_him_callback);
      if (err != 0) {
         L2K_ERR("failed to register HIM callback");
      }
   }
   if (err == 0) {
      device_open_count++;
   }
   spin_unlock_bh(&device_open_lock);
   if (err == 0) {
      spin_lock_bh(&(p->lock));
      memset(&p->stats, 0, sizeof(struct net_device_stats));
      spin_unlock_bh(&(p->lock));
      netif_start_queue(dev);
      L2K_DBG(5,"l2k_eth_open() returns");
   }
   return 0;
}


/** l2k_eth_close
 *    Closes network device. Usually it is called as a result of ifconfig down
 */
static int l2k_eth_close(struct net_device *dev)
{struct l2k_eth_private *p = netdev_priv(dev);
   L2K_DBG(5,"l2k_eth_close() called");
   L2K_ETH_PARANOID_WARN_ON(2,p == NULL);
   spin_lock_bh(&(p->lock));
   memset(&p->stats, 0, sizeof(struct net_device_stats));
   spin_unlock_bh(&(p->lock));
   netif_stop_queue(dev);
   spin_lock_bh(&device_open_lock);
   L2K_ETH_PARANOID_WARN_ON(1,device_open_count <= 0);
   device_open_count--;
   if (device_open_count == 0) {
      L2K_DBG(4,"unregistering HIM callback");
      if (lte_him_register_cb_from_ved(NULL) != 0) {
         L2K_ERR("failed to unregister HIM callback");
      }
   }
   spin_unlock_bh(&device_open_lock);
   L2K_DBG(5,"l2k_eth_close() returns");
   return 0;
}



/** l2k_eth_xmit
 *  Kernel packet transmit entry point
 */
static int l2k_eth_xmit(struct sk_buff *skb, struct net_device *dev)
{struct l2k_eth_private *p;
#if defined(L2K_ETH_TX_ASYNC)
 struct l2k_eth_work* work;
#endif
 int ret;
   L2K_DBG(5,"l2k_eth_xmit() called");
   L2K_ETH_PARANOID_WARN_ON(5,skb == NULL);
   L2K_ETH_PARANOID_WARN_ON(5,dev == NULL);
   L2K_ETH_PARANOID_WARN_ON(6,atomic_read(&skb->users) <= 0);
   p = netdev_priv(dev);
   L2K_ETH_PARANOID_WARN_ON(2,p == NULL);
   if (netif_queue_stopped(dev)) {
      L2K_DBG(1,"l2k_eth_xmit() called when netif_queue is stopped");
      //dev_kfree_skb_irq(skb);
      ret = NETDEV_TX_BUSY;
   } else {
      /**/
      spin_lock_bh(&(p->lock));
      p->stats.tx_packets++;
      p->stats.tx_bytes += skb->len;
      spin_unlock_bh(&(p->lock));
      /**/
#if defined(L2K_ETH_TX_ASYNC)
      work = l2k_eth_tx_work_alloc();
      if (work) {
         spin_lock_bh(&l2k_eth_tx_work_lock);
         if (l2k_eth_tx_work_queued > queue_high_thr &&
             !l2k_eth_tx_work_queued_high) {
            L2K_DBG(2,"l2k_eth_xmit(): TX queue above high threshold, stopping net queue");
            netif_stop_queue(dev);
            l2k_eth_tx_work_queued_high = 1;
         }
         spin_unlock_bh(&l2k_eth_tx_work_lock);
         work->dev = dev;
         work->skb = skb;
         INIT_WORK(&(work->work), l2k_eth_tx_task);
         queue_work(l2k_eth_tx_wq, &(work->work));
         ret = NETDEV_TX_OK;
      } else {
         /**/
         //spin_lock_bh(&(p->lock));
         //p->stats.tx_dropped++;
         //spin_unlock_bh(&(p->lock));
         /**/
         //dev_kfree_skb_irq(skb);
         ret = NETDEV_TX_BUSY;
      }
#else
      l2k_eth_handle_tx_packet(dev,skb);
      ret = NETDEV_TX_OK;
#endif
   }
   L2K_DBG1(5,"l2k_eth_xmit() returns %d",ret);
   return ret;
}

/** l2k_eth_get_stats
 *  Returns packet statistics to kernel
 */
static struct net_device_stats *l2k_eth_get_stats(struct net_device *dev)
{struct l2k_eth_private *p = netdev_priv(dev);
   L2K_DBG(5,"l2k_eth_get_stats() called");
   L2K_ETH_PARANOID_WARN_ON(2,p == NULL);
   L2K_DBG(5,"l2k_eth_get_stats() returns");
   return &p->stats;
}


/** l2k_eth_set_multicast_list
 *  Handles multicast changes (empty)
 */
static void l2k_eth_set_multicast_list(struct net_device *dev)
{
   L2K_DBG(5,"l2k_eth_set_multicast_list() called");
   L2K_DBG(5,"l2k_eth_set_multicast_list() returns");
}

/** l2k_eth_tx_timeout
 *  Handles transmit timeout (empty)
 */
static void l2k_eth_tx_timeout(struct net_device *dev)
{
   L2K_DBG(5,"l2k_eth_tx_timeout() called");
   L2K_ERR("TX timeout");
   L2K_DBG(5,"l2k_eth_tx_timeout() returns");
}

/** l2k_eth_netdev_ops
 *  Network device interface for kernel
 */
static const struct net_device_ops l2k_eth_netdev_ops = {
   .ndo_open               = l2k_eth_open,
   .ndo_stop               = l2k_eth_close,
   .ndo_start_xmit         = l2k_eth_xmit,
   .ndo_tx_timeout         = l2k_eth_tx_timeout,
   .ndo_get_stats          = l2k_eth_get_stats,
   .ndo_set_multicast_list = l2k_eth_set_multicast_list,
   .ndo_validate_addr      = eth_validate_addr,
   .ndo_set_mac_address    = eth_mac_addr,
   .ndo_change_mtu         = eth_change_mtu,
};


/** l2k_eth_setup
 *  Configures specific l2k_eth instance
 */
static void __init l2k_eth_setup(struct net_device *dev)
{struct l2k_eth_private *p = netdev_priv(dev);
   L2K_DBG(5,"l2k_eth_setup() called");
   L2K_ETH_PARANOID_WARN_ON(2,p == NULL);
   ether_setup(dev);
   dev->netdev_ops = &l2k_eth_netdev_ops;
   /* generate local MAC address */
   random_ether_addr(dev->dev_addr);
   /* generate remote (modem) MAC address */
   random_ether_addr(&(p->l2k_eth_addr));
#ifdef L2K_ETH_IPV6_SUPPORT
   p->ipv6_dad = ipv6_dad;
#endif
   /**/
   spin_lock_init(&(p->lock));
   /**/
   L2K_DBG(5,"l2k_eth_setup() returns");
}


/** l2k_eth_init_module
 *   Module initialization
 */
static int __init l2k_eth_init_module(void)
{int err = 0;
 int n;
#ifdef L2K_ETH_SYSFS
 struct device *d;
#endif
 struct net_device *dev;
 struct l2k_eth_private *p;

   L2K_DBG(3,"l2k_eth_init_module() called");
   printk(KERN_INFO "L2000 Virtual Ethernet driver. Version: %s\n",l2k_eth_driver_version);
   device_open_count = 0;
   if (num_of_devices <= 0 || num_of_devices > L2K_ETH_NUMBER_OF_DEVICES_MAX) {
      num_of_devices = L2K_ETH_NUMBER_OF_DEVICES_DEFAULT;
   }
#if defined(L2K_ETH_TX_ASYNC) || defined(L2K_ETH_RX_ASYNC)
   if (queue_max <= 0 || queue_max > L2K_ETH_QUEUE_MAX) {
      queue_max = L2K_ETH_QUEUE_MAX;
   }
   if (queue_low_thr <= 0 || queue_low_thr > L2K_ETH_QUEUE_MAX) {
      queue_max = L2K_ETH_QUEUE_LOW_THR;
   }
   if (queue_high_thr <= 0 || queue_high_thr > L2K_ETH_QUEUE_MAX) {
      queue_max = L2K_ETH_QUEUE_HIGH_THR;
   }
   if (queue_high_thr < queue_low_thr) {
      queue_high_thr = queue_low_thr;
   }
#endif
#ifdef L2K_ETH_IPV6_SUPPORT
   if (ipv6_dad != 0) {
      ipv6_dad = 1;
   }
#endif
   for (n = 0; n < L2K_ETH_NUMBER_OF_DEVICES_MAX; n++) {
      devices[n] = NULL;
   }
   spin_lock_init(&device_open_lock);
#ifdef L2K_ETH_TX_ASYNC
   if (err == 0) {
      /* create shared TX workqueue */
#ifdef L2K_ETH_WORK_STRUCT_POOL_SIZE
      L2K_DBG(4,"initializing TX work pool");
      for(n = 0; n < L2K_ETH_WORK_STRUCT_POOL_SIZE - 1; n++) {
         l2k_eth_tx_work_pool[n].next_avail = &l2k_eth_tx_work_pool[n+1];
         L2K_ETH_WORK_SET_TX(&l2k_eth_tx_work_pool[n]);
         L2K_ETH_SET_WORK_UNUSED(&l2k_eth_tx_work_pool[n]);
      }
      l2k_eth_tx_work_pool[L2K_ETH_WORK_STRUCT_POOL_SIZE - 1].next_avail = NULL;
      l2k_eth_tx_work_avail = &l2k_eth_tx_work_pool[0];
#endif
      l2k_eth_tx_work_queued = 0;
      l2k_eth_tx_work_queued_peak = 0;
      l2k_eth_tx_work_queued_overruns = 0;
      l2k_eth_tx_work_queued_high = 0;
      L2K_DBG(4,"allocating TX workqueue");
      l2k_eth_tx_wq = create_singlethread_workqueue(L2K_ETH_NAME "_tx");
      if (!l2k_eth_tx_wq) {
         L2K_ERR("failed to allocate TX workqueue")
         err = -ENOMEM;
      }
      spin_lock_init(&l2k_eth_tx_work_lock);
   }
#endif
#ifdef L2K_ETH_RX_ASYNC
   if (err == 0) {
      /* create shared RX workqueue */
#ifdef L2K_ETH_WORK_STRUCT_POOL_SIZE
      L2K_DBG(4,"initializing RX work pool");
      for(n = 0; n < L2K_ETH_WORK_STRUCT_POOL_SIZE - 1; n++) {
         l2k_eth_rx_work_pool[n].next_avail = &l2k_eth_rx_work_pool[n+1];
         L2K_ETH_WORK_SET_RX(&l2k_eth_rx_work_pool[n]);
         L2K_ETH_SET_WORK_UNUSED(&l2k_eth_rx_work_pool[n]);
      }
      l2k_eth_rx_work_pool[L2K_ETH_WORK_STRUCT_POOL_SIZE - 1].next_avail = NULL;
      l2k_eth_rx_work_avail = &l2k_eth_rx_work_pool[0];
#endif
      l2k_eth_rx_work_queued = 0;
      l2k_eth_rx_work_queued_peak = 0;
      l2k_eth_rx_work_queued_overruns = 0;
      l2k_eth_rx_work_queued_high = 0;
      L2K_DBG(4,"allocating RX workqueue");
      l2k_eth_rx_wq = create_singlethread_workqueue(L2K_ETH_NAME "_rx");
      if (!l2k_eth_rx_wq) {
         L2K_ERR("failed to allocate RX workqueue")
         err = -ENOMEM;
      }
      spin_lock_init(&l2k_eth_rx_work_lock);
   }
#endif
   if (err == 0) {
      /* allocate devices */
      L2K_DBG(4,"registering network devices");
      for (n = 0; (err == 0) && (n < num_of_devices); n++) {
         dev = alloc_netdev(sizeof(struct l2k_eth_private),
                            L2K_ETH_NAME "%d", l2k_eth_setup);

         if (!dev) {
            L2K_ERR("failed to allocate device")
            err = -ENOMEM;
         }
         if (err == 0) {
            p = netdev_priv(dev);
            /**/
            p->pdn = -1;
            /**/
            err = register_netdev(dev);
#ifdef L2K_ETH_SYSFS
            if (err == 0) {
               d = &(dev->dev);
               err = device_create_file(d, &dev_attr_pdn);
               if (err != 0) {
                  L2K_ERR("failed to create sysfs node")
               }
            }
#ifdef L2K_ETH_DEBUG_LEVEL
            if (err == 0) {
               err = device_create_file(d, &dev_attr_debug_level);
               if (err != 0) {
                  L2K_ERR("failed to create sysfs node")
               }
            }
#endif
#ifdef L2K_ETH_TX_ASYNC
            if (err == 0) {
               err = device_create_file(d, &dev_attr_txqueue);
               if (err != 0) {
                  L2K_ERR("failed to create sysfs node")
               }
            }
#endif
#ifdef L2K_ETH_RX_ASYNC
            if (err == 0) {
               err = device_create_file(d, &dev_attr_rxqueue);
               if (err != 0) {
                  L2K_ERR("failed to create sysfs node")
               }
            }
#endif
#ifdef L2K_ETH_IPV6_SUPPORT
            if (err == 0) {
                err = device_create_file(d, &dev_attr_ipv6_dad);
                if (err != 0) {
                   L2K_ERR("failed to create sysfs node")
                }
            }
#endif
#endif
            if (err == 0) {
               devices[n] = dev;
            } else {
               L2K_ERR1("failed to register device, error = %d",err);
               free_netdev(dev);
            }
         }
      }
   }
   /*
    * cleanup in error case
    */
   if (err != 0) {
      /* release devices */
      L2K_DBG(4,"unregistering network devices");
      for (n = 0; n < L2K_ETH_NUMBER_OF_DEVICES_MAX; n++) {
         if (devices[n] != NULL) {
#ifdef L2K_ETH_SYSFS
            d = &(devices[n]->dev);
            device_remove_file(d, &dev_attr_pdn);
#ifdef L2K_ETH_DEBUG_LEVEL
            device_remove_file(d, &dev_attr_debug_level);
#endif
#ifdef L2K_ETH_TX_ASYNC
            device_remove_file(d, &dev_attr_txqueue);
#endif
#ifdef L2K_ETH_RX_ASYNC
            device_remove_file(d, &dev_attr_rxqueue);
#endif
#ifdef L2K_ETH_IPV6_SUPPORT
            device_remove_file(d, &dev_attr_ipv6_dad);
#endif
#endif
            unregister_netdev(devices[n]);
            free_netdev(devices[n]);
            devices[n] = NULL;
         }
      }
#ifdef L2K_ETH_TX_ASYNC
      /* release TX workqueue */
      if (l2k_eth_tx_wq)   {
         L2K_DBG(4,"releasing TX workqueue");
         flush_workqueue(l2k_eth_tx_wq);
         destroy_workqueue(l2k_eth_tx_wq);
         l2k_eth_tx_wq = NULL;
      }
#endif
#ifdef L2K_ETH_RX_ASYNC
      /* release RX workqueue */
      if (l2k_eth_rx_wq)   {
         L2K_DBG(4,"releasing RX workqueue");
         flush_workqueue(l2k_eth_rx_wq);
         destroy_workqueue(l2k_eth_rx_wq);
         l2k_eth_rx_wq = NULL;
      }
#endif
   }
   L2K_DBG1(3,"l2k_eth_init_module() returns %d",err);
   return err;
}

/** l2k_eth_exit_module
 *  Module de-initialization
 */
static void __exit l2k_eth_exit_module(void)
{
 int n;
#ifdef L2K_ETH_SYSFS
 struct device *d;
#endif
   L2K_DBG(3,"l2k_eth_exit_module() called");
   L2K_DBG(4,"unregistering network devices");
   for (n = 0; n < L2K_ETH_NUMBER_OF_DEVICES_MAX; n++) {
      if (devices[n] != NULL) {
#ifdef L2K_ETH_SYSFS
         d = &(devices[n]->dev);
         device_remove_file(d, &dev_attr_pdn);
#ifdef L2K_ETH_DEBUG_LEVEL
         device_remove_file(d, &dev_attr_debug_level);
#endif
#ifdef L2K_ETH_TX_ASYNC
         device_remove_file(d, &dev_attr_txqueue);
#endif
#ifdef L2K_ETH_RX_ASYNC
         device_remove_file(d, &dev_attr_rxqueue);
#endif
#ifdef L2K_ETH_IPV6_SUPPORT
         device_remove_file(d, &dev_attr_ipv6_dad);
#endif
#endif
         unregister_netdev(devices[n]);
         free_netdev(devices[n]);
         devices[n] = NULL;
      }
   }
#if defined(L2K_ETH_TX_ASYNC)
   if (l2k_eth_tx_wq) {
      L2K_DBG(4,"releasing TX workqueue");
      flush_workqueue(l2k_eth_tx_wq);
      destroy_workqueue(l2k_eth_tx_wq);
      l2k_eth_tx_wq = NULL;
   }
#endif
#if defined(L2K_ETH_RX_ASYNC)
   if (l2k_eth_rx_wq) {
      L2K_DBG(4,"releasing RX workqueue");
      flush_workqueue(l2k_eth_rx_wq);
      destroy_workqueue(l2k_eth_rx_wq);
      l2k_eth_rx_wq = NULL;
   }
#endif
   device_open_count = 0;
   L2K_DBG(3,"l2k_eth_exit_module() returns");
}

module_init(l2k_eth_init_module);
module_exit(l2k_eth_exit_module);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Virtual Ethernet driver for L2000 LTE modem");
module_param(num_of_devices, int, 0);
MODULE_PARM_DESC(num_of_devices, "Number of ethernet devices for L2000 LTE modem network driver");
#ifdef L2K_ETH_DEBUG_LEVEL
module_param(debug_level, int, 0);
MODULE_PARM_DESC(debug_level, "Debug output level for L2000 LTE modem network driver");
#endif
#if defined(L2K_ETH_TX_ASYNC) || defined(L2K_ETH_RX_ASYNC)
module_param(queue_max, int, 0);
MODULE_PARM_DESC(queue_max, "max queue size for L2000 LTE modem network driver");
module_param(queue_low_thr, int, 0);
MODULE_PARM_DESC(queue_max, "Queue low threshold for L2000 LTE modem network driver");
module_param(queue_high_thr, int, 0);
MODULE_PARM_DESC(queue_max, "Queue high threshold for L2000 LTE modem network driver");
#endif
#ifdef L2K_ETH_IPV6_SUPPORT
module_param(ipv6_dad, int, 0);
MODULE_PARM_DESC(ipv6_dad, "Whether to allow DAD to L2000 LTE modem");
#endif

/* Eof : l2k_eth.c */
