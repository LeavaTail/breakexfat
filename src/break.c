// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */

#include "exfat.h"
#include "breakexfat.h"

/**
 * break exFAT image
 */
struct break_pattern_information {
	char *name;
	bool choice;
	int type;
	int (*func)(struct super_block *, struct cache *, int);
};

static int break_boot_jumpboot(struct super_block *sb, struct cache *cache, int type);
static int break_boot_fsname(struct super_block *sb, struct cache *cache, int type);
static int break_boot_zero(struct super_block *sb, struct cache *cache, int type);
static int break_boot_partoff(struct super_block *sb, struct cache *cache, int type);

/* Array for break pattern */
static struct break_pattern_information break_boot_info[] =
{
	{"Invalid JumpBoot", false, 0, break_boot_jumpboot},
	{"Invalid FileSystemName", false, 0, break_boot_fsname},
	{"Not zero in MustBeZero", false, 0, break_boot_zero},
	{"Invalid PartitionOffset", false, 0, break_boot_partoff},
};

/**
 * enable_break_pattern - Enable break pattern
 * @sb:                   Filesystem metadata
 * @index:                index of break_info
 *
 * @return                == 0 (success)
 *                        <  0 (failed)
 */
int enable_break_pattern(struct super_block *sb, unsigned int index)
{
	if (sizeof(break_boot_info)/sizeof(break_boot_info[0]) < index)
		return -EINVAL;

	break_boot_info[index].choice = true;

	return 0;
}

/**
 * disable_break_pattern - Disable break pattern
 * @sb:                    Filesystem metadata
 * @index:                 index of break_info
 *
 * @return                 == 0 (success)
 *                         <  0 (failed)
 */
int disable_break_pattern(struct super_block *sb, unsigned int index)
{
	if (sizeof(break_boot_info)/sizeof(break_boot_info[0]) < index)
		return -EINVAL;

	break_boot_info[index].choice = false;

	return 0;
}
/**
 * break_boot_jumpboot - break jumoboot in boot sector
 * @sb:                  Filesystem metadata
 * @cache:               boot sector cache
 * @type:                break pattern
 *
 * @return               == 0 (success)
 *                       <  0 (failed)
 */
static int break_boot_jumpboot(struct super_block *sb, struct cache *cache, int type)
{
	struct boot_sector *boot = cache->data;

	boot->jmp_boot[0] = 0xFF;
	boot->jmp_boot[1] = 0xFF;
	boot->jmp_boot[2] = 0xFF;
	cache->dirty = true;

	return 0;
}

/**
 * break_boot_fsname - break FileSystemName in boot sector
 * @sb:                Filesystem metadata
 * @cache:             boot sector cache
 * @type:              break pattern
 *
 * @return             == 0 (success)
 *                     <  0 (failed)
 */
static int break_boot_fsname(struct super_block *sb, struct cache *cache, int type)
{
	struct boot_sector *boot;

	boot = cache->data;
	memcpy(boot->fs_name, "        ", BOOTSEC_FSNAME_LEN);
	cache->dirty = true;

	return 0;
}

/**
 * break_boot_zero - break MustBeZero in boot sector
 * @sb:              Filesystem metadata
 * @cache:           boot sector cache
 * @type:            break pattern
 *
 * @return           == 0 (success)
 *                   <  0 (failed)
 */
static int break_boot_zero(struct super_block *sb, struct cache *cache, int type)
{
	int i;
	struct boot_sector *boot;

	boot = cache->data;
	for (i = 0; i < BOOTSEC_ZERO_LEN; i++)
		boot->must_be_zero[i] = 0xff;
	cache->dirty = true;

	return 0;
}

/**
 * break_boot_partoff - break PartitionOffset in boot sector
 * @sb:                 Filesystem metadata
 * @cache:              boot sector cache
 * @type:               break pattern
 *
 * @return              == 0 (success)
 *                      <  0 (failed)
 */
static int break_boot_partoff(struct super_block *sb, struct cache *cache, int type)
{
	struct boot_sector *boot;

	boot = cache->data;
	boot->partition_offset = ULONG_MAX;
	cache->dirty = true;

	return 0;
}

