// #include <linux/types.h>
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


void free_ptrace_snapshot_ctx(struct ptrace_snapshot_ctx *ctx)
{
	struct bucket_table *tbl;
	struct rhashtable *ht = ctx->snapshots;
	int bkt;

	tbl = rht_dereference(ht->tbl, ht);
 
        BUG_ON(!tbl);

	for (bkt = 0; bkt < tbl->size; ++bkt) {
		struct ptrace_snapshot *cur;
		struct rhash_head *pos;

		// Iterate over each entry in the bucket
		rht_for_each_entry(cur, pos, tbl, bkt, psnap_rhash) {
			printk(KERN_EMERG "free_ptrace_snapshot_ctx: cur->size: %d\n", cur->size);

			kfree(cur);
		}
	}
	
	rhashtable_destroy(ht);
}

int alloc_init_snapshots(struct ptrace_snapshot_ctx *ctx)
{
	void *tmp;
	tmp = kmalloc(sizeof(struct rhashtable), GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;
	rhashtable_init(tmp, psnap_rhash_params);
	ctx->snapshots = tmp;
}


int insert_snapshot(struct ptrace_snapshot_ctx *ctx,
		    unsigned long addr,
		    unsigned int size,
		    void *data)
{
	struct rhashtable *ht = ctx->snapshots;
	struct ptrace_snapshot *snap = kvmalloc(sizeof(struct ptrace_snapshot), GFP_KERNEL);
	int rv;

	if (!snap)
		return -ENOMEM;

	snap->addr = addr;
	snap->size = size;
	snap->data = data;

	rv = rhashtable_insert_fast(ht, &snap->psnap_rhash, psnap_rhash_params);
	if (ret) {
		printk(KERN_ERR "Failed to insert snapshot into rhashtable: %d\n", ret);
		kvfree(snap);
		return ret;
	}

	++ctx->num_active_snapshots;
	ctx->total_snapshot_size += size;

	return 0;
}


int remove_snapshot(struct ptrace_snapshot_ctx *ctx,
		    struct ptrace_snapshot *snap)
{
	struct rhashtable *ht = ctx->snapshots;
	int rv = rhashtable_remove_fast(ht, &snap->psnap_rhash, psnap_rhash_params);

	if (rv == -ENONENT)
		return rv;

	--ctx->num_active_snapshots;
	ctx->total_snapshot_size -= snap->size;

	kvfree(snap->data);
	kvfree(snap);

	return 0;
}

bool lookup_snapshot(struct ptrace_snapshot *ht, unsigned long addr)
{
	return rhashtable_lookup(ht, addr, psnap_rhash_params);
}
