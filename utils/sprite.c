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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>

enum OP_TYPE {
	EXPORT,
};

struct palette {
	unsigned start;
	unsigned size;
	void *next;
	uint8_t data[0];
};

struct frame {
	unsigned compressed;
	unsigned width;
	unsigned height;
	signed palette_off;
	uint8_t data[0];
};

void print_help(void)
{
	printf("usage: [OPTION]... [FILE]...\n"
		"If FILE is not specified, then standard input will be used\n\n"
		" -e, --export\texport input file and write to the output\n"
		" -h, --help\tprint this help\n");
}

int check_args(int argc, char *argv[],
	enum OP_TYPE *operation)
{
	int c;
	int opt_index = 0;
	static struct option long_opt[] = {
		{ "export",	0, NULL, 'e' },
		{ "help",	0, NULL, 'h' },
		{ NULL,		0, NULL, 0   }
	};

	while ((c = getopt_long(argc, argv, "h",
		long_opt, &opt_index)) != -1) {
		switch (c) {
		case 'e':
			*operation = EXPORT;
			break;
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

//int read_chunk
void saveppm(FILE *fw, unsigned width, unsigned height, uint8_t *data)
{
	char header[] = "P6\n";
	char comment[] = "#This is comment\n";

	fwrite(header, strlen(header), 1, fw);
	fwrite(comment, strlen(comment), 1, fw);

	fprintf(fw, "%u %u\n", width, height);
	fprintf(fw, "255\n");

	fwrite(data, 3, width * height, fw);

	return;
}

int read_palette(FILE *f, uint8_t *palette)
{
	uint16_t chunk_size;

	if (fread(&chunk_size, sizeof(chunk_size), 1, f) != 1) {
		fprintf(stderr, "Unable to read chunk size for offsets\n");
		return 0;
	}

	printf("chunk_size %d\n", chunk_size);
	uint16_t pos = 2;
	while (pos < chunk_size) {
		uint8_t start;
		fread(&start, sizeof(start), 1, f);

		uint8_t size;
		fread(&size, sizeof(size), 1, f);

		printf("start: %d, size: %d\n", start, size);
		if(size == 0xFF && start == 0xFF)
			break;

		for (unsigned i = 0; i < size; i++) {
			uint8_t b1, b2, b3;

			fread(&b1, sizeof(size), 1, f);
			fread(&b2, sizeof(size), 1, f);
			fread(&b3, sizeof(size), 1, f);

			palette[(start + i) * 3 + 0] = b1 << 2;
			palette[(start + i) * 3 + 1] = b2 << 2;
			palette[(start + i) * 3 + 2] = b3 << 2;
		}
	}
	fseek(f, chunk_size, SEEK_SET);
	return 0;
}

/* replace by read_offsets, get_offsets, getoffset */
int read_offsets(FILE *f, uint16_t **offsets)
{
	unsigned frames = 0;
	uint16_t chunk_size;

	if (fread(&chunk_size, sizeof(chunk_size), 1, f) != 1) {
		fprintf(stderr, "Unable to read chunk size for offsets\n");
		return -1;
	} else if ((chunk_size & 1) != 0) {
		fprintf(stderr, "Chunk size for offsets has to be multiple"
			"of two (is %u)\n", chunk_size);
		return -1;
	}

	frames = chunk_size / 2;
	*offsets = (uint16_t *) malloc(frames * sizeof(uint16_t));
	if (offsets == NULL) {
		fprintf(stderr, "Unable to alloc memory for offsets\n");
		return -1;
	}

	*(offsets[0]) = chunk_size;
	if (fread(&((*offsets)[1]), sizeof(uint16_t), frames - 1, f) != frames - 1) {
		fprintf(stderr, "Unable to read all offsets\n");
		free(*offsets);
		return -1;
	}

	return frames;
}

struct frame * read_file(FILE *f, int start, int offset)
{
	uint8_t bytes[4];
	unsigned width;
	unsigned height;
	struct frame *fr = NULL;

	if (fseek(f, start + offset, SEEK_SET)) {
		printf("sakra\n");
		return fr;
	}

	fread(bytes, 4, 1, f);

	width = (unsigned) bytes[0] | (unsigned) ((bytes[1] & 0x7f) << 8);
	height = bytes[2];

	fr = malloc(sizeof(struct frame) + width * height * 3);
	fr->compressed = !!(bytes[1] & 0x80);
	fr->width = width;
	fr->height=height;
	fr->palette_off = *((int8_t *) &bytes[3]);

	printf("isCompressed: %s\n", (bytes[1] & 0x80) ? "yes" : "no");
	printf("width: %d\n",  width);
	printf("height: %d\n", height);
	printf("palOffset: %d\n\n", fr->palette_off);


	int pos = 0;

	if (fr->compressed) {
	while (pos < width * height) {
		/* ridici bajt
		 * XYYYYYY
		 * X - pouzit hodnotu pouze jedenkrat, jinak precist v kazdem pruchodu
		 * Y - pocet opakovani
		 */
		uint8_t control;
		if (fread(&control, 1, 1, f) != 1) {
			printf("aha\n");
			return NULL;
		}
		uint8_t pixel;
		if (control & 0x80) {
			pixel = fread(bytes, 1, 1, f);
			printf("READC\n");
		}
		for (int i = 0; i < (control & 0x7F); i++) {
			if (!(control & 0x80)) {
				pixel = fread(bytes, 1, 1, f);
				printf("READB\n");
			}
//			if (pos + 2 > width * height)
//				break;
			fr->data[pos + 0] = (pixel & 0xF) + fr->palette_off;
			fr->data[pos + 1] = (pixel >> 4)  + fr->palette_off;
			printf("%d: %d\n%d: %d\n", pos + 0, fr->data[pos + 0], pos + 1, fr->data[pos + 1]);
			pos += 2;
			if (pos >= width * height)
				break;
		}
//		printf("pos %d, %x\n", pos, control);
	}
	}

	return fr;
}

int export(FILE *fr, FILE *fw)
{
	uint16_t chunk_size;
	uint16_t *offsets = NULL;
	uint8_t **data = NULL;
	int ptr = 0;
	int frames = 0;
	uint8_t buf[65536];

	/* read palete */

	uint8_t pal[256*3] = { 0 };
	read_palette(fr, pal);
	ptr = 270;
/*
	fread(&chunk_size, 2, 1, fr); ptr += 2;
	printf("palette chunk size %u\n", chunk_size);
	fread(buf, chunk_size - 2, 1, fr); ptr += chunk_size - 2;
*/
	int off_ptr = ptr;

	frames = read_offsets(fr, &offsets);
	printf("frames %d\n", frames);
	ptr += frames * 2;
	for (int i = 0; i < frames; i++)
		printf("frame %d at %d\n", i, offsets[i]);

	data = (uint8_t **) malloc(frames * sizeof(uint8_t *));
	/* read data */
	for (int i = 1; i < frames - 1; i++) {	// nezpracovavame posledni frame
		printf("frame %d\n", i);
		struct frame *ff = read_file(fr, off_ptr, offsets[i]);
		uint8_t *d = calloc(ff->width * ff->height * 3, 1);
		for (int i = 0; i < ff->width * ff->height; i++) {
			//printf("%d\n", i);
			//printf("aha\n");
			if (ff->data[i] < 256) {
/*
				printf("%d\n", ff->data[i]);
				printf("%d -> %d\n", pa->start, pa->start + pa->size);
*/
				d[i * 3 + 0] = pal[ff->data[i] * 3 + 0];
				d[i * 3 + 1] = pal[ff->data[i] * 3 + 1];
				d[i * 3 + 2] = pal[ff->data[i] * 3 + 2];
//				printf("[%d][%d]: %03d|%03d|%03d\n", i / ff->width, i % ff->width, d[i * 3 + 0], d[i * 3 + 1], d[i * 3 + 2]);
			}
			else printf("sakra\n");
		}
		printf("save\n");
		saveppm(stderr, ff->width, ff->height, d);
		break;
	}
/*
	uint8_t dat[16*3] = { 0 };

	dat[3] = 0xff;
	dat[7] = 0xff;
	dat[11] = 0xff;

	saveppm(stderr, 4, 4, dat);
*/
	return 0;
}

int main(int argc, char *argv[])
{
	FILE *fr;
	FILE *fw;
	enum OP_TYPE operation;

	if (check_args(argc, argv, &operation))
		return 0;

	fr = stdin;
	fw = stdout;

	export(fr, fw);

	return 0;
}

