#include "dfont.h"
#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#define HASH_SIZE 4096
#define TINY_FONT 12

struct hash_rect {
	struct hash_rect * next_hash;
	struct list_head next_char;
	struct list_head time;
	int version;
	int c;
	int line;
	int font;
	struct dfont_rect rect;
};

struct font_line {
	int start_line;
	int height;
	int space;
	struct list_head head;
};

struct dfont {
	int width;
	int height;
	int max_line;
	int version;
	struct list_head time;
	struct hash_rect *freelist;
	struct font_line *line;
	struct hash_rect *hash[HASH_SIZE];
};

static void
init_hash(struct dfont *df, int max) {
	int i;
	for (i=0;i<max;i++) {
		struct hash_rect * hr = &df->freelist[i];
		hr->next_hash = &df->freelist[i+1];
	}
	df->freelist[max-1].next_hash = NULL;

	memset(df->hash, 0, sizeof(df->hash));
}

static inline int
hash(int c, int font) {
	return (c ^ (font * 97)) % HASH_SIZE;
}

struct dfont *
dfont_create(int width, int height) {
	int max_line = height / TINY_FONT;
	int max_char = max_line * width / TINY_FONT;
	size_t ssize = max_char * sizeof(struct hash_rect);
	size_t lsize = max_line * sizeof(struct font_line);
	struct dfont *df = (struct dfont *)malloc(sizeof(struct dfont) + ssize + lsize);
	df->width = width;
	df->height = height;
	df->max_line = 0;
	df->version = 0;
	INIT_LIST_HEAD(&df->time);
	df->freelist = (struct hash_rect *)(df+1);
	df->line = (struct font_line *)((intptr_t)df->freelist + ssize);
	init_hash(df, max_char);

	return df;
}

void
dfont_release(struct dfont *df) {
	free(df);
}

void 
dfont_flush(struct dfont *df) {
	++df->version;
}

const struct dfont_rect * 
dfont_lookup(struct dfont *df, int c, int font) {
	int h = hash(c, font);
	struct hash_rect *hr = df->hash[h];
	while (hr) {
		if (hr->c == c && hr->font == font) {
			list_move(&hr->time, &df->time);
			hr->version = df->version;
			return &(hr->rect);
		}
		hr = hr->next_hash;
	}
	return NULL;
}

static struct font_line *
new_line(struct dfont *df, int height) {
	int start_line = 0;
	if (df->max_line > 0) {
		struct font_line * lastline = &df->line[df->max_line-1];
		start_line = lastline->start_line + lastline->height;
	}
	if (start_line + height > df->height)
		return NULL;
	int max = df->height / TINY_FONT;
	if (df->max_line >= max)
		return NULL;
	struct font_line * line = &df->line[df->max_line++];
	line->start_line = start_line;
	line->height = height;
	line->space = df->width;
	INIT_LIST_HEAD(&line->head);
	return line;
}

static struct font_line *
find_line(struct dfont *df, int width, int height) {
	int i;
	for (i=0;i<df->max_line;i++) {
		struct font_line * line = &df->line[i];
		if (height == line->height && width <= line->space) {
			return line;
		}
	}
	return new_line(df, height);
}

static struct hash_rect *
new_node(struct dfont *df) {
	if (df->freelist == NULL)
		return NULL;
	struct hash_rect *ret = df->freelist;
	df->freelist = ret->next_hash;
	return ret;
}

static struct hash_rect *
find_space(struct dfont *df, struct font_line *line, int width) {
	int start_pos = 0;
	struct hash_rect * hr;
	int max_space = 0;
	list_for_each_entry(hr, struct hash_rect, &line->head, next_char) {
		int space = hr->rect.x - start_pos;
		if (space >= width) {
			struct hash_rect *n = new_node(df);
			if (n == NULL)
				return NULL;
			n->line = line - df->line;
			n->rect.x = start_pos;
			n->rect.y = line->start_line;
			n->rect.w = width;
			n->rect.h = line->height;
			list_add_tail(&n->next_char, &hr->next_char);

			return n;
		}

		if (space > max_space) {
			max_space = space;
		}
		start_pos = hr->rect.x + hr->rect.w;
	}
	int space = df->width - start_pos;
	if (space < width) {
		if (space > max_space) {
			line->space = space;
		} else {
			line->space = max_space;
		}
		return NULL;
	}
	struct hash_rect *n = new_node(df);
	if (n == NULL)
		return NULL;
	n->line = line - df->line;
	n->rect.x = start_pos;
	n->rect.y = line->start_line;
	n->rect.w = width;
	n->rect.h = line->height;
	list_add_tail(&n->next_char, &line->head);

	return n;
}

static void
adjust_space(struct dfont *df, struct hash_rect *hr) {
	struct font_line *line = &df->line[hr->line];
	if (hr->next_char.next == &line->head) {
		hr->rect.w = df->width - hr->rect.x;
	} else {
		struct hash_rect *next = list_entry(hr->next_char.next, struct hash_rect, next_char);
		hr->rect.w = next->rect.x - hr->rect.x;
	}

	if (hr->next_char.prev == &line->head) {
		hr->rect.w += hr->rect.x;
		hr->rect.x = 0;
	} else {
		struct hash_rect *prev = list_entry(hr->next_char.prev, struct hash_rect, next_char);
		int x = prev->rect.x + prev->rect.w;
		hr->rect.w += hr->rect.x - x;
		hr->rect.x = x;
	}
	if (hr->rect.w > line->space) {
		line->space = hr->rect.w;
	}
}

static struct hash_rect *
release_char(struct dfont *df, int c, int font) {
	int h = hash(c, font);
	struct hash_rect *hr = df->hash[h];
	if (hr->c == c && hr->font == font) {
		df->hash[h] = hr->next_hash;
		list_del(&hr->time);
		adjust_space(df, hr);
		return hr;
	}
	struct hash_rect * last = hr;
	hr = hr->next_hash;
	while (hr) {
		if (c == hr->c && hr->font == font) {
			last->next_hash = hr->next_hash;
			list_del(&hr->time);
			adjust_space(df, hr);
			return hr;
		}
		last = hr;
		hr = hr->next_hash;
	}
	assert(0);
	return NULL;
}

static struct hash_rect *
release_space(struct dfont *df, int width, int height) {
	struct hash_rect *hr, *tmp;
	list_for_each_entry_safe(hr, struct hash_rect, tmp, &df->time, time) {
		if (hr->version == df->version)
			continue;
		if (hr->rect.h != height) {
			continue;
		}
		struct hash_rect * ret = release_char(df, hr->c, hr->font);
		int w = hr->rect.w;
		if (w >= width) {
			ret->rect.w = width;
			return ret;
		} else {
			list_del(&ret->next_char);
			ret->next_hash = df->freelist;
			df->freelist = ret;
		}
	}
	return NULL;
}

static struct dfont_rect *
insert_char(struct dfont *df, int c, int font, struct hash_rect *hr) {
	hr->c = c;
	hr->font = font;
	hr->version = df->version;
	list_add_tail(&hr->time, &df->time);
	int h = hash(c, font);
	hr->next_hash = df->hash[h];
	df->hash[h] = hr;
	return &hr->rect;
}

const struct dfont_rect * 
dfont_insert(struct dfont *df, int c, int font, int width, int height) {
	if (width > df->width)
		return NULL;
	assert(dfont_lookup(df,c,font) == NULL);
	for (;;) {
		struct font_line *line = find_line(df, width, height);
		if (line == NULL)
			break;
		struct hash_rect * hr = find_space(df, line, width);
		if (hr) {
			return insert_char(df,c,font,hr);
		}
	}
	struct hash_rect * hr = release_space(df, width, height);
	if (hr) {
		return insert_char(df,c,font,hr);
	}
	return NULL;
}

static void
dump_node(struct hash_rect *hr) {
	printf("(%d/%d : %d %d %d %d) ", hr->c, hr->font, hr->rect.x, hr->rect.y, hr->rect.w, hr->rect.h);
}

void 
dfont_dump(struct dfont * df) {
	printf("version = %d\n",df->version);
	printf("By version : ");
	struct hash_rect *hr;
	int version = -1;
	list_for_each_entry(hr, struct hash_rect, &df->time, time) {
		if (hr->version != version) {
			version = hr->version;
			printf("\nversion %d : ", version);
		}
		dump_node(hr);
	}
	printf("\n");
	printf("By line : \n");
	int i;
	for (i=0;i<df->max_line;i++) {
		struct font_line *line = &df->line[i];
		printf("line (y=%d h=%d space=%d) :",line->start_line, line->height,line->space);
		list_for_each_entry(hr, struct hash_rect, &line->head, next_char) {
			printf("%d(%d-%d) ",hr->c,hr->rect.x,hr->rect.x+hr->rect.w-1);
		}
		printf("\n");
	}
	printf("By hash : \n");
	for (i=0;i<HASH_SIZE;i++) {
		struct hash_rect *hr = df->hash[i];
		if (hr) {
			printf("%d : ",i);
			while (hr) {
				dump_node(hr);
				hr = hr->next_hash;
			}

			printf("\n");
		}
	}
}
