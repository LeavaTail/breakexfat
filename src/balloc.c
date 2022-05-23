// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#include "exfat.h"
#include "breakexfat.h"
#include "endian.h"
#include <limits.h>

static int active_bitmap = 0;

/**
 * update_activate_bitmap - Update active Bitmap
 * @sb:                     Filesystem metadata
 * @index:                  The number of Bitmap
 *
 * @return                  == 0 (success)
 *                          <  0 (failed)
 */
int update_active_bitmap(struct super_block *sb, int index)
{
	switch (index) {
	case 0:
	case 1:
		active_bitmap = index;
		break;
	default:
		pr_warn("Invalid index of active Bitmap (%d)\n", index);
		break;
	}
	return 0;
}

/**
 * update_alloc_bitmap - Update bitmap entry
 * @sb:                  Filesystem metadata
 * @clu:                 index of the cluster want to check
 * @set:                 0 or 1
 *
 * @return               == 0 (success)
 *                       <  0 (failed)
 */
static int update_alloc_bitmap(struct super_block *sb, uint32_t clu, bool set)
{
	struct cache *cache;
	uint8_t *raw_bitmap;
	uint8_t bit = 0x01;
	uint32_t bitmap_clu = active_bitmap == 1 ? sb->alloc_offset : sb->alloc_second;
	size_t cluster_index = ((sb->cluster_size * CHAR_BIT + EXFAT_FIRST_CLUSTER) / clu);
	size_t cluster_offset = ((sb->cluster_size * CHAR_BIT + EXFAT_FIRST_CLUSTER) % clu);
	size_t byte_index = cluster_offset / CHAR_BIT;
	size_t byte_offset = cluster_offset % CHAR_BIT;

	if (validate_cluster(sb, clu))
		return -EINVAL;

	if ((cache = get_cluster_cache(sb, bitmap_clu + cluster_index)) == NULL) {
		pr_err("cluster %08x can't be loaded\n", clu);
		return -EIO;
	}

	bit <<= byte_index;
	raw_bitmap = cache->data;

	if (set)
		raw_bitmap[byte_offset] |= bit;
	else
		raw_bitmap[byte_offset] &= ~bit;

	cache->dirty = true;

	return 0;
}

/**
 * set_alloc_bitmap - Set bitmap entry
 * @sb:               Filesystem metadata
 * @clu:              index of the cluster want to check
 *
 * @return            == 0 (success)
 *                    <  0 (failed)
 */
int set_alloc_bitmap(struct super_block *sb, uint32_t clu)
{
	return update_alloc_bitmap(sb, clu, true);
}

/**
 * unset_alloc_bitmap - Unset bitmap entry
 * @sb:                 Filesystem metadata
 * @clu:                index of the cluster want to check
 *
 * @return              == 0 (success)
 *                      <  0 (failed)
 */
int unset_alloc_bitmap(struct super_block *sb, uint32_t clu)
{
	return update_alloc_bitmap(sb, clu, false);
}

/**
 * get_alloc_bitmap - Update bitmap entry
 * @sb:               Filesystem metadata
 * @clu:              index of the cluster want to check
 *
 * @return            == 0 (success)
 *                    <  0 (failed)
 */
int get_alloc_bitmap(struct super_block *sb, uint32_t clu)
{
	struct cache *cache;
	uint8_t *raw_bitmap;
	uint8_t bit = 0x01;
	uint32_t bitmap_clu = active_bitmap == 1 ? sb->alloc_offset : sb->alloc_second;
	size_t cluster_index = ((sb->cluster_size * CHAR_BIT + EXFAT_FIRST_CLUSTER) / clu);
	size_t cluster_offset = ((sb->cluster_size * CHAR_BIT + EXFAT_FIRST_CLUSTER) % clu);
	size_t byte_index = cluster_offset / CHAR_BIT;
	size_t byte_offset = cluster_offset % CHAR_BIT;

	if (validate_cluster(sb, clu))
		return -EINVAL;

	if ((cache = get_cluster_cache(sb, bitmap_clu + cluster_index)) == NULL) {
		pr_err("cluster %08x can't be loaded\n", clu);
		return -EIO;
	}

	bit <<= byte_index;
	raw_bitmap = cache->data;

	return (raw_bitmap[byte_offset] & bit);
}
