// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */

#include <unistd.h>
#include <ctype.h>

#include "exfat.h"
#include "breakexfat.h"

/**
 * @brief Get Raw-Data from any sector
 *
 * @param [in]  sb    Filesystem metadata
 * @param [out] data  Sector raw data
 * @param [in]  index Start sector index
 * @param [in]  count The number of sectors
 *
 * @retval 0 success
 * @retval Negative failed
 *
 * @attention Need to allocate @data before call it.
 */
int get_sector(struct super_block *sb, void *data, off_t index, size_t count)
{
	off_t offset = index * sb->sector_size;

	pr_debug("Get: Sector from 0x%lx to 0x%lx\n",
			offset, offset + (count * sb->sector_size) - 1);

	if ((pread(sb->fd, data, sb->sector_size * count, offset)) < 0) {
		pr_err("read: %s\n", strerror(errno));
		return -errno;
	}

	return 0;
}

/**
 * @brief Set Raw-Data from any sector
 *
 * @param [in] sb    Filesystem metadata
 * @param [in] data  Sector raw data
 * @param [in] index Start sector index
 * @param [in] count The number of sectors
 *
 * @retval 0 success
 * @retval Negative failed
 *
 * @attention  Need to allocate @data before call it.
 */
int set_sector(struct super_block *sb, void *data, off_t index, size_t count)
{
	off_t offset = index * sb->sector_size;

	pr_debug("Set: Sector from 0x%lx to 0x%lx\n",
			offset, offset + (count * sb->sector_size) - 1);

	if ((pwrite(sb->fd, data, sb->sector_size * count, offset)) < 0) {
		pr_err("write: %s\n", strerror(errno));
		return -errno;
	}

	return 0;
}

/**
 * @brief Print Raw-Data from any sector
 *
 * @param [in] sb    Filesystem metadata
 * @param [in] index Start sector index
 * @param [in] count The number of sectors
 *
 * @retval 0 success
 * @retval Negative failed
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
 * @brief Get Raw-Data from any cluster
 *
 * @param [in]  sb    Filesystem metadata
 * @param [out] data  Cluster raw data
 * @param [in]  index Start cluster index
 * @param [in]  count The number of clusters
 *
 * @retval 0 success
 * @retval Negative failed
 *
 * @attention Need to allocate @data before call it.
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
 * @brief Set Raw-Data from any cluster
 *
 * @param [in] sb    Filesystem metadata
 * @param [in] data  Cluster raw data
 * @param [in] index Start cluster index
 * @param [in] count The number of clusters
 *
 * @retval 0 success
 * @retval Negative failed
 *
 * @attention  Need to allocate @data before call it.
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
 * @brief Print Raw-Data from any cluster
 *
 * @param [in] sb    Filesystem metadata
 * @param [in] index Start cluster index
 * @param [in] count The number of clusters
 *
 * @retval 0 success
 * @retval Negative failed
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
