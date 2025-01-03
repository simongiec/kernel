/******************************************************************************
 *
 * Copyright(c) 2016 - 2022 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/

#include <linux/kernel.h>
#include <linux/gfp.h>
#include <drv_types.h>
#include <rtw_mem.h>

#ifndef RTKM_MAX_RECVBUF_SZ
#define RTKM_MAX_RECVBUF_SZ (0x8000) /* 32k */
#endif /* RTKM_MAX_RECVBUF_SZ */

#ifndef RTKM_MAX_NR_PREALLOC_RECV_SKB
#ifndef NR_PREALLOC_RECV_SKB
#define NR_PREALLOC_RECV_SKB 8
#endif /* NR_PREALLOC_RECV_SKB */
#define RTKM_MAX_NR_PREALLOC_RECV_SKB \
	(NR_PREALLOC_RECV_SKB * CONFIG_IFACE_NUMBER)
#endif /* RTKM_MAX_NR_PREALLOC_RECV_SKB */

#if defined(CONFIG_STACKTRACE) && \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0))
#define STACKTRACE 1
#define MAX_STACK_TRACE 4
#endif /* CONFIG_STACKTRACE */

#define TRACE_ORDER(a, b) ((a) && (((a)&BIT(b)) > 0))

#if (defined(CONFIG_RTKM) && defined(CONFIG_RTKM_STANDALONE))
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#undef RTW_PRINT
#define RTW_PRINT pr_info
unsigned int rtw_drv_log_level = 0; /* for compiler */
#else /* !CONFIG_RTKM */
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "RTKM: " fmt
#endif /* CONFIG_RTKM */

struct sk_buff_head rtkm_skb_q;

struct phy_mem_list {
	_lock lock;
	_list list;
	struct rb_root rb_tree;
	unsigned short entries;
	unsigned short used;
	unsigned short peak;
	unsigned char order;
} rtkm_phy_list[MAX_ORDER];

struct mem_entry {
	_list list;
	struct rb_node rbn;
	void *data;
	size_t size;
#ifdef STACKTRACE
#ifndef CONFIG_ARCH_STACKWALK
	struct stack_trace trace;
#endif /* CONFIG_ARCH_STACKWALK */
	unsigned long stack_entries[MAX_STACK_TRACE];
#endif /* STACKTRACE */
	unsigned char order;
	unsigned char is_use;
};

#ifdef STACKTRACE
#ifdef CONFIG_ARCH_STACKWALK
static inline void stacktrace_print(const struct mem_entry *entries,
				    unsigned int nr_entries, int spaces)
{
	stack_trace_print(entries->stack_entries, nr_entries, spaces);
}

static inline int stacktrace_save(struct mem_entry *store)
{
	return stack_trace_save(store->stack_entries,
				ARRAY_SIZE(store->stack_entries), 1);
}
#else /* !CONFIG_ARCH_STACKWALK */
static inline void stacktrace_print(const struct mem_entry *entries,
				    unsigned int nr_entries, int spaces)
{
	stack_trace_print(entries->trace.entries, nr_entries, spaces);
}

static inline void stacktrace_save(struct mem_entry *store)
{
	store->trace.skip = 0;
	store->trace.nr_entries = 0;
	store->trace.entries = store->stack_entries;
	store->trace.max_entries = MAX_STACK_TRACE;
	save_stack_trace(&store->trace);
}
#endif /* CONFIG_ARCH_STACKWALK */
#else /* !STACKTRACE */
#define stacktrace_print(a, b, c)
#define stacktrace_save(a)
#endif /* STACKTRACE */

/* Trace mpool */
static unsigned int rtkm_trace = 0;
module_param(rtkm_trace, uint, 0644);
MODULE_PARM_DESC(rtkm_trace, "Trace memory pool");

/* Preallocated memory expansion */
static bool rtkm_mem_exp = 1;
module_param(rtkm_mem_exp, bool, 0644);
MODULE_PARM_DESC(rtkm_mem_exp, "Preallocated memory expansion");

#ifndef RTKM_MPOOL_0
#define RTKM_MPOOL_0 0
#endif
#ifndef RTKM_MPOOL_1
#define RTKM_MPOOL_1 0
#endif
#ifndef RTKM_MPOOL_2
#define RTKM_MPOOL_2 0
#endif
#ifndef RTKM_MPOOL_3
#define RTKM_MPOOL_3 0
#endif
#ifndef RTKM_MPOOL_4
#define RTKM_MPOOL_4 0
#endif
#ifndef RTKM_MPOOL_5
#define RTKM_MPOOL_5 0
#endif
#ifndef RTKM_MPOOL_6
#define RTKM_MPOOL_6 0
#endif
#ifndef RTKM_MPOOL_7
#define RTKM_MPOOL_7 0
#endif
#ifndef RTKM_MPOOL_8
#define RTKM_MPOOL_8 0
#endif

/* Preallocated memory pool  */
static int mpool[MAX_ORDER] = {
	[0] = RTKM_MPOOL_0, [1] = RTKM_MPOOL_1, [2] = RTKM_MPOOL_2,
	[3] = RTKM_MPOOL_3, [4] = RTKM_MPOOL_4, [5] = RTKM_MPOOL_5,
	[6] = RTKM_MPOOL_6, [7] = RTKM_MPOOL_7, [8] = RTKM_MPOOL_8
};
static int n_mpool = 1;
module_param_array(mpool, int, &n_mpool, 0644);
MODULE_PARM_DESC(mpool, "Preallocated memory pool");

static inline void dump_mstatus(void *sel)
{
	int i;
	unsigned int a, b, c;
	long unsigned int musage = 0;

	a = b = c = 0;

	RTW_PRINT_SEL(sel,
		      "====================== RTKM ======================\n");
	RTW_PRINT_SEL(sel, "%6s %10s %10s %10s %10s\n", "order", "use", "peak",
		      rtkm_mem_exp ? "alloc+" : "alloc", "size");
	RTW_PRINT_SEL(sel,
		      "--------------------------------------------------\n");
	for (i = 0; i < MAX_ORDER; i++) {
		if (rtkm_phy_list[i].entries) {
			RTW_PRINT_SEL(
				sel, "%6d %10d %10d %10d %10lu\n", i,
				rtkm_phy_list[i].used, rtkm_phy_list[i].peak,
				rtkm_phy_list[i].entries,
				(rtkm_phy_list[i].entries) * (PAGE_SIZE << i));
			a += rtkm_phy_list[i].used;
			b += rtkm_phy_list[i].peak;
			c += rtkm_phy_list[i].entries;
			musage += (rtkm_phy_list[i].entries) * (PAGE_SIZE << i);
		}
	}
	RTW_PRINT_SEL(sel, "%6s %10d %10d %10d %10lu\n", "sum", a, b, c,
		      musage);
}

void rtkm_dump_mstatus(void *sel)
{
	dump_mstatus(sel);
}
EXPORT_SYMBOL(rtkm_dump_mstatus);

void rtkm_set_trace(unsigned int mask)
{
	rtkm_trace = mask;
}
EXPORT_SYMBOL(rtkm_set_trace);

static void rb_insert_mem(struct phy_mem_list *mlist, struct mem_entry *entry)
{
	struct rb_node **p = &mlist->rb_tree.rb_node;
	struct rb_node *parent = NULL;
	struct mem_entry *tmp = NULL;

	while (*p) {
		parent = *p;
		tmp = rb_entry(parent, struct mem_entry, rbn);

		if (tmp->data < entry->data)
			p = &(*p)->rb_left;
		else
			p = &(*p)->rb_right;
	}

	rb_link_node(&entry->rbn, parent, p);
	rb_insert_color(&entry->rbn, &mlist->rb_tree);
}

static struct mem_entry *rb_find_mem(struct phy_mem_list *mlist,
				     const void *objp)
{
	struct rb_node *n = mlist->rb_tree.rb_node;
	struct mem_entry *entry = NULL;

	while (n) {
		entry = rb_entry(n, struct mem_entry, rbn);

		if (entry->data == objp)
			return entry;
		else if (entry->data < objp)
			n = n->rb_left;
		else
			n = n->rb_right;
	}

	return NULL;
}

static inline void *create_mem_entry(int order)
{
	struct mem_entry *entry;

	entry = rtw_malloc(sizeof(struct mem_entry));
	if (entry == NULL) {
		pr_warn("%s: alloc memory entry fail!\n", __func__);
		return NULL;
	}
	entry->order = order;
	entry->is_use = _FALSE;
	/* get memory by pages */
	entry->data = (void *)__get_free_pages(
		in_interrupt() ? GFP_ATOMIC : GFP_KERNEL, entry->order);
	if (entry->data == NULL) {
		rtw_mfree(entry, sizeof(struct mem_entry));
		pr_warn("%s: alloc memory oreder-%d fail!\n", __func__, order);
		return NULL;
	}

	return entry;
}

static inline void *_kmalloc(size_t size, gfp_t flags, int clear)
{
	int order = 0;
	int warn = _FALSE;
	struct mem_entry *entry = NULL;
	struct mem_entry *temp_entry = NULL;
	_irqL irqL;

	order = get_order(size);
	if (order >= MAX_ORDER) {
		pr_warn("%s: Out-of-bounds size %zu (order-%d)\n", __func__,
			size, order);
		WARN_ON(1);
		return NULL;
	}

	if (rtkm_phy_list[order].entries == rtkm_phy_list[order].used) {
		if (rtkm_mem_exp) {
			warn = _TRUE;
			pr_warn("%s: No enough order-%d pool\n", __func__,
				order);
			entry = create_mem_entry(order);
			if (entry) {
				_enter_critical(&rtkm_phy_list[order].lock,
						&irqL);
				list_add_tail(
					&entry->list,
					&rtkm_phy_list[entry->order].list);
				rtkm_phy_list[entry->order].entries++;
				_exit_critical(&rtkm_phy_list[order].lock,
					       &irqL);
			}
		}

		if (entry == NULL) {
			pr_warn("%s: No more memory for size %zu\n", __func__,
				size);
			WARN_ON(1);
			return NULL;
		}
	}

	_enter_critical(&rtkm_phy_list[order].lock, &irqL);
	list_for_each_entry_safe(entry, temp_entry, &rtkm_phy_list[order].list, list) {
		if (entry->is_use == _FALSE) {
			entry->is_use = _TRUE;
			entry->size = size;
			if (clear == _TRUE)
				memset(entry->data, 0, size);
			stacktrace_save(entry);
			rtkm_phy_list[order].used++;
			list_move_tail(&entry->list, &rtkm_phy_list[order].list);
			rb_insert_mem(&rtkm_phy_list[order], entry);
			break;
		}
	}
	if (rtkm_phy_list[order].peak < rtkm_phy_list[order].used)
		rtkm_phy_list[order].peak = rtkm_phy_list[order].used;
	_exit_critical(&rtkm_phy_list[order].lock, &irqL);

	if ((warn) || TRACE_ORDER(rtkm_trace, order)) {
		pr_info("%s: require(%p, %zu) usage(%d %u/%u)\n", __func__,
			entry->data, entry->size, order,
			rtkm_phy_list[order].used,
			rtkm_phy_list[order].entries);
		stacktrace_print(entry, MAX_STACK_TRACE, 0);
	}

	return entry->data;
}

static inline void _kfree(const void *objp, size_t size)
{
	int order = 0;
	struct mem_entry *entry;
	_irqL irqL;

	order = get_order(size);
	if (order >= MAX_ORDER) {
		pr_warn("%s: not found (%p, %zu)\n", __func__, objp, size);
		pr_warn("%s: Out-of-bounds size %zu (order-%d)\n", __func__,
			size, order);
		WARN_ON(1);
		return;
	}

	if (list_empty(&rtkm_phy_list[order].list)) {
		pr_warn("%s: list is empty (%p, %zu)\n", __func__, objp, size);
		return;
	}

	_enter_critical(&rtkm_phy_list[order].lock, &irqL);
	entry = rb_find_mem(&rtkm_phy_list[order], objp);
	if (entry && (entry->is_use == _TRUE) && (entry->data == objp)) {
		if (TRACE_ORDER(rtkm_trace, order)) {
			pr_info("%s: release(%p, %zu)\n", __func__, objp, size);
		}
		rtw_list_delete(&entry->list);
		rtkm_phy_list[order].used--;
		entry->is_use = _FALSE;
		entry->size = 0;
		rb_erase(&entry->rbn, &rtkm_phy_list[order].rb_tree);
		list_add(&entry->list, &rtkm_phy_list[order].list);
	} else {
		pr_warn("%s: not found (%p, %zu)\n", __func__, objp, size);
	}
	_exit_critical(&rtkm_phy_list[order].lock, &irqL);
}

void *rtkm_kmalloc(size_t size, gfp_t flags)
{
	if (size > RTKM_MGMT_SIZE)
		return _kmalloc(size, flags, _FALSE);
	else
		return kmalloc(size, flags);
}
EXPORT_SYMBOL(rtkm_kmalloc);

void *rtkm_kzalloc(size_t size, gfp_t flags)
{
	if (size > RTKM_MGMT_SIZE)
		return _kmalloc(size, flags, _TRUE);
	else
		return kzalloc(size, flags);
}
EXPORT_SYMBOL(rtkm_kzalloc);

void rtkm_kfree(const void *objp, size_t size)
{
	if (size > RTKM_MGMT_SIZE)
		return _kfree(objp, size);
	else
		return kfree(objp);
}
EXPORT_SYMBOL(rtkm_kfree);

static inline int rtkm_init_phy(void)
{
	int ret = 0, i, j;
	struct mem_entry *entry;

	pr_info("%s", __func__);
	pr_info("%s: memory expansion:%d\n", __func__, rtkm_mem_exp);

	for (i = (MAX_ORDER - 1); i >= 0; i--) {
		INIT_LIST_HEAD(&rtkm_phy_list[i].list);
		_rtw_spinlock_init(&rtkm_phy_list[i].lock);
		rtkm_phy_list[i].rb_tree = RB_ROOT;

		for (j = 0; (ret == 0) && (j < mpool[i]); j++) {
			entry = create_mem_entry(i);
			if (entry == NULL) {
				ret = -ENOMEM;
				break;
			}

			list_add_tail(&entry->list,
				      &rtkm_phy_list[entry->order].list);
			rtkm_phy_list[entry->order].entries++;
		}
	}
	if (ret == 0)
		dump_mstatus(RTW_DBGDUMP);

	return ret;
}

static inline void rtkm_destroy_phy(void)
{
	int i = 0;
	struct mem_entry *entry;

	pr_info("%s", __func__);
	dump_mstatus(RTW_DBGDUMP);

	for (i = 0; i < MAX_ORDER; i++) {
		if (rtkm_phy_list[i].used)
			pr_err("%s: memory leak! order=%d num=%d\n", __func__,
			       i, rtkm_phy_list[i].used);

		if (rtkm_phy_list[i].rb_tree.rb_node != NULL)
			pr_err("%s: rb tree leak! order=%d\n", __func__, i);

		while (!list_empty(&rtkm_phy_list[i].list)) {
			entry = list_entry(rtkm_phy_list[i].list.next,
					   struct mem_entry, list);
			list_del_init(&entry->list);
			if (entry->is_use == _TRUE) {
				rb_erase(&entry->rbn,
					 &rtkm_phy_list[i].rb_tree);
				pr_err("%s: memory leak! (%p, %zu)\n", __func__,
				       entry->data, entry->size);
				stacktrace_print(entry, MAX_STACK_TRACE, 0);
			}
			if (entry->data)
				free_pages((unsigned long)(entry->data),
					   entry->order);
			entry->data = NULL;
			entry->size = 0;
			entry->is_use = _FALSE;
			rtw_mfree(entry, sizeof(struct mem_entry));
			entry = NULL;
			rtkm_phy_list[i].entries--;
		}
		_rtw_spinlock_free(&rtkm_phy_list[i].lock);
	}
}

u16 rtkm_get_max_buff_size(void)
{
	return RTKM_MAX_RECVBUF_SZ;
}
EXPORT_SYMBOL(rtkm_get_max_buff_size);

u8 rtkm_get_max_nr_rx_skb(void)
{
	return RTKM_MAX_NR_PREALLOC_RECV_SKB;
}
EXPORT_SYMBOL(rtkm_get_max_nr_rx_skb);

inline struct sk_buff *rtkm_alloc_skb(unsigned int length)
{
	struct sk_buff *skb = NULL;

	if (length > (RTKM_MAX_RECVBUF_SZ + RECVBUFF_ALIGN_SZ)) {
		pr_warn("%s: allocate size(%u) > MAX_SZ(%d + %d)\n", __func__,
			length, RTKM_MAX_RECVBUF_SZ, RECVBUFF_ALIGN_SZ);
		WARN_ON(1);
		return skb;
	}

	skb = skb_dequeue(&rtkm_skb_q);

	return skb;
}
EXPORT_SYMBOL(rtkm_alloc_skb);

void rtkm_kfree_skb_any(struct sk_buff *skb)
{
	skb_queue_tail(&rtkm_skb_q, skb);
}
EXPORT_SYMBOL(rtkm_kfree_skb_any);

inline int rtkm_init_skb(void)
{
	int i;
	int ret = 0;
	SIZE_PTR tmpaddr = 0;
	SIZE_PTR alignment = 0;
	struct sk_buff *pskb = NULL;

	pr_info("%s: RTKM_MAX_NR_PREALLOC_RECV_SKB: %d\n", __func__,
		RTKM_MAX_NR_PREALLOC_RECV_SKB);
	pr_info("%s: RTKM_MAX_RECVBUF_SZ: %d\n", __func__, RTKM_MAX_RECVBUF_SZ);

	skb_queue_head_init(&rtkm_skb_q);

	for (i = 0; i < RTKM_MAX_NR_PREALLOC_RECV_SKB; i++) {
		pskb = __dev_alloc_skb(RTKM_MAX_RECVBUF_SZ + RECVBUFF_ALIGN_SZ,
				       in_interrupt() ? GFP_ATOMIC :
							GFP_KERNEL);
		if (pskb) {
			tmpaddr = (SIZE_PTR)pskb->data;
			alignment = tmpaddr & (RECVBUFF_ALIGN_SZ - 1);
			skb_reserve(pskb, (RECVBUFF_ALIGN_SZ - alignment));

			skb_queue_tail(&rtkm_skb_q, pskb);
		} else {
			printk("%s: alloc skb memory fail!\n", __func__);
			ret = -ENOMEM;
		}

		pskb = NULL;
	}

	printk("%s: rtkm_skb_q len : %d\n", __func__,
	       skb_queue_len(&rtkm_skb_q));

	return ret;
}

inline void rtkm_destroy_skb(void)
{
	pr_info("%s", __func__);

	if (skb_queue_len(&rtkm_skb_q))
		pr_info("%s: rtkm_skb_q len : %d\n", __func__,
			skb_queue_len(&rtkm_skb_q));

	skb_queue_purge(&rtkm_skb_q);

	return;
}

int rtkm_prealloc_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	ret = rtkm_init_phy();
	if (ret == -ENOMEM) {
		pr_err("No enough memory for phiscal.");
		rtkm_destroy_phy();
	}

	ret = rtkm_init_skb();
	if (ret == -ENOMEM) {
		pr_err("No enough memory for skb.");
		rtkm_destroy_skb();
	}

	pr_info("%s: done ret=%d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL(rtkm_prealloc_init);

void rtkm_prealloc_destroy(void)
{
	pr_info("%s\n", __func__);

	rtkm_destroy_phy();
	rtkm_destroy_skb();

	pr_info("%s: done\n", __func__);
}
EXPORT_SYMBOL(rtkm_prealloc_destroy);
