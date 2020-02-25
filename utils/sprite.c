/*
 * sprite.c - TODO.
 *
 * Import - TODO.
 * Export - TODO.
 *
 * Copyright (C) 2020 Jan Havran
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <getopt.h>

void print_help(void)
{
	printf("usage: [OPTION]... \n"
		" -h, --help\tprint this help\n");
}

int check_args(int argc, char *argv[])
{
	int c;
	int opt_index = 0;
	static struct option long_opt[] = {
		{ "help",	0, NULL, 'h' },
		{ NULL,		0, NULL, 0   }
	};

	while ((c = getopt_long(argc, argv, "h",
		long_opt, &opt_index)) != -1) {
		switch (c) {
		case 'h':
			print_help();
			return 1;
		case '?':
			print_help();
			return 1;
		}
	}

	if (argc - optind > 1) {
		print_help();
		return 1;
	}

	return 0;
}
int main(int argc, char *argv[])
{
	printf("a\n");
	if (check_args(argc, argv))
		return 0;

	return 0;
}

