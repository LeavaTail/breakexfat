// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#ifndef _BREAKEXFAT_H
#define _BREAKEXFAT_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <linux/types.h>

#include "exfat.h"
#include "list.h"

/**
 * Program Name, version, author.
 * displayed when 'usage' and 'version'
 */
#define PROGRAM_NAME     "breakexfat"
#define PROGRAM_VERSION  "0.1.0"
#define PROGRAM_AUTHOR   "LeavaTail"
#define COPYRIGHT_YEAR   "2022"

/**
 * Debug code
 */
extern unsigned int print_level;

#define PRINT_ERR      1
#define PRINT_WARNING  2
#define PRINT_INFO     3
#define PRINT_DEBUG    4

#define print(level, fmt, ...) \
	do { \
		if (print_level >= level) { \
			if (level == PRINT_DEBUG) \
			fprintf( stdout, "(%s:%u): " fmt, \
					__func__, __LINE__, ##__VA_ARGS__); \
			else \
			fprintf( stdout, "" fmt, ##__VA_ARGS__); \
		} \
	} while (0) \

#define pr_err(fmt, ...)   print(PRINT_ERR, fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...)  print(PRINT_WARNING, fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...)  print(PRINT_INFO, fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...) print(PRINT_DEBUG, fmt, ##__VA_ARGS__)
#define pr_msg(fmt, ...)   fprintf(stdout, fmt, ##__VA_ARGS__)

/**
 * Cached data (sector/cluster)
 */
struct cache {
	struct super_block *sb;
	void *data;
	off_t offset;
	size_t count;
	bool dirty;
	int (*read)(struct super_block *, void *, off_t, size_t);
	int (*write)(struct super_block *, void *, off_t, size_t);
	int (*print)(struct super_block *, off_t, size_t);
	struct list_head *next;
};

#define MAX(a, b)      ((a) > (b) ? (a) : (b))
#define MIN(a, b)      ((a) < (b) ? (a) : (b))
#define ROUNDUP(a, b)  ((a + b - 1) / b)

static inline bool is_power2(unsigned int n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

static inline uint64_t power2(uint32_t n)
{
	return 1 << n;
}

static inline int validate_cluster(struct super_block *sb, uint32_t clu)
{
	if (clu == EXFAT_LASTCLUSTER)
		return 0;
	if (clu < EXFAT_FIRST_CLUSTER)
		return -EINVAL;
	if (clu > sb->cluster_count + 1)
		return -EINVAL;
	if (clu == EXFAT_BADCLUSTER)
		return -EINVAL;
	return 0;
}

int get_sector(struct super_block *sb, void *data, off_t index, size_t count);
int set_sector(struct super_block *sb, void *data, off_t index, size_t count);
int print_sector(struct super_block *sb, off_t index, size_t count);
int get_cluster(struct super_block *sb, void *data, off_t index, size_t count);
int set_cluster(struct super_block *sb, void *data, off_t index, size_t count);
int print_cluster(struct super_block *sb, off_t index, size_t count);

int fill_super(struct super_block *sb, const char *name);
int put_super(struct super_block *sb);
struct inode *alloc_inode(struct super_block *sb);
int free_inode(struct inode *inode);

struct cache *create_cluster_cache(struct super_block *sb, uint32_t index, size_t count);
struct cache *create_sector_cache(struct super_block *sb, uint32_t index, size_t count);
struct cache *get_cluster_cache(struct super_block *sb, uint32_t index);
struct cache *get_sector_cache(struct super_block *sb, uint32_t index);
int remove_cache(struct super_block *sb, struct list_head *prev);
int remove_cache_list(struct super_block *sb, struct list_head *head);

int enable_break_pattern(struct super_block *sb, unsigned int index);
int disable_break_pattern(struct super_block *sb, unsigned int index);

int update_active_fat(struct super_block *sb, int index);
int get_fat_entry(struct super_block *sb, uint32_t clu, uint32_t *entry);
int set_fat_entry(struct super_block *sb, uint32_t clu, uint32_t entry);
int get_next_cluster(struct super_block *sb, struct inode *inode, uint32_t clu, uint32_t *entry);

int update_active_bitmap(struct super_block *sb, int index);
int set_alloc_bitmap(struct super_block *sb, uint32_t clu);
int unset_alloc_bitmap(struct super_block *sb, uint32_t clu);
int get_alloc_bitmap(struct super_block *sb, uint32_t clu);

#endif /*_DEBUGFATFS_H */
