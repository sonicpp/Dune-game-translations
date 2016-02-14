/*
 * tu.c - Text Util for Cryo's game Dune.
 *
 * Unpack - Extract sentences from Dune files (COMMAND{1,3}, PHRASE{1,3}{1,2})
 * Pack - Pack files created by this util back to the game format.
 *
 * Copyright (C) 2016 Jan Havran <havran.jan@email.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

enum OP_TYPE {
	PACK,
	UNPACK,
	CHECK
};

typedef unsigned short t_word;

void print_help(void)
{
	printf("usage: [OPTION]... [FILE]...\n"
		" -v, --verbose\tverbose mode\n"
		" -p, --pack\tpack FILE1 and save as FILE2\n"
		" -u, --unpack\tunpack FILE1 and save as FILE2\n"
		" -c, --check\tcheck text file FILE\n"
		" -h, --help\tprint this help\n");
}

int is_little(void)
{
	t_word w = 0x0001;

	return *((unsigned char *) &w);
}

t_word to_little(const t_word w)
{
	if (is_little())
		return w;
	else
		return    ((w & 0x00FF) << 8)
			| ((w & 0xFF00) >> 8);
}

int check_args(const int argc, char * const argv[],
	int *verbose, enum OP_TYPE *operation,
	char **fin, char **fout)
{
	int c;
	int pack = 0;
	int unpack = 0;
	int check = 0;
	int opt_index = 0;
	static struct option long_opt[] = {
		{ "verbose",	0, NULL, 'v' },
		{ "pack",	0, NULL, 'p' },
		{ "unpack",	0, NULL, 'u' },
		{ "check",	0, NULL, 'c' },
		{ "help",	0, NULL, 'h' },
		{ NULL,		0, NULL, 0   }
	};

	*verbose = 0;

	while ((c = getopt_long(argc, argv, "vpuch",
		long_opt, &opt_index)) != -1) {
		switch (c) {
		case 'v':
			*verbose = 1;
			break;
		case 'p':
			pack = 1;
			*operation = PACK;
			break;
		case 'u':
			unpack = 1;
			*operation = UNPACK;
			break;
		case 'c':
			check = 1;
			*operation = CHECK;
			break;
		case 'h':
			print_help();
			return 1;
		case '?':
			print_help();
			return 1;
		}
	}

	if (pack + unpack + check != 1) {
		print_help();
		return 1;
	}

	if ((pack | unpack) == 1) {
		if (argc - optind != 2) {
			print_help();
			return 1;
		}
		*operation = (pack) ? PACK : UNPACK;
		*fin = argv[optind];
		*fout = argv[optind + 1];
	}
	else if (check == 1) {
		if (argc - optind != 1) {
			print_help();
			return 1;
		}
		*fin = argv[optind];
		*fout = NULL;
	}

	return 0;
}

long file_size(FILE *f)
{
	long len;

	if (fseek(f, 0L, SEEK_END))
		return -1L;
	if ((len = ftell(f)) == -1L)
		return -1L;
	if (fseek(f, 0L, SEEK_SET))
		return -1L;

	return len;
}

char *read_file(char *name, long *len)
{
	FILE *f;
	char *buffer;
	long read;

	if ((f = fopen(name, "rb")) == NULL) {
		fprintf(stderr, "Opening file '%s' failed: %s\n",
			name, strerror(errno));
		return NULL;
	}

	if ((*len = file_size(f)) == -1L) {
		fclose(f);

		fprintf(stderr, "Getting file size failed\n");
		return NULL;
	}

	buffer = (char *) malloc(sizeof(char) * (*len));
	if (buffer == NULL) {
		fclose(f);

		fprintf(stderr, "Memory alloc failed\n");
		return NULL;
	}

	read = fread(buffer, 1, *len, f);
	if (read != *len) {
		fclose(f);
		free(buffer);

		fprintf(stderr, "File read error (read %ld of %ld)\n",
			read, *len);
		return NULL;
	}

	fclose(f);

	return buffer;
}

int check(char *buf, long len, int verbose)
{
	t_word *offset_ptr, *text_bgn;
	t_word offset, offset_prev;
	char *sentence_end;

	offset_ptr = (t_word *) buf;
	offset = to_little(*offset_ptr);
	text_bgn = (t_word *) (buf + offset);

	/* File is too short, corrupted, or too long for 16 bit addressing */
	if (len < 3L || offset > len || len > 65535L)
		return 1;

	offset_ptr++;
	offset_prev = offset;
	offset = to_little(*offset_ptr);

	/* Compare offset and real length of sentence */
	while (offset < len && offset_ptr != text_bgn) {
		sentence_end = memchr(buf + offset_prev, (char) 0xFF,
			len - offset_prev);
		if (sentence_end == NULL || sentence_end - buf + 1 != offset)
			return 1;

		offset_ptr++;
		offset_prev = offset;
		offset = to_little(*offset_ptr);
	}

	/* Test for last sentece */
	sentence_end = memchr(buf + offset_prev, (char) 0xFF,
		len - offset_prev);
	if (sentence_end == NULL || sentence_end - buf + 1 != len)
		return 1;

	return 0;
}

int unpack(char *buf, long len, char *name, int verbose)
{
	FILE *f;
	long i;
	t_word offset;

	offset = to_little(*((t_word *) buf));

	if (memchr(buf + offset, '\n', len - offset) != NULL) {
		fprintf(stderr, "Char '\\n' is used!\n");
		return 1;
	}

	if ((f = fopen(name, "wb")) == NULL) {
		fprintf(stderr, "Error opening file '%s'\n", name);
		return 1;
	}

	for (i = offset; i < len; i++) {
		if (buf[i] == (char) 0xFF)
			buf[i] = '\n';
	}

	if (fwrite(buf + offset, 1, len - offset, f) != len - offset) {
		fclose(f);

		fprintf(stderr, "Write to file failed\n");
		return 1;
	}

	fclose(f);

	return 0;
}

int pack(char *buf, long len, char *name, int verbose)
{
	FILE *f;
	int i;
	int phrases = 0;
	int phrase_len;
	char *phrase_end;
	t_word offset, offset_le;
	t_word off_start;

	if ((f = fopen(name, "wb")) == NULL) {
		fprintf(stderr, "Error opening file '%s'\n", name);
		return 1;
	}

	for (i = 0; i < len; i++) {
		if (buf[i] == '\n') {
			buf[i] = 0xFF;
			phrases++;
		}
	}

	if (phrases * 2L + len > 65535L) {
		fclose(f);

		fprintf(stderr, "Input file is too big\n");
		return 1;
	}

	if (verbose)
		printf("%d phrases found\n", phrases);

	offset = phrases * 2;
	offset_le = to_little(offset);
	off_start = offset;
	phrase_end = buf + offset;

	/* Write offsets */
	for (i = 0; i < phrases; i++) {
		if (fwrite(&offset_le, 1, 2, f) != 2) {
			fclose(f);

			fprintf(stderr, "Write to file failed\n");
			return 1;
		}

		phrase_end = (char *) memchr(buf + offset - off_start, 0xFF,
			len - offset + off_start);
		if (phrase_end == NULL) {
			fclose(f);

			fprintf(stderr,
				"Unknown error (input file corrupted?)\n");
			return 1;
		}

		phrase_len = phrase_end - buf + 1;
		offset = off_start + phrase_len;
		offset_le = to_little(offset);
	}

	/* Write sentences */
	if (fwrite(buf, 1, len, f) != len) {
		fclose(f);

		fprintf(stderr, "Write to file failed\n");
		return 1;
	}

	fclose(f);

	if (verbose)
		printf("Created file '%s' of size: %ld\n",
			name, len + phrases * 2L);

	return 0;
}

int main(int argc, char *argv[])
{
	char *fr_name, *fw_name;
	long f_len;
	char *buffer;
	int verbose;
	int little_endian;
	enum OP_TYPE operation;

	if (sizeof(t_word) != 2) {
		fprintf(stderr, "This util is not compatible with your hw!\n"
			"Your 'short int' is size of %lu,"
			"but required size is 2.\n",
			sizeof(t_word));

		return 1;
	}

	if (check_args(argc, argv, &verbose, &operation,
		&fr_name, &fw_name))
		return 0;

	little_endian = is_little();
	/* TODO test it on big endian and remove this msg */
	if (!little_endian)
		printf("Warning: this util was not tested on big endian hw\n");
#ifdef DEBUG
	else
		printf("Running on little endian hw\n");
#endif

	if ((buffer = read_file(fr_name, &f_len)) == NULL)
		return 0;

	if (operation != PACK) {
		if (check(buffer, f_len, verbose)) {
			free(buffer);

			printf("Text file '%s' is not valid!\n", fr_name);
			return 1;
		}
		if (operation == UNPACK) {
			if (unpack(buffer, f_len, fw_name, verbose)) {
				free(buffer);

				fprintf(stderr,
					"Error occured while unpacking file\n");
				return 1;
			}
		}
	}
	else {
		if (pack(buffer, f_len, fw_name, verbose)) {
			free(buffer);

			fprintf(stderr, "Error occured while packing file\n");
			return 1;
		}
	}

	free(buffer);

	return 0;
}

