// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "exfat.h"
#include "breakexfat.h"
#include "endian.h"
#include "cache.h"

/**
 * initialize_super - initialize super_block
 * @sb:               Filesystem metadata
 * @name:             Target exFAT filesystem image path
 *
 * @return            == 0 (success)
 *                    <  0 (failed)
 */
int initialize_super(struct super_block *sb, const char *name)
{
	struct stat stat;

	if (!sb)
		return -EINVAL;

	sb->sector_size = 512;

	if ((sb->fd = open(name, O_RDWR)) < 0) {
		pr_err("open: %s\n", strerror(errno));
		return -errno;
	}

	if (fstat(sb->fd, &stat) < 0) {
		pr_err("stat: %s\n", strerror(errno));
		close(sb->fd);
		return -errno;
	}
	sb->total_size = stat.st_size;

	sb->sector_list = NULL;
	sb->cluster_list = NULL;

	return 0;
}

/**
 * finalize_super - finalize_super super_block
 * @sb:             Filesystem metadata
 *
 * @return          == 0 (success)
 *                  <  0 (failed)
 */
int finalize_super(struct super_block *sb)
{
	if (!sb)
		return -EINVAL;

	remove_cache_list(sb, sb->sector_list);
	remove_cache_list(sb, sb->cluster_list);

	if (sb->fd)
		close(sb->fd);

	return 0;
}

/**
 * read_boot_sector - read boot sector in exFAT
 * @sb:               Filesystem metadata
 *
 * @return            == 0 (success)
 *                    <  0 (failed)
 */
int read_boot_sector(struct super_block *sb)
{
	int ret = 0;
	struct boot_sector *boot;

	if ((boot = malloc(sizeof(struct boot_sector))) == NULL)
		return -ENOMEM;

	get_sector(sb, boot, 0, 1);

	if (verify_boot_sector(sb, boot)) {
		ret = -EINVAL;
		goto out;
	}

	sb->part_offset = le64_to_cpu(boot->partition_offset);
	sb->vol_size = le32_to_cpu(boot->vol_length);
	sb->sector_size = 1 << boot->sect_size_bits;
	sb->cluster_size = 1 << (boot->sect_size_bits + boot->sect_per_clus_bits);
	sb->cluster_count = le32_to_cpu(boot->clu_count);
	sb->fat_offset = le32_to_cpu(boot->fat_offset);
	sb->fat_length = le32_to_cpu(boot->fat_length);
	sb->num_fats = boot->num_fats;
	sb->heap_offset = le32_to_cpu(boot->clu_offset);
	sb->root_offset = le32_to_cpu(boot->root_cluster);

	sb->sector_list = init_list_head(create_sector_cache(sb, 0, 1));
	sb->cluster_list = init_list_head(create_cluster_cache(sb, sb->root_offset, 1));
out:
	free(boot);

	return ret;
}

/**
 * verify_boot_sector - verify boot sector
 * @sb:                 Filesystem metadata
 *
 * @return              == 0 (success)
 *                      <  0 (failed)
 */
int verify_boot_sector(struct super_block *sb, struct boot_sector *b)
{
	if ((b->jmp_boot[0] != 0xEB) ||
		(b->jmp_boot[1] != 0x76) ||
		(b->jmp_boot[2] != 0x90)) {
		pr_err("invalid JumpBoot: 0x%x%x%x\n",
				b->jmp_boot[0], b->jmp_boot[1], b->jmp_boot[2]);
		return -EINVAL;
	}

	if (memcmp(b->fs_name, "EXFAT   ", BOOTSEC_FSNAME_LEN)) {
		pr_err("invalid FileSystemName: \"%8s\"\n", b->fs_name);
		return -EINVAL;
	}


	if (le16_to_cpu(b->signature != 0xAA55)) {
		pr_err("invalid boot record signature");
		return -EINVAL;
	}

	return 0;
}
