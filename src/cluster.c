// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */

#include <unistd.h>
#include <ctype.h>

#include "exfat.h"
#include "breakexfat.h"

/**
 * get_sector - Get Raw-Data from any sector
 * @sb:         Filesystem metadata
 * @data:       Sector raw data (Output)
 * @index:      Start bytes
 * @count:      The number of sectors
 *
 * @return       == 0 (success)
 *               <  0 (failed)
 *
 * NOTE: Need to allocate @data before call it.
 */
int get_sector(struct super_block *sb, void *data, off_t index, size_t count)
{
	pr_debug("Get: Sector from 0x%lx to 0x%lx\n", index, index + (count * sb->sector_size) - 1);

	if ((pread(sb->fd, data, sb->sector_size * count, index)) < 0) {
		pr_err("read: %s\n", strerror(errno));
		return -errno;
	}

	return 0;
}

/**
 * set_sector - Set Raw-Data from any sector
 * @sb:         Filesystem metadata
 * @data:       Sector raw data
 * @index:      Start bytes
 * @count:      The number of sectors
 *
 * @return      == 0 (success)
 *              <  0 (failed)
 *
 * NOTE: Need to allocate @data before call it.
 */
int set_sector(struct super_block *sb, void *data, off_t index, size_t count)
{
	pr_debug("Set: Sector from 0x%lx to 0x%lx\n", index, index + (count * sb->sector_size) - 1);

	if ((pwrite(sb->fd, data, sb->sector_size * count, index)) < 0) {
		pr_err("write: %s\n", strerror(errno));
		return -errno;
	}

	return 0;
}

/**
 * print_sector -  Print Raw-Data from any sector
 * @sb:            Filesystem metadata
 * @index:         Start cluster index
 * @count:         The number of clusters
 *
 * @return         == 0 (success)
 *                 <  0 (failed)
 */
int print_sector(struct super_block *sb, off_t index, size_t count)
{
	int i;
	int ret = 0;
	size_t line, byte = 0;
	size_t size = sb->sector_size;
	size_t lines = size / 0x10;
	void *data;

	if ((data = malloc(size)) == NULL)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		ret = get_sector(sb, data, index + i, 1);
		if (ret < 0)
			goto out;

		for (line = 0; line < lines; line++) {
			pr_msg("%08lX:  ", line * 0x10);
			for (byte = 0; byte < 0x10; byte++) {
				pr_msg("%02X ", ((unsigned char *)data)[line * 0x10 + byte]);
			}
			putchar(' ');
			for (byte = 0; byte < 0x10; byte++) {
				char ch = ((unsigned char *)data)[line * 0x10 + byte];
				pr_msg("%c", isprint(ch) ? ch : '.');
			}
			pr_msg("\n");
		}
	}
out:
	free(data);
	
	return ret;
}
/**
 * get_cluster -  Get Raw-Data from any cluster
 * @sb:           Filesystem metadata
 * @data:         cluster raw data (Output)
 * @index:        Start cluster index
 * @count:        The number of clusters
 *
 * @return        == 0 (success)
 *                <  0 (failed)
 *
 * NOTE: Need to allocate @data before call it.
 */
int get_cluster(struct super_block *sb, void *data, off_t index, size_t count)
{
	size_t clu_per_sec = sb->cluster_size / sb->sector_size;
	off_t heap_start = sb->heap_offset * sb->sector_size;

	if (index < EXFAT_FIRST_CLUSTER || index + count > sb->cluster_count) {
		pr_err("Internal Error: invalid cluster range %lu ~ %lu.\n", index, index + count - 1);
		return -EINVAL;
	}

	return get_sector(sb,
			data,
			heap_start + ((index - EXFAT_FIRST_CLUSTER) * sb->cluster_size),
			clu_per_sec * count);
}

/**
 * set_cluster -  Set Raw-Data from any cluster
 * @sb:           Filesystem metadata
 * @data:         cluster raw data
 * @index:        Start cluster index
 * @count:        The number of clusters
 *
 * @return        == 0 (success)
 *                <  0 (failed to read)
 *
 * NOTE: Need to allocate @data before call it.
 */
int set_cluster(struct super_block *sb, void *data, off_t index, size_t count)
{
	size_t clu_per_sec = sb->cluster_size / sb->sector_size;
	off_t heap_start = sb->heap_offset * sb->sector_size;

	if (index < EXFAT_FIRST_CLUSTER || index + count > sb->cluster_count) {
		pr_err("Internal Error: invalid cluster range %lu ~ %lu.\n", index, index + count - 1);
		return -EINVAL;
	}

	return set_sector(sb,
			data,
			heap_start + ((index - EXFAT_FIRST_CLUSTER) * sb->cluster_size),
			clu_per_sec * count);
}

/**
 * print_cluster -  Print Raw-Data from any cluster
 * @sb:             Filesystem metadata
 * @index:          Start cluster index
 * @count:          The number of clusters
 *
 * @return          == 0 (success)
 *                  <  0 (failed)
 */
int print_cluster(struct super_block *sb, off_t index, size_t count)
{
	int i;
	int ret = 0;
	size_t line, byte = 0;
	size_t size = sb->cluster_size;
	size_t lines = size / 0x10;
	void *data;

	if ((data = malloc(size)) == NULL)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		ret = get_cluster(sb, data, index + i, 1);
		if (ret < 0)
			goto out;

		for (line = 0; line < lines; line++) {
			pr_msg("%08lX:  ", line * 0x10);
			for (byte = 0; byte < 0x10; byte++) {
				pr_msg("%02X ", ((unsigned char *)data)[line * 0x10 + byte]);
			}
			putchar(' ');
			for (byte = 0; byte < 0x10; byte++) {
				char ch = ((unsigned char *)data)[line * 0x10 + byte];
				pr_msg("%c", isprint(ch) ? ch : '.');
			}
			pr_msg("\n");
		}
	}
out:
	free(data);
	
	return ret;
}
