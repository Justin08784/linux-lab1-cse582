/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PTRACE_UTILITIES_H
#define _LINUX_PTRACE_UTILITIES_H

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/rhashtable.h>
#include <linux/bug.h>


/*
 * Snapshot structs */
struct ptrace_snapshot {
	unsigned long addr;
	unsigned int size;
	void *data;
	struct rhash_head psnap_rhash;
};

struct ptrace_snapshot_ctx {
	struct rhashtable *snapshots;
	unsigned int num_active_snapshots;
	unsigned int total_snapshot_size;
};

static const struct rhashtable_params psnap_rhash_params = {
	.key_len		= sizeof_field(struct ptrace_snapshot, addr),
	.key_offset		= offsetof(struct ptrace_snapshot, addr),
	.head_offset		= offsetof(struct ptrace_snapshot, psnap_rhash),
	.automatic_shrinking	= true,
};


static inline void free_ptrace_snapshot_ctx(struct ptrace_snapshot_ctx *ctx)
{
	struct bucket_table *tbl;
	struct rhashtable *ht = ctx->snapshots;
	int bkt;

	tbl = rht_dereference(ht->tbl, ht);
	if (!tbl) {
		printk(KERN_ERR "free_ptrace_snapshot_ctx: table is NULL.\n");
		return;
	}

	for (bkt = 0; bkt < tbl->size; ++bkt) {
		struct ptrace_snapshot *cur;
		struct rhash_head *pos;

		// Iterate over each entry in the bucket
		rht_for_each_entry(cur, pos, tbl, bkt, psnap_rhash) {
			printk(KERN_EMERG "free_ptrace_snapshot_ctx: cur->size: %d\n", cur->size);

			kvfree(cur);
		}
	}
	
	rhashtable_destroy(ht);
}

static inline int alloc_init_snapshots(struct ptrace_snapshot_ctx *ctx)
{
	void *tmp;
	int rv;
	tmp = kvmalloc(sizeof(struct rhashtable), GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;
	rv = rhashtable_init(tmp, &psnap_rhash_params);
	ctx->snapshots = tmp;
	return rv;
}


static inline int insert_snapshot(struct ptrace_snapshot_ctx *ctx,
		    struct ptrace_snapshot *snap)
{
	struct rhashtable *ht = ctx->snapshots;
	int rv;

	rv = rhashtable_insert_fast(ht, &snap->psnap_rhash, psnap_rhash_params);
	if (rv) {
		printk(KERN_ERR "Failed to insert snapshot into rhashtable: %d\n", rv);
		kvfree(snap);
		return rv;
	}

	++ctx->num_active_snapshots;
	ctx->total_snapshot_size += snap->size;

	return 0;
}


static inline int remove_snapshot(struct ptrace_snapshot_ctx *ctx,
		    struct ptrace_snapshot *snap)
{
	struct rhashtable *ht = ctx->snapshots;
	int rv = rhashtable_remove_fast(ht, &snap->psnap_rhash, psnap_rhash_params);

	if (rv == -ENOENT)
		return rv;

	--ctx->num_active_snapshots;
	ctx->total_snapshot_size -= snap->size;

	kvfree(snap->data);
	kvfree(snap);

	return 0;
}

static inline struct ptrace_snapshot *lookup_snapshot(struct rhashtable *ht, unsigned long addr)
{
	unsigned long *key = &addr;
	return rhashtable_lookup(ht, key, psnap_rhash_params);
}
#endif