/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "unipaste.h"

/* Helper to duplicate string safely */
static char *
xstrdup(const char *s)
{
	char *res;
	size_t len;

	if (!s)
		return NULL;
	len = strlen(s);
	res = malloc(len + 1);
	if (!res) {
		fprintf(stderr, "unipaste: memory allocation failed\n");
		exit(1);
	}
	memcpy(res, s, len + 1);
	return res;
}

/* Trim leading and trailing whitespace from string */
static void
trim_whitespace(char *s)
{
	char *start, *end;

	if (!s || !*s)
		return;

	start = s;
	while (*start && isspace((unsigned char)*start))
		start++;

	end = s + strlen(s) - 1;
	while (end > start && isspace((unsigned char)*end)) {
		*end = '\0';
		end--;
	}

	if (start > s)
		memmove(s, start, strlen(start) + 1);
}

struct table *
table_create(void)
{
	struct table *t = calloc(1, sizeof(*t));
	if (!t) {
		fprintf(stderr, "unipaste: memory allocation failed\n");
		exit(1);
	}
	return t;
}

void
table_free(struct table *t)
{
	int r, c;

	if (!t)
		return;

	for (r = 0; r < t->num_rows; r++) {
		for (c = 0; c < t->num_cols; c++) {
			if (t->cells[r][c]) {
				free(t->cells[r][c]->text);
				free(t->cells[r][c]);
			}
		}
	}
	free(t);
}

void
table_add_row(struct table *t)
{
	if (!t || t->num_rows >= MAX_TABLE_ROWS)
		return;

	t->num_rows++;
	t->current_col = 0;
}

void
table_add_cell(struct table *t, const char *text, int is_header)
{
	struct table_cell *cell;
	char *cleaned;

	if (!t || t->num_rows == 0 || t->current_col >= MAX_TABLE_COLS)
		return;

	cleaned = xstrdup(text ? text : "");
	trim_whitespace(cleaned);

	cell = malloc(sizeof(*cell));
	if (!cell) {
		fprintf(stderr, "unipaste: memory allocation failed\n");
		exit(1);
	}
	cell->text = cleaned;
	cell->is_header = is_header;
	cell->colspan = 1;
	cell->rowspan = 1;

	t->cells[t->num_rows - 1][t->current_col] = cell;
	t->current_col++;
	if (t->current_col > t->num_cols)
		t->num_cols = t->current_col;
}

/* Calculate column widths */
static void
table_calculate_widths(struct table *t)
{
	int r, c;
	size_t len;

	for (c = 0; c < t->num_cols; c++)
		t->col_widths[c] = 3; /* minimum width */

	for (r = 0; r < t->num_rows; r++) {
		for (c = 0; c < t->num_cols; c++) {
			if (t->cells[r][c] && t->cells[r][c]->text) {
				len = strlen(t->cells[r][c]->text);
				if ((int)len > t->col_widths[c])
					t->col_widths[c] = (int)len;
			}
		}
	}
}

/* Render horizontal separator for ASCII grid table */
static void
render_grid_sep(struct strbuf *out, const struct table *t, char left, char mid, char right, char fill)
{
	int c, i;

	strbuf_putc(out, left);
	for (c = 0; c < t->num_cols; c++) {
		for (i = 0; i < t->col_widths[c] + 2; i++)
			strbuf_putc(out, fill);
		strbuf_putc(out, (c == t->num_cols - 1) ? right : mid);
	}
	strbuf_puts(out, "\n");
}

/* Render ASCII / Unicode Grid Table */
static void
render_grid(const struct table *t, struct strbuf *out, int unicode)
{
	int r, c, i, pad;
	const char *txt;

	if (unicode) {
		/* Unicode table box-drawing */
		strbuf_puts(out, "┌");
		for (c = 0; c < t->num_cols; c++) {
			for (i = 0; i < t->col_widths[c] + 2; i++)
				strbuf_puts(out, "─");
			strbuf_puts(out, (c == t->num_cols - 1) ? "┐\n" : "┬");
		}

		for (r = 0; r < t->num_rows; r++) {
			strbuf_puts(out, "│");
			for (c = 0; c < t->num_cols; c++) {
				txt = (t->cells[r][c] && t->cells[r][c]->text) ? t->cells[r][c]->text : "";
				strbuf_puts(out, " ");
				strbuf_puts(out, txt);
				pad = t->col_widths[c] - (int)strlen(txt) + 1;
				for (i = 0; i < pad; i++)
					strbuf_puts(out, " ");
				strbuf_puts(out, "│");
			}
			strbuf_puts(out, "\n");

			if (r == 0 && t->num_rows > 1) {
				strbuf_puts(out, "├");
				for (c = 0; c < t->num_cols; c++) {
					for (i = 0; i < t->col_widths[c] + 2; i++)
						strbuf_puts(out, "─");
					strbuf_puts(out, (c == t->num_cols - 1) ? "┤\n" : "┼");
				}
			}
		}

		strbuf_puts(out, "└");
		for (c = 0; c < t->num_cols; c++) {
			for (i = 0; i < t->col_widths[c] + 2; i++)
				strbuf_puts(out, "─");
			strbuf_puts(out, (c == t->num_cols - 1) ? "┘\n" : "┴");
		}
	} else {
		/* Standard ASCII grid */
		render_grid_sep(out, t, '+', '+', '+', '-');

		for (r = 0; r < t->num_rows; r++) {
			strbuf_puts(out, "|");
			for (c = 0; c < t->num_cols; c++) {
				txt = (t->cells[r][c] && t->cells[r][c]->text) ? t->cells[r][c]->text : "";
				strbuf_puts(out, " ");
				strbuf_puts(out, txt);
				pad = t->col_widths[c] - (int)strlen(txt) + 1;
				for (i = 0; i < pad; i++)
					strbuf_putc(out, ' ');
				strbuf_puts(out, "|");
			}
			strbuf_puts(out, "\n");

			if (r == 0 && t->num_rows > 1)
				render_grid_sep(out, t, '+', '+', '+', '-');
		}

		render_grid_sep(out, t, '+', '+', '+', '-');
	}
}

/* Render Markdown Table */
static void
render_markdown(const struct table *t, struct strbuf *out)
{
	int r, c, i, pad;
	const char *txt;

	for (r = 0; r < t->num_rows; r++) {
		strbuf_puts(out, "|");
		for (c = 0; c < t->num_cols; c++) {
			txt = (t->cells[r][c] && t->cells[r][c]->text) ? t->cells[r][c]->text : "";
			strbuf_puts(out, " ");
			strbuf_puts(out, txt);
			pad = t->col_widths[c] - (int)strlen(txt) + 1;
			for (i = 0; i < pad; i++)
				strbuf_putc(out, ' ');
			strbuf_puts(out, "|");
		}
		strbuf_puts(out, "\n");

		/* Output header separator row after row 0 */
		if (r == 0) {
			strbuf_puts(out, "|");
			for (c = 0; c < t->num_cols; c++) {
				strbuf_puts(out, " ");
				for (i = 0; i < t->col_widths[c]; i++)
					strbuf_putc(out, '-');
				strbuf_puts(out, " |");
			}
			strbuf_puts(out, "\n");
		}
	}
}

/* Render TSV Table */
static void
render_tsv(const struct table *t, struct strbuf *out)
{
	int r, c;
	const char *txt;

	for (r = 0; r < t->num_rows; r++) {
		for (c = 0; c < t->num_cols; c++) {
			txt = (t->cells[r][c] && t->cells[r][c]->text) ? t->cells[r][c]->text : "";
			strbuf_puts(out, txt);
			if (c < t->num_cols - 1)
				strbuf_putc(out, '\t');
		}
		strbuf_puts(out, "\n");
	}
}

/* Render Simple Aligned Columns */
static void
render_simple(const struct table *t, struct strbuf *out)
{
	int r, c, i, pad;
	const char *txt;

	for (r = 0; r < t->num_rows; r++) {
		for (c = 0; c < t->num_cols; c++) {
			txt = (t->cells[r][c] && t->cells[r][c]->text) ? t->cells[r][c]->text : "";
			strbuf_puts(out, txt);
			if (c < t->num_cols - 1) {
				pad = t->col_widths[c] - (int)strlen(txt) + 2;
				for (i = 0; i < pad; i++)
					strbuf_putc(out, ' ');
			}
		}
		strbuf_puts(out, "\n");

		if (r == 0 && t->num_rows > 1) {
			for (c = 0; c < t->num_cols; c++) {
				for (i = 0; i < t->col_widths[c]; i++)
					strbuf_putc(out, '-');
				if (c < t->num_cols - 1)
					strbuf_puts(out, "  ");
			}
			strbuf_puts(out, "\n");
		}
	}
}

void
table_render(struct table *t, struct strbuf *out, const struct config *cfg)
{
	enum table_style style;

	if (!t || t->num_rows == 0 || t->num_cols == 0)
		return;

	table_calculate_widths(t);

	style = cfg->table_style;
	if (style == TABLE_STYLE_AUTO) {
		if (cfg->mode == MODE_MARKDOWN)
			style = TABLE_STYLE_MARKDOWN;
		else
			style = TABLE_STYLE_GRID;
	}

	switch (style) {
	case TABLE_STYLE_MARKDOWN:
		render_markdown(t, out);
		break;
	case TABLE_STYLE_TSV:
		render_tsv(t, out);
		break;
	case TABLE_STYLE_SIMPLE:
		render_simple(t, out);
		break;
	case TABLE_STYLE_GRID:
	default:
		render_grid(t, out, cfg->unicode_tables);
		break;
	}
}
