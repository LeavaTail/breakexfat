// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#include "exfat.h"
#include "breakexfat.h"
#include "endian.h"
#include <limits.h>

/**
 * Active Allocation Bitmap(1st or 2nd)
 */
static int active_bitmap = 0;

/**
 * @brief Update active Bitmap
 * @param [in] sb    Filesystem metadata
 * @param [in] index The number of Bitmap
 *
 * @return success or failed
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
 * @brief Update bitmap entry
 * @param [in] sb  Filesystem metadata
 * @param [in] clu index of the cluster want to check
 * @param [in] set set bitmap 0 or unset bitmap 1
 *
 * @retval 0 success
 * @retval Negative failed
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
 * @brief Set bitmap entry
 * @param [in] sb  Filesystem metadata
 * @param [in] clu index of the cluster want to check
 *
 * @retval 0 success
 * @retval Negative failed
 */
int set_alloc_bitmap(struct super_block *sb, uint32_t clu)
{
	return update_alloc_bitmap(sb, clu, true);
}

/**
 * @brief Unset bitmap entry
 * @param [in] sb  Filesystem metadata
 * @param [in] clu index of the cluster want to check
 *
 * @retval 0 success
 * @retval Negative failed
 */
int unset_alloc_bitmap(struct super_block *sb, uint32_t clu)
{
	return update_alloc_bitmap(sb, clu, false);
}

/**
 * @brief Update bitmap entry
 * @param [in] sb   Filesystem metadata
 * @param [in] clu  index of the cluster want to check
 *
 * @retval 0 success
 * @retval Negative failed
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
