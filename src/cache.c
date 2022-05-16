// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#include "exfat.h"
#include "breakexfat.h"
#include "cache.h"
#include "list.h"

/**
 * create_cache - create cache
 * @sb:           Filesystem metadata
 * @index:        Start bytes
 * @count:        The number of sectors
 *
 * @return       != NULL (success)
 *               == NULL (failed)
 */
static struct cache *create_cache(struct super_block *sb, __le32 index, size_t count)
{
	struct cache *cache;

	if ((cache = malloc(sizeof(struct cache))) == NULL) {
		pr_err("malloc: %s\n", strerror(errno));
		return NULL;
	}

	cache->sb = sb;
	cache->data = NULL;
	cache->offset = index;
	cache->count = count;
	cache->dirty = false;
	cache->read = NULL;
	cache->write = NULL;
	cache->print = NULL;
	cache->next = NULL;
	return cache;
}

/**
 * create_cluster_cache - create cache for cluster
 * @sb:                   Filesystem metadata
 * @index:                Start bytes
 * @count:                The number of sectors
 *
 * @return                != NULL (success)
 *                        == NULL (failed)
 */
struct cache *create_cluster_cache(struct super_block *sb, __le32 index, size_t count)
{
	void *data;
	struct cache *clu;

	if ((clu = create_cache(sb, index, count)) == NULL)
		goto err;

	if ((data = malloc(sb->cluster_size * count)) == NULL)
		goto free_clu;

	if (get_cluster(sb, data, index, count))
		goto free_data;

	clu->data = data;
	clu->read = get_cluster;
	clu->write = set_cluster;
	clu->print = print_cluster;
	pr_debug("Create cache for cluster#%x (nums: %lu)\n", index, count);

	return clu;

free_data:
	free(data);
free_clu:
	free(clu);
err:
	return NULL;
}

/**
 * create_sector_cache - create cache for sector
 * @sb:                  Filesystem metadata
 * @index:               Start bytes
 * @count:               The number of sectors
 *
 * @return               != NULL (success)
 *                       == NULL (failed)
 */
struct cache *create_sector_cache(struct super_block *sb, __le32 index, size_t count)
{
	void *data;
	struct cache *clu;

	if ((clu = create_cache(sb, index, count)) == NULL)
		goto err;

	if ((data = malloc(sb->sector_size * count)) == NULL)
		goto free_clu;

	if (get_sector(sb, data, index, count))
		goto free_data;

	clu->data = data;
	clu->read = get_sector;
	clu->write = set_sector;
	clu->print = print_sector;
	pr_debug("Create cache for sector#%x (nums: %lu)\n", index, count);

	return clu;

free_data:
	free(data);
free_clu:
	free(clu);
err:
	return NULL;
}

/**
 * search_cache - search cache from list
 * @sb:           Filesystem metadata
 * @index:        Start bytes
 * @list:         Searched list
 *
 * @return        != NULL (success)
 *                == NULL (failed)
 */
struct cache *search_cache(struct super_block *sb, struct list_head *head, __le32 index)
{
	struct list_head *node;
	struct cache *cache;

	for (node = head; node != NULL; node = node->next) {
		cache = node->data;
		if (cache->offset == index)
			return cache;
	}

	return NULL;
}

/**
 * remove_cache - remove node from list
 * @sb:           Filesystem metadata
 * @prev:         removed previous node
 *
 * @return        == 0 (success)
 */
int remove_cache(struct super_block *sb, struct list_head *prev)
{
	struct list_head *node;
	struct cache *cache;

	if (!prev)
		return 0;

	if ((node = prev->next) == NULL)
		node = prev;

	cache = node->data;
	if (cache->dirty)
		cache->write(sb, cache->data, cache->offset, cache->count);

	free(cache->data);
	free(cache);

	list_del(prev);

	return 0;
}

/**
 * remove_cache_list - remove list
 * @sb:                Filesystem metadata
 * @head:              removed head of list
 *
 * @return             == 0 (success)
 */
int remove_cache_list(struct super_block *sb, struct list_head *head)
{
	if (!head)
		return 0;

	while (head->next != NULL) {
		remove_cache(sb, head);
	}
	remove_cache(sb, head);

	return 0;
}
