// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */

#include "exfat.h"
#include "breakexfat.h"

/**
 * break exFAT image information
 */
struct break_pattern_information {
	char *name;
	bool choice;
	int type;
	int (*func)(struct super_block *, int);
};

static int break_boot_jumpboot(struct super_block *sb, int type);
static int break_boot_fsname(struct super_block *sb, int type);
static int break_boot_zero(struct super_block *sb, int type);
static int break_boot_partoff(struct super_block *sb, int type);
static int break_boot_vollen(struct super_block *sb, int type);
static int break_boot_fatoff(struct super_block *sb, int type);
static int break_boot_fatlen(struct super_block *sb, int type);
static int break_boot_cluoff(struct super_block *sb, int type);
static int break_boot_clucount(struct super_block *sb, int type);
static int break_boot_rootclu(struct super_block *sb, int type);
static int break_boot_fsrev(struct super_block *sb, int type);
static int break_boot_volflags(struct super_block *sb, int type);
static int break_boot_bps(struct super_block *sb, int type);
static int break_boot_spc(struct super_block *sb, int type);
static int break_boot_numfats(struct super_block *sb, int type);
static int break_boot_inuse(struct super_block *sb, int type);
static int break_boot_bootcode(struct super_block *sb, int type);

//! Array for break pattern information
static struct break_pattern_information break_boot_info[] =
{
	{"Invalid JumpBoot", false, 0, break_boot_jumpboot},
	{"Invalid FileSystemName", false, 0, break_boot_fsname},
	{"Not zero in MustBeZero", false, 0, break_boot_zero},
	{"Invalid PartitionOffset", false, 0, break_boot_partoff},
	{"Too small VolumeLength", false, 0, break_boot_vollen},
	{"Too small FatOffset", false, 0, break_boot_fatoff},
	{"Too large FatOffset", false, 1, break_boot_fatoff},
	{"Too small FatLength", false, 0, break_boot_fatlen},
	{"Too large FatLength", false, 1, break_boot_fatlen},
	{"Too small ClusterHeapOffset", false, 0, break_boot_cluoff},
	{"Too large ClusterHeapOffset", false, 1, break_boot_cluoff},
	{"Too small ClusterCount", false, 0, break_boot_clucount},
	{"Too large ClusterCount", false, 1, break_boot_clucount},
	{"Too small FirstClusterOfRootDirectory", false, 0, break_boot_rootclu},
	{"Too large FirstClusterOfRootDirectory", false, 1, break_boot_rootclu},
	{"Invalid FirstClusterOfRootDirectory", false, 2, break_boot_rootclu},
	{"Too small FileSystemRevision", false, 0, break_boot_fsrev},
	{"Too large FileSystemRevision", false, 1, break_boot_fsrev},
	{"Set ActiveFat in VolumeFlags", false, 0, break_boot_volflags},
	{"Set VolumeDirty in VolumeFlags", false, 1, break_boot_volflags},
	{"Set MediaFailure in VolumeFlags", false, 2, break_boot_volflags},
	{"Set ClearToZero in VolumeFlags", false, 3, break_boot_volflags},
	{"Too small BytesPerSectorShift", false, 0, break_boot_bps},
	{"Too large BytesPerSectorShift", false, 1, break_boot_bps},
	{"Too large SectorPerClusterShift", false, 0, break_boot_spc},
	{"Too small NumberOfFats", false, 0, break_boot_numfats},
	{"Too large NumberOfFats", false, 1, break_boot_numfats},
	{"Too large PercentInUse", false, 0, break_boot_inuse},
	{"Invalid BootCode", false, 0, break_boot_bootcode},
};

/**
 * @brief Enable break pattern
 * @param [in] sb    Filesystem metadata
 * @param [in] index index of break_info
 *
 * @retval 0 success
 * @retval Negative failed
 */
int enable_break_pattern(struct super_block *sb, unsigned int index)
{
	if (sizeof(break_boot_info)/sizeof(break_boot_info[0]) < index)
		return -EINVAL;

	break_boot_info[index].choice = true;

	return 0;
}

/**
 * @brief Disable break pattern
 * @param [in] sb    Filesystem metadata
 * @param [in] index index of break_info
 *
 * @retval 0 success
 * @retval Negative failed
 */
int disable_break_pattern(struct super_block *sb, unsigned int index)
{
	if (sizeof(break_boot_info)/sizeof(break_boot_info[0]) < index)
		return -EINVAL;

	break_boot_info[index].choice = false;

	return 0;
}

/**
 * @brief Enable All break pattern
 * @param [in] sb    Filesystem metadata
 *
 * @retval 0 success
 * @retval Negative failed
 */
int enable_break_all_pattern(struct super_block *sb)
{
	int i;

	for (i = 0; i < sizeof(break_boot_info)/sizeof(break_boot_info[0]); i++)
		if (enable_break_pattern(sb, i))
			return -EINVAL;

	return 0;
}

/**
 * @brief Run break exFAT filesystem image
 * @param [in] sb    Filesystem metadata
 *
 * @retval 0 success
 * @retval Negative failed
 */
int run_break(struct super_block *sb)
{
	int i;
	struct break_pattern_information tmp;

	for (i = 0; i < sizeof(break_boot_info) / sizeof(break_boot_info[0]); i++) {
		tmp = break_boot_info[i];
		if (tmp.choice) {
			pr_msg("Break pattern: %s\n", tmp.name);
			tmp.func(sb, tmp.choice);
		}
	}

	return 0;
}

/**
 * @brief break jumoboot in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_jumpboot(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	boot->jmp_boot[0] = 0xFF;
	boot->jmp_boot[1] = 0xFF;
	boot->jmp_boot[2] = 0xFF;
	cache->dirty = true;

	return 0;
}

/**
 * @bried break FileSystemName in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_fsname(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	memcpy(boot->fs_name, "        ", BOOTSEC_FSNAME_LEN);
	cache->dirty = true;

	return 0;
}

/**
 * @brief break MustBeZero in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_zero(struct super_block *sb, int type)
{
	int i;
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	for (i = 0; i < BOOTSEC_ZERO_LEN; i++)
		boot->must_be_zero[i] = 0xff;
	cache->dirty = true;

	return 0;
}

/**
 * @brief break PartitionOffset in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_partoff(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	boot->partition_offset = ULONG_MAX;
	cache->dirty = true;

	return 0;
}

/**
 * @brief break VolumeLength in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_vollen(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	boot->vol_length = (power2(20) / sb->sector_size) - 1;
	cache->dirty = true;

	return 0;
}

/**
 * @brief break FatOffset in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_fatoff(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	switch (type) {
		case 0:
			boot->fat_offset = 24 - 1;
			break;
		case 1:
			boot->fat_offset = sb->heap_offset - (sb->fat_length * sb->num_fats) + 1;
			break;
		default:
			return -EINVAL;
	}
	cache->dirty = true;

	return 0;
}

/**
 * @brief break FatLength in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_fatlen(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;
	uint64_t clu_nums;

	clu_nums = sb->cluster_count + EXFAT_FIRST_CLUSTER;
	switch (type) {
		case 0:
			boot->fat_length = ROUNDUP(clu_nums * power2(2), sb->sector_size) - 1;
			break;
		case 1:
			boot->fat_length = (sb->heap_offset - sb->fat_offset) / sb->num_fats + 1;
			break;
		default:
			return -EINVAL;
	}
	cache->dirty = true;

	return 0;
}

/**
 * @brief break ClusterHeapOffset in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_cluoff(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	switch (type) {
			case 0:
			boot->clu_offset = (sb->fat_offset + sb->fat_length * sb->num_fats) - 1;
			break;
		case 1:
			boot->clu_offset = UINT32_MAX;
			break;
		default:
			return -EINVAL;
	}
	cache->dirty = true;

	return 0;
}

/**
 * @brief break ClusterCount in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_clucount(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	switch (type) {
			case 0:
			boot->clu_count = (sb->vol_size + sb->heap_offset) / (sb->cluster_size / sb->sector_size) - 1;
			break;
		case 1:
			boot->clu_offset = power2(32) - 11 + 1;
			break;
		default:
			return -EINVAL;
	}
	cache->dirty = true;

	return 0;
}

/**
 * @brief break FirstClusterOfRootDirectory in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_rootclu(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	switch (type) {
		case 0:
			boot->root_cluster = 0;
			break;
		case 1:
			boot->root_cluster = sb->cluster_count + 1 + 1;
			break;
		case 2:
			boot->root_cluster++;
			break;
		default:
			return -EINVAL;
	}
	cache->dirty = true;

	return 0;
}

/**
 * @brief break FileSystemRevision in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_fsrev(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	switch (type) {
		case 0:
			boot->fs_revision[0] = 0x00;
			boot->fs_revision[1] = 0x00;
			break;
		case 1:
			boot->fs_revision[0] = 0x99;
			boot->fs_revision[1] = 0x99;
			break;
		default:
			return -EINVAL;
	}
	cache->dirty = true;

	return 0;
}

/**
 * @brief break VolumeFlags in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_volflags(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	switch (type) {
		case 0:
			boot->vol_flags |= BIT(0);
			break;
		case 1:
			boot->vol_flags |= BIT(1);
			break;
		case 2:
			boot->vol_flags |= BIT(2);
			break;
		case 3:
			boot->vol_flags |= BIT(3);
			break;
		default:
			return -EINVAL;
	}
	cache->dirty = true;

	return 0;
}

/**
 * @brief break BytesPerSectorShift in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_bps(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	switch (type) {
		case 0:
			boot->sect_size_bits = log_2(EXFAT_SECTOR_MIN) - 1;
			break;
		case 1:
			boot->sect_size_bits = log_2(EXFAT_SECTOR_MAX) + 1;
			break;
		default:
			return -EINVAL;
	}
	cache->dirty = true;

	return 0;
}

/**
 * @brief break SectorPerClusterShift in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_spc(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	boot->sect_per_clus_bits = log_2(EXFAT_CLUSTER_MAX) - boot->sect_size_bits + 1;
	cache->dirty = true;

	return 0;
}

/**
 * @brief break NumberOfFats in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_numfats(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	switch (type) {
		case 0:
			boot->num_fats = 0;
			break;
		case 1:
			boot->num_fats = 3;
			break;
		default:
			return -EINVAL;
	}
	cache->dirty = true;

	return 0;
}

/**
 * @brief break PercentInUse in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_inuse(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	boot->percent_in_use = 100 + 1;
	cache->dirty = true;

	return 0;
}

/**
 * @brief break BootCode in boot sector
 * @param [in] sb    Filesystem metadata
 * @param [in] type  break pattern
 *
 * @retval 0 success
 * @retval Negative failed
 */
static int break_boot_bootcode(struct super_block *sb, int type)
{
	struct cache *cache = get_sector_cache(sb, 0);
	struct boot_sector *boot = cache->data;

	memset(boot->boot_code, 0, 390);
	cache->dirty = true;

	return 0;
}

