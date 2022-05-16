// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#ifndef _CACHE_H
#define _CACHE_H

#include <stdbool.h>

#include "exfat.h"
#include "list.h"

struct cache {
	struct super_block *sb;
	void *data;
	off_t offset;
	size_t count;
	bool dirty;
	int (*read)(struct super_block *, void *, off_t, size_t);
	int (*write)(struct super_block *, void *, off_t, size_t);
	int (*print)(struct super_block *, off_t, size_t);
	struct list_head *next;
};

struct cache *create_cluster_cache(struct super_block *sb, __le32 index, size_t count);
struct cache *create_sector_cache(struct super_block *sb, __le32 index, size_t count);
struct cache *search_cache(struct super_block *sb, struct list_head *head, __le32 index);
int remove_cache(struct super_block *sb, struct list_head *prev);
int remove_cache_list(struct super_block *sb, struct list_head *head);

#endif /*_CACHE_H */
