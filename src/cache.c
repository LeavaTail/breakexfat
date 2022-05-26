// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#include "exfat.h"
#include "breakexfat.h"
#include "list.h"

/**
 * @brief create cache
 * @param [in] sb    Filesystem metadata
 * @param [in] index Start index
 * @param [in] count The number of sectors
 *
 * @return created cache (or NULL)
 */
static struct cache *create_cache(struct super_block *sb, uint32_t index, size_t count)
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
 * @brief create cache for cluster
 * @param [in] sb    Filesystem metadata
 * @param [in] index Start cluster index
 * @param [in] count The number of cluster
 *
 * @return created cache (or NULL)
 */
struct cache *create_cluster_cache(struct super_block *sb, uint32_t index, size_t count)
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
 * @brief create cache for sector
 * @param [in] sb    Filesystem metadata
 * @param [in] index Start sector index
 * @param [in] count The number of sectors
 *
 * @return created cache (or NULL)
 */
struct cache *create_sector_cache(struct super_block *sb, uint32_t index, size_t count)
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
 * @brief Search cache from list
 * @param [in] sb    Filesystem metadata
 * @param [in] index Start sector index
 * @param [in] list  Searched list
 *
 * @return target cache (or NULL)
 */
static struct cache *search_cache(struct super_block *sb, struct list_head *head, uint32_t index)
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
 * @brief Get cluster cache
 * @param [in] sb    Filesystem metadata
 * @param [in] index Start sector index
 *
 * @return target cache (or NULL)
 */
struct cache *get_cluster_cache(struct super_block *sb, uint32_t index)
{
	struct cache *cache;

	if ((cache = search_cache(sb, sb->cluster_list, index)) != NULL)
		return cache;

	cache = create_cluster_cache(sb, index, 1);
	if (!cache)
		return NULL;

	list_add(sb->cluster_list, cache);

	return cache;
}

/**
 * @brief Get sector cache
 * @param [in] sb    Filesystem metadata
 * @param [in] index Start cluster index
 *
 * @return target cache (or NULL)
 */
struct cache *get_sector_cache(struct super_block *sb, uint32_t index)
{
	struct cache *cache;

	if ((cache = search_cache(sb, sb->sector_list, index)) != NULL)
		return cache;

	cache = create_sector_cache(sb, index, 1);
	if (!cache)
		return NULL;

	list_add(sb->sector_list, cache);

	return cache;
}

/**
 * @brief remove node from list
 * @param [in] sb    Filesystem metadata
 * @param [in] prev  removed previous node
 *
 * @return success or failed
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
 * @brief remove list
 * @param [in] sb    Filesystem metadata
 * @param [in] head  removed head of list
 *
 * @return success or failed
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
