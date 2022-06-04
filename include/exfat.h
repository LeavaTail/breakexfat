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

#define EXFAT_FIRST_CLUSTER  2           //!< Cluster will be started from 2
#define EXFAT_BADCLUSTER     0xFFFFFFF7  //!< FATentry's corresponding cluster as "bad"
#define EXFAT_LASTCLUSTER    0xFFFFFFFF  //!< cluster as the last cluster of a cluster chain

#define EXFAT_SECTOR_MIN     512         //!< 512 bytes is minimum sector
#define EXFAT_SECTOR_MAX     4096        //!< 4096 bytes is maximum sector
#define EXFAT_CLUSTER_MAX    (32* 1024 * 1024)  //!< 32 Mega bytes is maximum cluster

/**
 * information about the enclosing target exFAT filesystem
 */
struct super_block {
	int fd;                 //!< opened file for exFAT filesystem image
	off_t total_size;       //!< volume size

	/* Derived from Boot sector */
	uint64_t part_offset;   //!< media-relative sector offset of the partition
	uint32_t vol_size;      //!< size of the given exFAT volume in sectors
	uint16_t sector_size;   //!< bytes per sector
	uint32_t cluster_size;  //!< bytes per cluster
	uint32_t cluster_count; //!< the number of clusters the Cluster Heap
	uint32_t fat_offset;    //!< volume-relative sector offset of the First FAT
	uint32_t fat_length;    //!< length in sectors of each FAT table
	uint8_t num_fats;       //!< the number of FATs and Allocation Bitmaps
	uint32_t heap_offset;   //!< volume-relative sector offset of the Cluster Heap
	uint32_t root_offset;   //!< cluster index of the first cluster of the root directory
	uint32_t alloc_offset;  //!< cluster index of the first cluster of the 1st Allocation Bitmap
	uint32_t alloc_second;  //!< cluster index of the first cluster of the 2nd Allocation Bitmap
	uint64_t alloc_length;  //!< length of Allocation Bitmap
	uint32_t upcase_offset; //!< cluster index of the first cluster of the Up-case table
	uint32_t upcase_size;   //!< length of Up-case table

	/* Meta Data */
	uint64_t opt;           //!< Command line option

	/* cached list */
	struct list_head *inodes;       //!< cached inode

	struct list_head *sector_list;  //!< cached sector
	struct list_head *cluster_list; //!< cached cluster
};

/**
 * metadata pertaining to the file/directory
 */
struct inode {
	char *name;       //!< File/Directory name
	uint8_t name_len; //!< Name length
	uint8_t flags;    //!< GeneralSecondaryFlags in Stream dentry
	uint16_t attr;    //!< FileAttributes in File dentry
	uint32_t clu;     //!< FirstCluster in Stream dentry
	uint64_t len;     //!< DataLength in Stream dentry

	struct tm mtime;  //!< LastModified timestamp in File dentry
	struct tm atime;  //!< LastAccessed timestamp in File dentry
	struct tm ctime;  //!< LastCreate timestamp in File dentry

	struct inode *p_inode;  //!< Parent Directory inode

	atomic_int refcount;    //!< reference count for inode
};

/* For GeneralPrimaryFlags Field */
#define ALLOC_POSSIBLE BIT(0) //!< allocation in the Cluster Heap is possible
#define NOFATCHAIN     BIT(1) //!< given allocation's cluster chain

/* For Boot sector */
#define BOOTSEC_JUMPBOOT_LEN  3  //!< length of JumpBoot
#define BOOTSEC_FSNAME_LEN    8  //!< length of FileSystemName
#define BOOTSEC_ZERO_LEN      53 //!< length of MustBeZero

#define FILENAME_LEN     15  //!< length of the maximum FileName in dentry
#define FILENAME_NUM     17  //!< the maximum number of File Name dentries
#define MAX_NAME_LENGTH  (FILENAME_LEN * FILENAME_NUM) //!< length of the maximum FileName

/**
 * boot-strapping from an exFAT volume (512 bytes)
 */
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

/**
 * directory entry (32 bytes)
 */
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
