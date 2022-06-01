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

/* displayed when 'usage' and 'version' */
#define PROGRAM_NAME     "breakexfat"
#define PROGRAM_VERSION  "0.1.0"
#define PROGRAM_AUTHOR   "LeavaTail"
#define COPYRIGHT_YEAR   "2022"

/**
 * print level variable
 */
extern unsigned int print_level;

/**
 * print level
 */
enum {
	PRINT_ERR,      //!< error message level
	PRINT_WARNING,  //!< warning message level
	PRINT_INFO,     //!< information message level
	PRINT_DEBUG,    //!< debug message level
};

/**
 * standard print message function for breakexfat
 */
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

#define pr_err(fmt, ...)   print(PRINT_ERR, fmt, ##__VA_ARGS__)      //!< error message
#define pr_warn(fmt, ...)  print(PRINT_WARNING, fmt, ##__VA_ARGS__)  //!< warning message
#define pr_info(fmt, ...)  print(PRINT_INFO, fmt, ##__VA_ARGS__)     //!< information message
#define pr_debug(fmt, ...) print(PRINT_DEBUG, fmt, ##__VA_ARGS__)    //!< debug message
#define pr_msg(fmt, ...)   fprintf(stdout, fmt, ##__VA_ARGS__)       //!< normal message

/**
 * Command line option
 */
enum {
	OPT_ALL,      //!< All failure
};

/**
 * Cached data (sector/cluster)
 */
struct cache {
	struct super_block *sb;  //!< super block cache
	void *data;              //!< cached sector/cluster
	off_t offset;            //!< sector/cluster offset
	size_t count;            //!< the number of cached data
	bool dirty;              //!< whether cache is modified from storage
	int (*read)(struct super_block *, void *, off_t, size_t);  //!< read operator
	int (*write)(struct super_block *, void *, off_t, size_t); //!< write operator
	int (*print)(struct super_block *, off_t, size_t);         //!< print operator
	struct list_head *next;  //!< next cache
};

#define MAX(a, b)      ((a) > (b) ? (a) : (b))  //!< compare and return max value
#define MIN(a, b)      ((a) < (b) ? (a) : (b))  //!< compare and return min value
#define ROUNDUP(a, b)  ((a + b - 1) / b)        //!< Calulate division round up
#define BIT(N)         (1UL << (N))             //!< bitwise operation

/**
 * @brief check if it's power of 2
 * @param [in] n target value
 *
 * @retval 1 power of 2
 * @retval 0 not power of 2
 */
static inline bool is_power2(unsigned int n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

/**
 * @brief eveluate power of 2
 * @param [in] n target value
 *
 * @return  power of 2
 */
static inline uint64_t power2(uint32_t n)
{
	return 1 << n;
}

/**
 * @brief check if @clu is valid cluster
 * @param [in] sb  Filesystem metadata
 * @param [in] clu target cluster index
 *
 * @retval 0 valid cluster
 * @retval Negative in valid cluster
 */
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
int enable_break_all_pattern(struct super_block *sb);

int update_active_fat(struct super_block *sb, int index);
int get_fat_entry(struct super_block *sb, uint32_t clu, uint32_t *entry);
int set_fat_entry(struct super_block *sb, uint32_t clu, uint32_t entry);
int get_next_cluster(struct super_block *sb, struct inode *inode, uint32_t clu, uint32_t *entry);

int update_active_bitmap(struct super_block *sb, int index);
int set_alloc_bitmap(struct super_block *sb, uint32_t clu);
int unset_alloc_bitmap(struct super_block *sb, uint32_t clu);
int get_alloc_bitmap(struct super_block *sb, uint32_t clu);

#endif /*_DEBUGFATFS_H */
