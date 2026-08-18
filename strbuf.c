/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unipaste.h"

void
strbuf_init(struct strbuf *sb, size_t initial_cap)
{
	if (initial_cap < 32)
		initial_cap = 32;

	sb->data = malloc(initial_cap);
	if (!sb->data) {
		fprintf(stderr, "unipaste: memory allocation failed\n");
		exit(1);
	}
	sb->data[0] = '\0';
	sb->len = 0;
	sb->cap = initial_cap;
}

void
strbuf_free(struct strbuf *sb)
{
	if (sb && sb->data) {
		free(sb->data);
		sb->data = NULL;
		sb->len = 0;
		sb->cap = 0;
	}
}

void
strbuf_reset(struct strbuf *sb)
{
	if (sb && sb->data) {
		sb->data[0] = '\0';
		sb->len = 0;
	}
}

static void
strbuf_grow(struct strbuf *sb, size_t needed)
{
	size_t new_cap;
	char *new_data;

	if (sb->len + needed + 1 <= sb->cap)
		return;

	new_cap = sb->cap * 2;
	if (new_cap < sb->len + needed + 1)
		new_cap = sb->len + needed + 1024;

	new_data = realloc(sb->data, new_cap);
	if (!new_data) {
		fprintf(stderr, "unipaste: memory allocation failed\n");
		exit(1);
	}
	sb->data = new_data;
	sb->cap = new_cap;
}

void
strbuf_putc(struct strbuf *sb, char c)
{
	strbuf_grow(sb, 1);
	sb->data[sb->len++] = c;
	sb->data[sb->len] = '\0';
}

void
strbuf_puts(struct strbuf *sb, const char *s)
{
	size_t slen;

	if (!s || !*s)
		return;
	slen = strlen(s);
	strbuf_grow(sb, slen);
	memcpy(sb->data + sb->len, s, slen);
	sb->len += slen;
	sb->data[sb->len] = '\0';
}

void
strbuf_append(struct strbuf *sb, const char *data, size_t len)
{
	if (!data || len == 0)
		return;
	strbuf_grow(sb, len);
	memcpy(sb->data + sb->len, data, len);
	sb->len += len;
	sb->data[sb->len] = '\0';
}
