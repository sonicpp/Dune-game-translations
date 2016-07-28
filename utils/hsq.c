/*
 * hsq.c - HSQ compression util.
 *
 * Compress - Compress file by HSQ algorithm.
 * This program tends to give the EXACT same results as HSQ program used
 * by Cryo developers.
 *
 * Copyright (C) 2016 Jan Havran
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>

#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define WINDOW_SMALL	0x100U
#define WINDOW_BIG	0x2000U
#define FILE_CHUNK	0x6000U
#define HSQ_CHECKSUM	((uint8_t) 0xAB)

enum OP_TYPE {
	COMPRESS,
	DECOMPRESS,
};

typedef struct flags {
	uint16_t flag;
	uint8_t flag_size;
	uint8_t buffer[32];
	uint8_t buf_size;
} flags_t;

typedef struct lookahead_buf {
	const uint8_t *bgn;
	const uint8_t *end;
	uint16_t size;
} lookahead_buf_t;

void print_help(void)
{
	printf("usage: [OPTION]... [FILE]...\n"
		"If FILE is not specified, then standard input will be used\n\n"
		" -v, --verbose\tverbose mode\n"
		" -c, --compress\tcompress input file and write to the output\n"
		" -d, --decompress\tdecompress input file and write to the output\n"
		" -o, --out FILE\tsave output to FILE instead to stdout\n"
		" -h, --help\tprint this help\n");
 }

int check_args(int argc, char *argv[],
	int *verbose, enum OP_TYPE *operation,
	char **fin, char **fout)
{
	int c;
	int compress = 0;
	int decompress = 0;
	int opt_index = 0;
	static struct option long_opt[] = {
		{ "verbose",	0, NULL, 'v' },
		{ "compress",	0, NULL, 'c' },
		{ "decompress",	0, NULL, 'd' },
		{ "out",	1, NULL, 'o' },
		{ "help",	0, NULL, 'h' },
		{ NULL,		0, NULL, 0   }
	};

	*verbose = 0;
	*fin = NULL;
	*fout = NULL;

	while ((c = getopt_long(argc, argv, "vcdo:h",
		long_opt, &opt_index)) != -1) {
		switch (c) {
		case 'v':
			*verbose = 1;
			break;
		case 'c':
			compress = 1;
			*operation = COMPRESS;
			break;
		case 'd':
			decompress = 1;
			*operation = DECOMPRESS;
			break;
		case 'o':
			*fout = optarg;
			break;
		case 'h':
			print_help();
			return 1;
		case '?':
			print_help();
			return 1;
		}
	}

	if (compress + decompress != 1) {
		print_help();
		return 1;
	}

	if (argc - optind > 1) {
		print_help();
		return 1;
	}
	else if (argc - optind == 1) {
		*fin = argv[optind];
	}

	return 0;
}

void *memrmem(const void *mem1, size_t len1, const void *mem2, size_t len2)
{
	size_t i;

	if (len1 == 0 || len2 == 0)
		return NULL;

	if (len1 < len2)
		return NULL;

	if (mem1 == NULL || mem2 == NULL)
		return NULL;

	i = len1 - len2;
	do {
		if (memcmp(mem1 + i, mem2, len2) == 0)
			return (void *) mem1 + i;
	} while (i-- != 0);

	return NULL;
}

unsigned int putbit(flags_t *f, uint8_t bit, uint8_t *ostream)
{
	unsigned int ret = 0;

	if (f->flag_size == 16) {
		memcpy(ostream, &(f->flag), 2);
		f->flag = 0;
		ret += 2;

		memcpy(ostream + ret, f->buffer, f->buf_size);
		ret += f->buf_size;
		f->buf_size = 0;
		f->flag_size = 0;
	}

	f->flag = f->flag >> 1;
	f->flag = f->flag | ((bit & 1) << 15);
	f->flag_size++;

	return ret;
}

void putraw(flags_t *f, uint8_t byte)
{
	f->buffer[f->buf_size++] = byte;
}

size_t compress_pattern(flags_t *f, const uint8_t *lookahead_bgn,
	uint16_t lookahead_size, uint8_t *ostream, const uint8_t *match)
{
	uint8_t *ostream_bgn = ostream;
	size_t offset;
	uint16_t word = 0;

	/* Save as raw data */
	if (lookahead_size == 1) {
		ostream += putbit(f, 1, ostream);
		putraw(f, lookahead_bgn[0]);
	}
	/* Save as pointer */
	else {
		ostream += putbit(f, 0, ostream);
		offset = lookahead_bgn - match;

		/* Pointer at small sliding window */
		if (lookahead_size <= 5 && offset <= WINDOW_SMALL) {
			ostream += putbit(f, 0, ostream);
			ostream += putbit(f, ((lookahead_size - 2) >> 1) & 0x1,
				ostream);
			ostream += putbit(f, (lookahead_size - 2) & 0x1,
				ostream);
			putraw(f, WINDOW_SMALL - offset);
		}
		/* Pointer at big sliding window */
		else if (lookahead_size <= 257) {
			ostream += putbit(f, 1, ostream);

			if (lookahead_size <= 9)
				word = (lookahead_size - 2) & 7;

			word = ((WINDOW_BIG - offset) << 3) | word;

			putraw(f, word & 0xff);
			putraw(f, (word >> 8) & 0xff);

			if (lookahead_size > 9) {
				putraw(f, lookahead_size - 2);
			}
		}
	}

	return ostream - ostream_bgn;
}

const uint8_t *search_window(const uint8_t *istream,
	const uint8_t *lookahead_bgn, const uint8_t *lookahead_end,
	uint16_t lookahead_size, uint16_t window_size)
{
	const uint8_t *search_bgn;
	const uint8_t *match;

	search_bgn = MAX(istream, lookahead_bgn - window_size);
	match = memrmem(search_bgn, lookahead_end - search_bgn - 1,
		lookahead_bgn, lookahead_size);

	return match;
}

size_t save_lookh_buf(flags_t *f, const uint8_t *istream, long istream_len,
	lookahead_buf_t *lookh, uint8_t *ostream)
{
	uint8_t *ostream_bgn = ostream;
	size_t offset;
	const uint8_t *match;

	match = search_window(istream, lookh->bgn, lookh->end - 1,
		lookh->size - 1, WINDOW_BIG);
	offset = lookh->bgn - match;

	if (lookh->size > 3 || (lookh->size == 3 && offset <= WINDOW_SMALL)) {
		ostream += compress_pattern(f, lookh->bgn,
			lookh->size - 1, ostream, match);
		lookh->bgn += lookh->size - 1;
		lookh->end--;
	}
	else {
		ostream += compress_pattern(f, lookh->bgn, 1, ostream, match);
		lookh->bgn++;
		lookh->end -= lookh->size - 1;
	}

	return ostream - ostream_bgn;
}

size_t compress_chunk(flags_t *f, const uint8_t *istream,
	const uint8_t *istream_pos, long istream_len, uint8_t *ostream)
{
	const uint8_t *match;
	uint8_t *ostream_bgn = ostream;
	lookahead_buf_t lookh;

	lookh.size = 0;
	lookh.bgn = istream_pos;
	lookh.end = istream_pos;

	/* compress stream */
	while (lookh.end - istream < istream_len) {
		lookh.end++;
		lookh.size++;

		match = search_window(istream, lookh.bgn, lookh.end, lookh.size,
			WINDOW_BIG);
		if (match == NULL || lookh.size > 257) {
			ostream += save_lookh_buf(f, ostream, istream_len,
				&lookh, ostream);
			lookh.size = 0;
		}

	}

	/* compress rest of the lookahead buffer */
	if (lookh.size != 0) {
		ostream += save_lookh_buf(f, ostream, istream_len, &lookh,
			ostream);
		lookh.size = 0;
	}

	/* compress rest of the stream */
	while (lookh.end - istream < istream_len) {
		ostream += compress_pattern(f, lookh.end, 1, ostream, match);
		lookh.end++;
	}

	return ostream - ostream_bgn;
}

int decompress(FILE *fr, FILE *fw)
{
	return 0;
}

int compress(FILE *fr, FILE *fw)
{
	flags_t f;
	uint8_t out_buf[0x10000];
	uint8_t chunk_buf[FILE_CHUNK + WINDOW_BIG];
	long chunk_len;
	uint8_t *out_pos;
	long inlen = 0;
	unsigned int offset = WINDOW_BIG;
	unsigned int remain;

	f.flag = 0;
	f.flag_size = 0;
	f.buf_size = 0;

	out_pos = out_buf + 6; /* Start writing to buf just behind header */

	/* Compress input file as chunks */
	do {
		chunk_len = fread(chunk_buf + WINDOW_BIG, 1, FILE_CHUNK, fr);
		inlen += chunk_len;
		out_pos += compress_chunk(&f, chunk_buf + offset,
			chunk_buf + WINDOW_BIG,
			chunk_len + WINDOW_BIG - offset, out_pos);
		memcpy(chunk_buf, chunk_buf + FILE_CHUNK, WINDOW_BIG);
		offset = 0;
	} while (chunk_len == FILE_CHUNK);

	/* Mark end of HSQ file by setting pointer to the big sliding
	 * window of size 0 */
	out_pos += putbit(&f, 0, out_pos);
	out_pos += putbit(&f, 1, out_pos);
	for (int i = 0; i < 3; i++) {
		putraw(&f, 0);
	}

	/* Set remaining flag bits as 0 */
	while ((remain = putbit(&f, 0, out_pos)) == 0)
		;
	out_pos += remain;

	/*
	 * HSQ header
	 */
	/* Decompressed size */
	out_buf[0] = inlen >> 0;
	out_buf[1] = inlen >> 8;
	out_buf[2] = inlen >> 16;
	/* Compressed size */
	out_buf[3] = (out_pos - out_buf) >> 0;
	out_buf[4] = (out_pos - out_buf) >> 8;
	/* Checksum */
	out_buf[5] = HSQ_CHECKSUM - out_buf[0] - out_buf[1]
		- out_buf[2] - out_buf[3] - out_buf[4];

	fwrite(out_buf, 1, out_pos - out_buf, fw);

	return 0;
}

int main(int argc, char *argv[])
{
	char *fr_name, *fw_name;
	int verbose;
	FILE *fr;
	FILE *fw;
	enum OP_TYPE operation;

	if (check_args(argc, argv, &verbose, &operation,
		&fr_name, &fw_name))
		return 0;

	/* input file */
	if (fr_name) {
		if ((fr = fopen(fr_name, "r")) == NULL) {
			fprintf(stderr, "Opening file '%s' failed: %s\n",
				fr_name, strerror(errno));
			return 1;
		}
	}
	else
		fr = stdin;

	/* output file */
	if (fw_name) {
		if ((fw = fopen(fw_name, "w")) == NULL) {
			fprintf(stderr, "Opening file '%s' failed: %s\n",
				fw_name, strerror(errno));
			if (fr != stdin)
				fclose(fr);
			return 1;
		}
	}
	else
		fw = stdout;;

	if (operation == COMPRESS)
		compress(fr, fw);
	else
		decompress(fr, fw);

	if (fr != stdin)
		fclose(fr);
	if (fw != stdout)
		fclose(fw);

	return 0;
}

