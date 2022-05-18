// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#include "exfat.h"
#include "breakexfat.h"
#include "endian.h"

static int active_fat = 0;

/**
 * update_activate_fat - Update active FAT
 * @sb:                  Filesystem metadata
 * @index:               The number of FATs
 *
 * @return               == 0 (success)
 *                       <  0 (failed)
 */
int update_active_fat(struct super_block *sb, int index)
{
	switch (index) {
	case 0:
	case 1:
		active_fat = index;
		break;
	default:
		pr_warn("Invalid index of active FAT (%d)\n", index);
		break;
	}
	return 0;
}

/**
 * get_fat_entry - Get FAT entry
 * @sb:            Filesystem metadata
 * @clu:           index of the cluster want to check
 * @entry:         any cluster index (Output)
 *
 * @return         == 0 (success)
 *                 <  0 (failed)
 */
int get_fat_entry(struct super_block *sb, uint32_t clu, uint32_t *entry)
{
	size_t entry_per_sector = sb->sector_size / sizeof(uint32_t);
	uint32_t index = (clu) % entry_per_sector;
	uint32_t offset = sb->fat_offset;
	__le32 *fat;
	struct cache *cache;

	offset += sb->fat_length * active_fat;

	cache = search_cache(sb, sb->sector_list, offset);
	fat = cache->data;

	if (validate_cluster(sb, clu))  {
		pr_err("Internal Error: Cluster %08x is invalid.\n", clu);
		return -EINVAL;
	}

	*entry = cpu_to_le32(fat[index]);
	pr_debug("Get: FAT[%08x] %08x\n", clu, *entry);

	return 0;
}

/**
 * set_fat_entry - Update FAT Entry
 * @sb:            Filesystem metadata
 * @clu:           index of the cluster want to check
 * @entry:         any cluster index
 *
 * @return         == 0 (success)
 *                 <  0 (failed)
 */
int set_fat_entry(struct super_block *sb, uint32_t clu, uint32_t entry)
{
	size_t entry_per_sector = sb->sector_size / sizeof(uint32_t);
	uint32_t index = (clu) % entry_per_sector;
	uint32_t offset = sb->fat_offset;
	__le32 *fat;
	struct cache *cache;

	offset += sb->fat_length * active_fat;

	cache = search_cache(sb, sb->sector_list, offset);
	fat = cache->data;

	if (validate_cluster(sb, clu) || (validate_cluster(sb, entry))) {
		pr_err("Internal Error: Cluster %08x,%08x is invalid.\n", clu, entry);
		return -EINVAL;
	}

	fat[index] = cpu_to_le32(entry);
	cache->dirty = true;
	pr_debug("Set: FAT[%08x] %08x\n", clu, fat[index]);

	return 0;
}
