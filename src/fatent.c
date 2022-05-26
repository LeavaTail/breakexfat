// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#include "exfat.h"
#include "breakexfat.h"
#include "endian.h"

/**
 * Active FAT(1st or 2nd)
 */
static int active_fat = 0;

/**
 * @brief Update active FAT
 * @param [in] sb    Filesystem metadata
 * @param [in] index The number of FATs
 *
 * @return success or failed
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
 * @brief  Get FAT entry
 * @param [in]  sb    Filesystem metadata
 * @param [in]  clu   index of the cluster want to check
 * @param [out] entry FAT entry
 *
 * @retval 0 success
 * @retval Negative failed
 */
int get_fat_entry(struct super_block *sb, uint32_t clu, uint32_t *entry)
{
	size_t entry_per_sector = sb->sector_size / sizeof(uint32_t);
	uint32_t index = (clu) % entry_per_sector;
	uint32_t offset = sb->fat_offset;
	__le32 *fat;
	struct cache *cache;

	offset += sb->fat_length * active_fat;

	cache = get_sector_cache(sb, offset);
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
 * @brief Update FAT Entry
 * @param [in] sb     Filesystem metadata
 * @param [in] clu    index of the cluster want to check
 * @param [out] entry set bitmap 0 or unset bitmap 1
 *
 * @retval 0 success
 * @retval Negative failed
 */
int set_fat_entry(struct super_block *sb, uint32_t clu, uint32_t entry)
{
	size_t entry_per_sector = sb->sector_size / sizeof(uint32_t);
	uint32_t index = (clu) % entry_per_sector;
	uint32_t offset = sb->fat_offset;
	__le32 *fat;
	struct cache *cache;

	offset += sb->fat_length * active_fat;

	cache = get_sector_cache(sb, offset);
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

/**
 * @brief  Get next cluster in FAT chain
 * @param [in]  sb     Filesystem metadata
 * @param [in]  inode  target file/directory
 * @param [in]  clu    index of the cluster want to check
 * @param [out] entry FAT entry
 *
 * @retval 0 success
 * @retval Negative failed
 */
int get_next_cluster(struct super_block *sb, struct inode *inode, uint32_t clu, uint32_t *entry)
{
	if (inode->flags & NOFATCHAIN)
		*entry = clu + 1;
	else
		return get_fat_entry(sb, clu, entry);

	if (validate_cluster(sb, *entry)) {
		pr_err("Internal Error: Cluster %08x is invalid.\n", clu);
		return -EINVAL;
	}

	return 0;
}

