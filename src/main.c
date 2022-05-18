// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2021 LeavaTail
 */
#include <getopt.h>

#include "exfat.h"
#include "breakexfat.h"
#include "list.h"

#define MANDATORY_ARGUMENT 2

unsigned int print_level = PRINT_WARNING;

/**
 * Special Option(no short option)
 */
enum
{
	GETOPT_HELP_CHAR = (CHAR_MIN - 2),
	GETOPT_VERSION_CHAR = (CHAR_MIN - 3)
};

/* option data {"long name", needs argument, flags, "short name"} */
static struct option const longopts[] =
{
	{0,0,0,0}
};

/**
 * usage - print out usage
 */
static void usage(void)
{
	fprintf(stderr, "Usage: %s [OPTION]... FILE [PATTERN,...]\n", PROGRAM_NAME);
	fprintf(stderr, "break FAT/exFAT filesystem image.\n");
	fprintf(stderr, "\n");
}

/**
 * version        - print out program version
 * @command_name:   command name
 * @version:        program version
 * @author:         program authoer
 */
static void version(const char *command_name, const char *version, const char *author)
{
	fprintf(stdout, "%s %s\n", command_name, version);
	fprintf(stdout, "\n");
	fprintf(stdout, "Written by %s.\n", author);
}

/**
 * parse_break_pattern - Parse cmdline argument
 * @sb:                  Filesystem metadata
 * @line:                 cmdline argument
 *
 * @return               == 0 (success)
 *                       <  0 (failed)
 */
int parse_break_pattern(struct super_block *sb, char *line)
{
	long pattern;
	char *ptr, *end;

	ptr = strtok(line, ",");

	while (ptr != NULL) {
		pattern = strtol(ptr, &end, 10);
		if (*end != '\0') {
			pr_warn("Irregular character found %s\n", end);
			return -EINVAL;
		}
		enable_break_pattern(sb, pattern);
		ptr = strtok(NULL, ",");
	}

	return 0;
}

/**
 * main   - main function
 * @argc:   argument count
 * @argv:   argument vector
 */
int main(int argc, char *argv[])
{
	int opt;
	int longindex;
	struct super_block sb = {0};

	while ((opt = getopt_long(argc, argv,
					"",
					longopts, &longindex)) != -1) {
		switch (opt) {
			case GETOPT_HELP_CHAR:
				usage();
				exit(EXIT_SUCCESS);
			case GETOPT_VERSION_CHAR:
				version(PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_AUTHOR);
				exit(EXIT_SUCCESS);
			default:
				usage();
				exit(EXIT_FAILURE);
		}
	}

#ifdef EXFAT_DEBUG
	print_level = PRINT_DEBUG;
#endif

	if (optind != argc - MANDATORY_ARGUMENT) {
		usage();
		exit(EXIT_FAILURE);
	}

	initialize_super(&sb, argv[1]);
	read_boot_sector(&sb);
	read_fat_region(&sb);
	parse_break_pattern(&sb, argv[2]);
	finalize_super(&sb);

	return 0;
}
