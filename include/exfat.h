// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */

#ifndef _EXFAT_H
#define _EXFAT_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdatomic.h>
#include <linux/types.h>

#include "list.h"

#define EXFAT_FIRST_CLUSTER  2
#define EXFAT_BADCLUSTER     0xFFFFFFF7
#define EXFAT_LASTCLUSTER    0xFFFFFFFF

/*
 * exFAT filesystem superblock
 */
struct super_block {
	int fd;
	off_t total_size;

	uint64_t part_offset;
	uint32_t vol_size;
	uint16_t sector_size;
	uint32_t cluster_size;
	uint32_t cluster_count;
	uint32_t fat_offset;
	uint32_t fat_length;
	uint8_t num_fats;
	uint32_t heap_offset;
	uint32_t root_offset;
	uint32_t alloc_offset;
	uint32_t alloc_second;
	uint64_t alloc_length;
	uint32_t upcase_offset;
	uint32_t upcase_size;

	struct list_head *inodes;

	struct list_head *sector_list;
	struct list_head *cluster_list;
};

/*
 * exFAT filesystem inode
 */
struct inode {
	char *name;
	uint8_t name_len;
	uint8_t flags;
	uint16_t attr;
	uint32_t clu;
	uint64_t len;

	struct tm mtime;
	struct tm atime;
	struct tm ctime;

	struct inode *p_inode;

	atomic_int refcount;
};

enum {
	AllocationPossible_bit,
	NoFatChain_bit,
};

#define BOOTSEC_JUMPBOOT_LEN		3
#define BOOTSEC_FSNAME_LEN		8
#define BOOTSEC_ZERO_LEN		53

#define FILENAME_LEN			15
#define MAX_NAME_LENGTH         255

/* EXFAT: Main and Backup Boot Sector (512 bytes) */
struct boot_sector {
	__u8	jmp_boot[BOOTSEC_JUMPBOOT_LEN];
	__u8	fs_name[BOOTSEC_FSNAME_LEN];
	__u8	must_be_zero[BOOTSEC_ZERO_LEN];
	__le64	partition_offset;
	__le64	vol_length;
	__le32	fat_offset;
	__le32	fat_length;
	__le32	clu_offset;
	__le32	clu_count;
	__le32	root_cluster;
	__le32	vol_serial;
	__u8	fs_revision[2];
	__le16	vol_flags;
	__u8	sect_size_bits;
	__u8	sect_per_clus_bits;
	__u8	num_fats;
	__u8	drv_sel;
	__u8	percent_in_use;
	__u8	reserved[7];
	__u8	boot_code[390];
	__le16	signature;
} __attribute__((packed));

struct exfat_dentry {
	__u8 type;
	union {
		struct {
			__u8 num_ext;
			__le16 checksum;
			__le16 attr;
			__le16 reserved1;
			__le16 create_time;
			__le16 create_date;
			__le16 modify_time;
			__le16 modify_date;
			__le16 access_time;
			__le16 access_date;
			__u8 create_time_cs;
			__u8 modify_time_cs;
			__u8 create_tz;
			__u8 modify_tz;
			__u8 access_tz;
			__u8 reserved2[7];
		} __attribute__((packed)) file; /* file directory entry */
		struct {
			__u8 flags;
			__u8 reserved1;
			__u8 name_len;
			__le16 name_hash;
			__le16 reserved2;
			__le64 valid_size;
			__le32 reserved3;
			__le32 start_clu;
			__le64 size;
		} __attribute__((packed)) stream; /* stream extension directory entry */
		struct {
			__u8 flags;
			__le16 name[FILENAME_LEN];
		} __attribute__((packed)) name; /* file name directory entry */
		struct {
			__u8 flags;
			__u8 reserved[18];
			__le32 start_clu;
			__le64 size;
		} __attribute__((packed)) bitmap; /* allocation bitmap directory entry */
		struct {
			__u8 reserved1[3];
			__le32 checksum;
			__u8 reserved2[12];
			__le32 start_clu;
			__le64 size;
		} __attribute__((packed)) upcase; /* up-case table directory entry */
	} __attribute__((packed)) dentry;
} __attribute__((packed));

#endif /* !_EXFAT_H */
