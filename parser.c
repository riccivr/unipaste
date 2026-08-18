/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "unipaste.h"

/* Safe string duplicate */
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

/* Case-insensitive string comparison helper */
static int
ci_equal(const char *a, const char *b)
{
	while (*a && *b) {
		if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
			return 0;
		a++;
		b++;
	}
	return (*a == '\0' && *b == '\0');
}

/* Check if tag name matches */
static int
tag_is(const char *tag, const char *name)
{
	return ci_equal(tag, name);
}

/* Extract attribute value from tag string (e.g. href="...", class='...') */
static int
extract_attribute(const char *tag_str, const char *attr_name, char *val_out, size_t max_len)
{
	const char *p = tag_str;
	size_t attr_len;
	char quote;
	size_t i;

	if (!tag_str || !attr_name || !val_out || max_len == 0)
		return 0;

	attr_len = strlen(attr_name);
	val_out[0] = '\0';

	while (*p) {
		if (isspace((unsigned char)*p)) {
			p++;
			while (isspace((unsigned char)*p))
				p++;

			if (strncasecmp(p, attr_name, attr_len) == 0 && (p[attr_len] == '=' || isspace((unsigned char)p[attr_len]))) {
				p += attr_len;
				while (isspace((unsigned char)*p))
					p++;
				if (*p == '=') {
					p++;
					while (isspace((unsigned char)*p))
						p++;
					if (*p == '"' || *p == '\'') {
						quote = *p++;
						i = 0;
						while (*p && *p != quote && i < max_len - 1) {
							val_out[i++] = *p++;
						}
						val_out[i] = '\0';
						decode_html_entities_inplace(val_out);
						return 1;
					} else {
						/* Unquoted attribute */
						i = 0;
						while (*p && !isspace((unsigned char)*p) && *p != '>' && i < max_len - 1) {
							val_out[i++] = *p++;
						}
						val_out[i] = '\0';
						decode_html_entities_inplace(val_out);
						return 1;
					}
				}
			}
		} else {
			p++;
		}
	}
	return 0;
}

/* Emit newlines normalizing vertical whitespace */
static void
emit_newlines(struct parser_state *st, int count)
{
	int needed, i;

	if (st->current_table)
		return;

	needed = count - st->consecutive_newlines;
	if (needed <= 0)
		return;

	for (i = 0; i < needed; i++) {
		strbuf_putc(&st->outbuf, '\n');
		st->consecutive_newlines++;
	}
	st->at_line_start = 1;
	st->need_space = 0;
}

/* Emit indentation for blockquotes and lists */
static void
emit_indent(struct parser_state *st)
{
	int i;

	if (st->current_table || !st->at_line_start)
		return;

	/* Blockquotes */
	for (i = 0; i < st->quote_depth; i++)
		strbuf_puts(&st->outbuf, "> ");

	/* Lists */
	if (st->list_depth > 0) {
		for (i = 0; i < (st->list_depth - 1) * 2; i++)
			strbuf_putc(&st->outbuf, ' ');
	}

	st->at_line_start = 0;
}

/* Emit inline text to output or active buffer */
static void
emit_text(struct parser_state *st, const char *txt)
{
	if (!txt || !*txt)
		return;

	if (st->current_table) {
		if (st->need_space && st->textbuf.len > 0) {
			strbuf_putc(&st->textbuf, ' ');
		}
		st->need_space = 0;
		strbuf_puts(&st->textbuf, txt);
		return;
	}

	if (st->in_link) {
		if (st->need_space && st->link_text.len > 0) {
			strbuf_putc(&st->link_text, ' ');
		}
		st->need_space = 0;
		strbuf_puts(&st->link_text, txt);
		return;
	}

	if (st->heading_level > 0) {
		if (st->need_space && st->heading_text.len > 0) {
			strbuf_putc(&st->heading_text, ' ');
		}
		st->need_space = 0;
		strbuf_puts(&st->heading_text, txt);
		return;
	}

	if (st->at_line_start)
		emit_indent(st);

	if (st->need_space && !st->at_line_start && !st->pre_depth) {
		strbuf_putc(&st->outbuf, ' ');
		st->need_space = 0;
	}

	strbuf_puts(&st->outbuf, txt);
	st->consecutive_newlines = 0;
	st->at_line_start = 0;
}

/* Flush heading when closing h1-h6 */
static void
flush_heading(struct parser_state *st)
{
	int i, len;

	if (st->heading_level == 0)
		return;

	emit_newlines(st, 2);
	emit_indent(st);

	switch (st->cfg->mode) {
	case MODE_MARKDOWN:
		for (i = 0; i < st->heading_level; i++)
			strbuf_putc(&st->outbuf, '#');
		strbuf_putc(&st->outbuf, ' ');
		strbuf_puts(&st->outbuf, st->heading_text.data);
		break;
	case MODE_TERMINAL:
		strbuf_puts(&st->outbuf, "\033[1;36m"); /* Bold Cyan */
		for (i = 0; i < st->heading_level; i++)
			strbuf_putc(&st->outbuf, '#');
		strbuf_putc(&st->outbuf, ' ');
		strbuf_puts(&st->outbuf, st->heading_text.data);
		strbuf_puts(&st->outbuf, "\033[0m");
		break;
	case MODE_PLAIN:
	default:
		if (st->heading_level == 1) {
			strbuf_puts(&st->outbuf, st->heading_text.data);
			strbuf_putc(&st->outbuf, '\n');
			len = (int)st->heading_text.len;
			for (i = 0; i < len; i++)
				strbuf_putc(&st->outbuf, '=');
		} else if (st->heading_level == 2) {
			strbuf_puts(&st->outbuf, st->heading_text.data);
			strbuf_putc(&st->outbuf, '\n');
			len = (int)st->heading_text.len;
			for (i = 0; i < len; i++)
				strbuf_putc(&st->outbuf, '-');
		} else {
			for (i = 0; i < st->heading_level; i++)
				strbuf_putc(&st->outbuf, '#');
			strbuf_putc(&st->outbuf, ' ');
			strbuf_puts(&st->outbuf, st->heading_text.data);
		}
		break;
	}

	st->consecutive_newlines = 0;
	emit_newlines(st, 2);
	st->heading_level = 0;
	strbuf_reset(&st->heading_text);
}

/* Flush link when closing <a> */
static void
flush_link(struct parser_state *st)
{
	char *txt;
	char fn_buf[16];
	enum link_style lstyle;

	if (!st->in_link)
		return;

	txt = st->link_text.data;
	lstyle = st->cfg->link_style;

	if (lstyle == LINK_STYLE_AUTO) {
		if (st->cfg->mode == MODE_MARKDOWN)
			lstyle = LINK_STYLE_INLINE;
		else
			lstyle = LINK_STYLE_BRACKET;
	}

	if (st->at_line_start)
		emit_indent(st);

	if (st->need_space && !st->at_line_start) {
		strbuf_putc(&st->outbuf, ' ');
		st->need_space = 0;
	}

	if (st->current_href[0] == '\0' || lstyle == LINK_STYLE_TEXTONLY) {
		strbuf_puts(&st->outbuf, txt);
	} else if (lstyle == LINK_STYLE_INLINE) {
		/* [text](url) */
		if (strcmp(txt, st->current_href) == 0) {
			strbuf_puts(&st->outbuf, "<");
			strbuf_puts(&st->outbuf, st->current_href);
			strbuf_puts(&st->outbuf, ">");
		} else {
			strbuf_puts(&st->outbuf, "[");
			strbuf_puts(&st->outbuf, txt);
			strbuf_puts(&st->outbuf, "](");
			strbuf_puts(&st->outbuf, st->current_href);
			strbuf_puts(&st->outbuf, ")");
		}
	} else if (lstyle == LINK_STYLE_FOOTNOTE) {
		if (st->num_footnotes < 255) {
			st->footnotes[st->num_footnotes] = xstrdup(st->current_href);
			st->num_footnotes++;
			strbuf_puts(&st->outbuf, txt);
			snprintf(fn_buf, sizeof(fn_buf), " [%d]", st->num_footnotes);
			strbuf_puts(&st->outbuf, fn_buf);
		} else {
			strbuf_puts(&st->outbuf, txt);
		}
	} else {
		/* LINK_STYLE_BRACKET: text (url) */
		if (strcmp(txt, st->current_href) == 0) {
			strbuf_puts(&st->outbuf, st->current_href);
		} else {
			strbuf_puts(&st->outbuf, txt);
			strbuf_puts(&st->outbuf, " (");
			strbuf_puts(&st->outbuf, st->current_href);
			strbuf_puts(&st->outbuf, ")");
		}
	}

	st->consecutive_newlines = 0;
	st->in_link = 0;
	st->current_href[0] = '\0';
	strbuf_reset(&st->link_text);
}

/* Handle opening HTML tag */
static void
handle_open_tag(struct parser_state *st, const char *tag_str)
{
	char name[MAX_TAG_NAME];
	const char *p = tag_str;
	size_t i = 0;
	char attr_val[MAX_ATTR_VAL];

	while (*p && !isspace((unsigned char)*p) && *p != '>' && *p != '/' && i < MAX_TAG_NAME - 1)
		name[i++] = *p++;
	name[i] = '\0';

	/* Block level structural tags */
	if (tag_is(name, "p") || tag_is(name, "div") || tag_is(name, "article") ||
	    tag_is(name, "section") || tag_is(name, "main") || tag_is(name, "header") ||
	    tag_is(name, "footer")) {
		emit_newlines(st, 2);
	} else if (tag_is(name, "br")) {
		if (st->current_table) {
			strbuf_puts(&st->textbuf, " ");
		} else {
			strbuf_putc(&st->outbuf, '\n');
			st->consecutive_newlines = 1;
			st->at_line_start = 1;
			st->need_space = 0;
		}
	} else if (tag_is(name, "hr")) {
		emit_newlines(st, 2);
		emit_indent(st);
		if (st->cfg->mode == MODE_MARKDOWN)
			strbuf_puts(&st->outbuf, "---\n");
		else
			strbuf_puts(&st->outbuf, "----------------------------------------\n");
		st->consecutive_newlines = 1;
		st->at_line_start = 1;
	} else if (tag_is(name, "h1") || tag_is(name, "h2") || tag_is(name, "h3") ||
	           tag_is(name, "h4") || tag_is(name, "h5") || tag_is(name, "h6")) {
		st->heading_level = name[1] - '0';
		strbuf_reset(&st->heading_text);
		st->need_space = 0;
	} else if (tag_is(name, "ul")) {
		if (st->list_depth < MAX_LIST_DEPTH) {
			st->list_type[st->list_depth] = 1; /* UL */
			st->list_count[st->list_depth] = 0;
			st->list_depth++;
		}
		emit_newlines(st, 1);
	} else if (tag_is(name, "ol")) {
		if (st->list_depth < MAX_LIST_DEPTH) {
			st->list_type[st->list_depth] = 2; /* OL */
			st->list_count[st->list_depth] = 1;
			st->list_depth++;
		}
		emit_newlines(st, 1);
	} else if (tag_is(name, "li")) {
		char numbuf[32];
		emit_newlines(st, 1);
		emit_indent(st);
		if (st->list_depth > 0 && st->list_type[st->list_depth - 1] == 2) {
			/* Ordered list */
			snprintf(numbuf, sizeof(numbuf), "%d. ", st->list_count[st->list_depth - 1]++);
			strbuf_puts(&st->outbuf, numbuf);
		} else {
			/* Unordered list */
			strbuf_puts(&st->outbuf, "* ");
		}
		st->consecutive_newlines = 0;
		st->at_line_start = 0;
		st->need_space = 0;
	} else if (tag_is(name, "blockquote")) {
		st->quote_depth++;
		emit_newlines(st, 2);
	} else if (tag_is(name, "pre")) {
		st->pre_depth++;
		emit_newlines(st, 2);
		if (st->cfg->mode == MODE_MARKDOWN) {
			if (extract_attribute(tag_str, "data-lang", attr_val, sizeof(attr_val)) ||
			    extract_attribute(tag_str, "class", attr_val, sizeof(attr_val))) {
				char *lang = strstr(attr_val, "language-");
				if (lang)
					lang += 9;
				else
					lang = attr_val;
				strbuf_puts(&st->outbuf, "```");
				strbuf_puts(&st->outbuf, lang);
				strbuf_putc(&st->outbuf, '\n');
			} else {
				strbuf_puts(&st->outbuf, "```\n");
			}
			st->consecutive_newlines = 1;
			st->at_line_start = 1;
		}
	} else if (tag_is(name, "code")) {
		st->code_depth++;
		if (st->pre_depth > 0 && st->cfg->mode == MODE_MARKDOWN) {
			/* If pre opened without language, check code tag */
			if (st->outbuf.len >= 4 && strcmp(st->outbuf.data + st->outbuf.len - 4, "```\n") == 0) {
				if (extract_attribute(tag_str, "data-lang", attr_val, sizeof(attr_val)) ||
				    extract_attribute(tag_str, "class", attr_val, sizeof(attr_val))) {
					char *lang = strstr(attr_val, "language-");
					if (lang)
						lang += 9;
					else
						lang = attr_val;
					/* Replace ```\n with ```lang\n */
					st->outbuf.len -= 4;
					st->outbuf.data[st->outbuf.len] = '\0';
					strbuf_puts(&st->outbuf, "```");
					strbuf_puts(&st->outbuf, lang);
					strbuf_putc(&st->outbuf, '\n');
				}
			}
		} else if (!st->pre_depth) {
			if (st->cfg->mode == MODE_MARKDOWN)
				emit_text(st, "`");
		}
	} else if (tag_is(name, "a")) {
		if (st->need_space && !st->at_line_start && !st->current_table) {
			strbuf_putc(&st->outbuf, ' ');
			st->need_space = 0;
		}
		if (extract_attribute(tag_str, "href", st->current_href, sizeof(st->current_href))) {
			st->in_link = 1;
			strbuf_reset(&st->link_text);
		}
	} else if (tag_is(name, "table")) {
		if (st->current_table) {
			table_render(st->current_table, &st->outbuf, st->cfg);
			table_free(st->current_table);
			st->current_table = NULL;
		}
		emit_newlines(st, 2);
		st->current_table = table_create();
	} else if (tag_is(name, "tr")) {
		if (st->current_table)
			table_add_row(st->current_table);
	} else if (tag_is(name, "th") || tag_is(name, "td")) {
		strbuf_reset(&st->textbuf);
		st->need_space = 0;
	} else if (tag_is(name, "b") || tag_is(name, "strong")) {
		st->bold_depth++;
		if (st->cfg->mode == MODE_MARKDOWN && !st->current_table)
			emit_text(st, "**");
		else if (st->cfg->mode == MODE_TERMINAL)
			emit_text(st, "\033[1m");
	} else if (tag_is(name, "i") || tag_is(name, "em")) {
		st->italic_depth++;
		if (st->cfg->mode == MODE_MARKDOWN && !st->current_table)
			emit_text(st, "*");
		else if (st->cfg->mode == MODE_TERMINAL)
			emit_text(st, "\033[3m");
	} else if (tag_is(name, "s") || tag_is(name, "del") || tag_is(name, "strike")) {
		st->strike_depth++;
		if (st->cfg->mode == MODE_MARKDOWN && !st->current_table)
			emit_text(st, "~~");
	} else if (tag_is(name, "input")) {
		if (strstr(tag_str, "type=\"checkbox\"") || strstr(tag_str, "type='checkbox'")) {
			if (strstr(tag_str, "checked"))
				emit_text(st, "[x]");
			else
				emit_text(st, "[ ]");
			st->need_space = 1;
		}
	}
}

/* Handle closing HTML tag */
static void
handle_close_tag(struct parser_state *st, const char *tag_str)
{
	char name[MAX_TAG_NAME];
	const char *p = tag_str;
	size_t i = 0;

	while (*p && !isspace((unsigned char)*p) && *p != '>' && i < MAX_TAG_NAME - 1)
		name[i++] = *p++;
	name[i] = '\0';

	if (tag_is(name, "p") || tag_is(name, "div") || tag_is(name, "article") ||
	    tag_is(name, "section") || tag_is(name, "main") || tag_is(name, "header") ||
	    tag_is(name, "footer")) {
		emit_newlines(st, 2);
	} else if (tag_is(name, "h1") || tag_is(name, "h2") || tag_is(name, "h3") ||
	           tag_is(name, "h4") || tag_is(name, "h5") || tag_is(name, "h6")) {
		flush_heading(st);
	} else if (tag_is(name, "ul") || tag_is(name, "ol")) {
		if (st->list_depth > 0)
			st->list_depth--;
		emit_newlines(st, 1);
	} else if (tag_is(name, "li")) {
		emit_newlines(st, 1);
	} else if (tag_is(name, "blockquote")) {
		if (st->quote_depth > 0)
			st->quote_depth--;
		emit_newlines(st, 2);
	} else if (tag_is(name, "pre")) {
		if (st->pre_depth > 0) {
			st->pre_depth--;
			if (st->cfg->mode == MODE_MARKDOWN) {
				emit_newlines(st, 1);
				strbuf_puts(&st->outbuf, "```\n");
				st->consecutive_newlines = 1;
				st->at_line_start = 1;
			}
			emit_newlines(st, 2);
		}
	} else if (tag_is(name, "code")) {
		if (st->code_depth > 0) {
			st->code_depth--;
			if (!st->pre_depth && st->cfg->mode == MODE_MARKDOWN)
				emit_text(st, "`");
		}
	} else if (tag_is(name, "a")) {
		flush_link(st);
	} else if (tag_is(name, "table")) {
		if (st->current_table) {
			table_render(st->current_table, &st->outbuf, st->cfg);
			table_free(st->current_table);
			st->current_table = NULL;
			st->consecutive_newlines = 1;
			emit_newlines(st, 2);
		}
	} else if (tag_is(name, "th") || tag_is(name, "td")) {
		if (st->current_table)
			table_add_cell(st->current_table, st->textbuf.data, tag_is(name, "th"));
		strbuf_reset(&st->textbuf);
		st->need_space = 0;
	} else if (tag_is(name, "b") || tag_is(name, "strong")) {
		if (st->bold_depth > 0) {
			st->bold_depth--;
			if (st->cfg->mode == MODE_MARKDOWN && !st->current_table)
				emit_text(st, "**");
			else if (st->cfg->mode == MODE_TERMINAL)
				emit_text(st, "\033[0m");
		}
	} else if (tag_is(name, "i") || tag_is(name, "em")) {
		if (st->italic_depth > 0) {
			st->italic_depth--;
			if (st->cfg->mode == MODE_MARKDOWN && !st->current_table)
				emit_text(st, "*");
			else if (st->cfg->mode == MODE_TERMINAL)
				emit_text(st, "\033[0m");
		}
	} else if (tag_is(name, "s") || tag_is(name, "del") || tag_is(name, "strike")) {
		if (st->strike_depth > 0) {
			st->strike_depth--;
			if (st->cfg->mode == MODE_MARKDOWN && !st->current_table)
				emit_text(st, "~~");
		}
	}
}

/* Locate StartFragment / EndFragment if present in Windows CF_HTML */
static const char *
find_fragment_start(const char *src)
{
	const char *marker = "<!--StartFragment-->";
	const char *p = strstr(src, marker);
	if (p)
		return p + strlen(marker);
	return src;
}

static size_t
find_fragment_len(const char *src, size_t total_len)
{
	const char *marker = "<!--EndFragment-->";
	const char *p = strstr(src, marker);
	if (p && p >= src)
		return (size_t)(p - src);
	return total_len;
}

/* Main parsing loop for HTML input buffer */
int
unipaste_process_string(const char *input, size_t len, FILE *out, const struct config *cfg)
{
	struct parser_state st;
	const char *p, *end;
	char entity_buf[16];
	size_t consumed;
	int in_tag = 0;
	int in_script_or_style = 0;
	char char_buf[8];
	int i;

	if (!input || !out || !cfg)
		return 0;

	memset(&st, 0, sizeof(st));
	st.cfg = cfg;
	st.out = out;
	st.consecutive_newlines = 2; /* Treat start as beginning of fresh paragraph */
	st.at_line_start = 1;

	strbuf_init(&st.outbuf, len ? len * 2 : 4096);
	strbuf_init(&st.linebuf, 1024);
	strbuf_init(&st.tagbuf, 256);
	strbuf_init(&st.textbuf, 1024);
	strbuf_init(&st.link_text, 256);
	strbuf_init(&st.heading_text, 256);

	/* Check for Windows CF_HTML fragments */
	p = find_fragment_start(input);
	if (p != input) {
		len = find_fragment_len(p, len - (p - input));
	}
	end = p + len;

	while (p < end && *p) {
		/* HTML comment parsing */
		if (!in_tag && !in_script_or_style && *p == '<' && p + 3 < end && p[1] == '!' && p[2] == '-' && p[3] == '-') {
			p += 4;
			while (p + 2 < end && !(p[0] == '-' && p[1] == '-' && p[2] == '>'))
				p++;
			if (p + 2 < end)
				p += 3;
			else
				p = end;
			continue;
		}

		/* Tag Start */
		if (*p == '<') {
			in_tag = 1;
			strbuf_reset(&st.tagbuf);
			p++;
			while (p < end && *p != '>') {
				if (st.tagbuf.len < 65536)
					strbuf_putc(&st.tagbuf, *p);
				p++;
			}
			if (p < end && *p == '>')
				p++;

			in_tag = 0;

			/* Check script/style boundary */
			if (tag_is(st.tagbuf.data, "script") || tag_is(st.tagbuf.data, "style") || tag_is(st.tagbuf.data, "head")) {
				in_script_or_style = 1;
			} else if (tag_is(st.tagbuf.data, "/script") || tag_is(st.tagbuf.data, "/style") || tag_is(st.tagbuf.data, "/head")) {
				in_script_or_style = 0;
			} else if (!in_script_or_style) {
				if (st.tagbuf.data[0] == '/')
					handle_close_tag(&st, st.tagbuf.data + 1);
				else
					handle_open_tag(&st, st.tagbuf.data);
			}
			continue;
		}

		if (in_script_or_style) {
			p++;
			continue;
		}

		/* Preformatted text: preserve whitespace verbatim */
		if (st.pre_depth > 0) {
			if (*p == '&') {
				consumed = decode_html_entity(p, entity_buf, sizeof(entity_buf));
				if (consumed > 0) {
					emit_text(&st, entity_buf);
					p += consumed;
					continue;
				}
			}
			if (*p == '\n') {
				strbuf_putc(&st.outbuf, '\n');
				st.consecutive_newlines = 1;
				st.at_line_start = 1;
			} else {
				char_buf[0] = *p;
				char_buf[1] = '\0';
				emit_text(&st, char_buf);
			}
			p++;
			continue;
		}

		/* Normal text whitespace handling: collapse multiple whitespace into single space */
		if (isspace((unsigned char)*p)) {
			st.need_space = 1;
			p++;
			while (p < end && isspace((unsigned char)*p))
				p++;
			continue;
		}

		/* HTML entity decoding */
		if (*p == '&') {
			consumed = decode_html_entity(p, entity_buf, sizeof(entity_buf));
			if (consumed > 0) {
				emit_text(&st, entity_buf);
				p += consumed;
				continue;
			}
		}

		/* Normal character */
		char_buf[0] = *p++;
		char_buf[1] = '\0';
		emit_text(&st, char_buf);
	}

	/* Emit any pending footnotes */
	if (st.num_footnotes > 0) {
		emit_newlines(&st, 2);
		for (i = 0; i < st.num_footnotes; i++) {
			char fn_header[32];
			snprintf(fn_header, sizeof(fn_header), "[%d] ", i + 1);
			strbuf_puts(&st.outbuf, fn_header);
			if (st.footnotes[i])
				strbuf_puts(&st.outbuf, st.footnotes[i]);
			strbuf_putc(&st.outbuf, '\n');
			free(st.footnotes[i]);
			st.footnotes[i] = NULL;
		}
	}

	/* Flush any unclosed table */
	if (st.current_table) {
		table_render(st.current_table, &st.outbuf, st.cfg);
		table_free(st.current_table);
		st.current_table = NULL;
	}

	/* Ensure trailing newline */
	if (st.outbuf.len > 0 && st.outbuf.data[st.outbuf.len - 1] != '\n')
		strbuf_putc(&st.outbuf, '\n');

	/* Output result with CRLF conversion if requested */
	if (cfg->crlf) {
		for (size_t j = 0; j < st.outbuf.len; j++) {
			if (st.outbuf.data[j] == '\n' && (j == 0 || st.outbuf.data[j - 1] != '\r'))
				fputc('\r', out);
			fputc(st.outbuf.data[j], out);
		}
	} else {
		fputs(st.outbuf.data, out);
	}

	/* Clean up */
	strbuf_free(&st.outbuf);
	strbuf_free(&st.linebuf);
	strbuf_free(&st.tagbuf);
	strbuf_free(&st.textbuf);
	strbuf_free(&st.link_text);
	strbuf_free(&st.heading_text);

	return 0;
}

/* Process input from stream */
int
unipaste_process_stream(FILE *in, FILE *out, const struct config *cfg)
{
	struct strbuf input_buf;
	char chunk[4096];
	size_t n;
	int res;

	if (!in || !out || !cfg)
		return 1;

	strbuf_init(&input_buf, 8192);
	while ((n = fread(chunk, 1, sizeof(chunk), in)) > 0) {
		strbuf_append(&input_buf, chunk, n);
	}

	res = unipaste_process_string(input_buf.data, input_buf.len, out, cfg);
	strbuf_free(&input_buf);
	return res;
}
