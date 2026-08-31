/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "unipaste.h"
#include "plugin.h"

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

/* Case-insensitive bounded prefix comparison helper */
static int
ci_n_equal(const char *a, const char *b, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++) {
		if (!a[i] || !b[i])
			return (a[i] == b[i]);
		if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
			return 0;
	}
	return 1;
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

			if (ci_n_equal(p, attr_name, attr_len) && (p[attr_len] == '=' || isspace((unsigned char)p[attr_len]))) {
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

/* Check if query parameter key is a known telemetry / tracking key */
static int
is_tracking_param(const char *key, size_t key_len)
{
	static const char *const exact_params[] = {
		"fbclid", "gclid", "gbraid", "wbraid", "dclid", "msclkid",
		"twclid", "igshid", "mc_cid", "mc_eid", "_hsenc", "_hsmi",
		"_openstat", "vero_id", "vero_conv", "yclid", "si", "feature",
		"rcm", "trk", "trackingId", "ref", "mkt_tok"
	};
	size_t i;

	if (key_len >= 4 && strncmp(key, "utm_", 4) == 0)
		return 1;
	if (key_len >= 4 && strncmp(key, "ref_", 4) == 0)
		return 1;

	for (i = 0; i < sizeof(exact_params) / sizeof(exact_params[0]); i++) {
		if (strlen(exact_params[i]) == key_len &&
		    strncmp(key, exact_params[i], key_len) == 0)
			return 1;
	}

	return 0;
}

/* Strip tracking query parameters from URL in-place */
void
url_strip_tracking_inplace(char *url)
{
	char *qmark, *hash, *p, *param_start;
	char cleaned_query[MAX_ATTR_VAL];
	size_t cleaned_len = 0;
	int has_kept_param = 0;
	size_t hash_len = 0;
	char hash_buf[MAX_ATTR_VAL];

	if (!url || !*url)
		return;

	qmark = strchr(url, '?');
	if (!qmark)
		return;

	hash = strchr(qmark, '#');
	if (hash) {
		hash_len = strlen(hash);
		if (hash_len < sizeof(hash_buf)) {
			memcpy(hash_buf, hash, hash_len + 1);
		} else {
			hash_buf[0] = '\0';
			hash_len = 0;
		}
		*hash = '\0';
	}

	p = qmark + 1;
	while (*p) {
		param_start = p;
		while (*p && *p != '&' && *p != ';')
			p++;

		char delim = *p;
		size_t param_len = (size_t)(p - param_start);

		if (param_len > 0) {
			const char *eq = memchr(param_start, '=', param_len);
			size_t key_len = eq ? (size_t)(eq - param_start) : param_len;

			if (!is_tracking_param(param_start, key_len)) {
				if (has_kept_param) {
					if (cleaned_len + 1 < sizeof(cleaned_query))
						cleaned_query[cleaned_len++] = '&';
				}
				if (cleaned_len + param_len < sizeof(cleaned_query)) {
					memcpy(cleaned_query + cleaned_len, param_start, param_len);
					cleaned_len += param_len;
				}
				has_kept_param = 1;
			}
		}

		if (delim == '&' || delim == ';')
			p++;
	}

	cleaned_query[cleaned_len] = '\0';

	if (has_kept_param && cleaned_len > 0) {
		*qmark = '?';
		memcpy(qmark + 1, cleaned_query, cleaned_len);
		qmark += 1 + cleaned_len;
	}

	if (hash_len > 0) {
		memcpy(qmark, hash_buf, hash_len);
		qmark += hash_len;
	}
	*qmark = '\0';
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
	st->need_space = 0;
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
	case MODE_SLACK:
		strbuf_putc(&st->outbuf, '*');
		strbuf_puts(&st->outbuf, st->heading_text.data);
		strbuf_putc(&st->outbuf, '*');
		break;
	case MODE_JIRA: {
		char hbuf[16];
		snprintf(hbuf, sizeof(hbuf), "h%d. ", st->heading_level);
		strbuf_puts(&st->outbuf, hbuf);
		strbuf_puts(&st->outbuf, st->heading_text.data);
		break;
	}
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
	} else if (st->cfg->mode == MODE_SLACK) {
		/* Slack mrkdwn link: <url|text> or <url> */
		if (strcmp(txt, st->current_href) == 0) {
			strbuf_puts(&st->outbuf, "<");
			strbuf_puts(&st->outbuf, st->current_href);
			strbuf_puts(&st->outbuf, ">");
		} else {
			strbuf_puts(&st->outbuf, "<");
			strbuf_puts(&st->outbuf, st->current_href);
			strbuf_puts(&st->outbuf, "|");
			strbuf_puts(&st->outbuf, txt);
			strbuf_puts(&st->outbuf, ">");
		}
	} else if (st->cfg->mode == MODE_JIRA) {
		/* Jira wiki link: [text|url] or [url] */
		if (strcmp(txt, st->current_href) == 0) {
			strbuf_puts(&st->outbuf, "[");
			strbuf_puts(&st->outbuf, st->current_href);
			strbuf_puts(&st->outbuf, "]");
		} else {
			strbuf_puts(&st->outbuf, "[");
			strbuf_puts(&st->outbuf, txt);
			strbuf_puts(&st->outbuf, "|");
			strbuf_puts(&st->outbuf, st->current_href);
			strbuf_puts(&st->outbuf, "]");
		}
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
		if (st->cfg->mode == MODE_MARKDOWN || st->cfg->mode == MODE_SLACK || st->cfg->mode == MODE_JIRA) {
			if (extract_attribute(tag_str, "data-lang", attr_val, sizeof(attr_val)) ||
			    extract_attribute(tag_str, "class", attr_val, sizeof(attr_val))) {
				char *lang = strstr(attr_val, "language-");
				if (lang)
					lang += 9;
				else
					lang = attr_val;
				if (st->cfg->mode == MODE_JIRA) {
					strbuf_puts(&st->outbuf, "{code:");
					strbuf_puts(&st->outbuf, lang);
					strbuf_puts(&st->outbuf, "}\n");
				} else if (st->cfg->mode == MODE_SLACK) {
					strbuf_puts(&st->outbuf, "```\n");
				} else {
					strbuf_puts(&st->outbuf, "```");
					strbuf_puts(&st->outbuf, lang);
					strbuf_putc(&st->outbuf, '\n');
				}
			} else {
				if (st->cfg->mode == MODE_JIRA)
					strbuf_puts(&st->outbuf, "{code}\n");
				else
					strbuf_puts(&st->outbuf, "```\n");
			}
			st->consecutive_newlines = 1;
			st->at_line_start = 1;
		}
	} else if (tag_is(name, "code")) {
		st->code_depth++;
		if (st->pre_depth > 0 && (st->cfg->mode == MODE_MARKDOWN || st->cfg->mode == MODE_JIRA)) {
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
			} else if (st->outbuf.len >= 7 && strcmp(st->outbuf.data + st->outbuf.len - 7, "{code}\n") == 0) {
				if (extract_attribute(tag_str, "data-lang", attr_val, sizeof(attr_val)) ||
				    extract_attribute(tag_str, "class", attr_val, sizeof(attr_val))) {
					char *lang = strstr(attr_val, "language-");
					if (lang)
						lang += 9;
					else
						lang = attr_val;
					st->outbuf.len -= 7;
					st->outbuf.data[st->outbuf.len] = '\0';
					strbuf_puts(&st->outbuf, "{code:");
					strbuf_puts(&st->outbuf, lang);
					strbuf_puts(&st->outbuf, "}\n");
				}
			}
		} else if (!st->pre_depth) {
			if (st->cfg->mode == MODE_MARKDOWN || st->cfg->mode == MODE_SLACK)
				emit_text(st, "`");
			else if (st->cfg->mode == MODE_JIRA)
				emit_text(st, "{{");
		}
	} else if (tag_is(name, "a")) {
		if (st->need_space && !st->at_line_start && !st->current_table) {
			strbuf_putc(&st->outbuf, ' ');
			st->need_space = 0;
		}
		if (extract_attribute(tag_str, "href", st->current_href, sizeof(st->current_href))) {
			if (!st->cfg->keep_tracking)
				url_strip_tracking_inplace(st->current_href);
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
		else if ((st->cfg->mode == MODE_SLACK || st->cfg->mode == MODE_JIRA) && !st->current_table)
			emit_text(st, "*");
		else if (st->cfg->mode == MODE_TERMINAL)
			emit_text(st, "\033[1m");
	} else if (tag_is(name, "i") || tag_is(name, "em")) {
		st->italic_depth++;
		if (st->cfg->mode == MODE_MARKDOWN && !st->current_table)
			emit_text(st, "*");
		else if ((st->cfg->mode == MODE_SLACK || st->cfg->mode == MODE_JIRA) && !st->current_table)
			emit_text(st, "_");
		else if (st->cfg->mode == MODE_TERMINAL)
			emit_text(st, "\033[3m");
	} else if (tag_is(name, "s") || tag_is(name, "del") || tag_is(name, "strike")) {
		st->strike_depth++;
		if (st->cfg->mode == MODE_MARKDOWN && !st->current_table)
			emit_text(st, "~~");
		else if (st->cfg->mode == MODE_SLACK && !st->current_table)
			emit_text(st, "~");
		else if (st->cfg->mode == MODE_JIRA && !st->current_table)
			emit_text(st, "-");
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
			if (st->cfg->mode == MODE_MARKDOWN || st->cfg->mode == MODE_SLACK) {
				emit_newlines(st, 1);
				strbuf_puts(&st->outbuf, "```\n");
				st->consecutive_newlines = 1;
				st->at_line_start = 1;
			} else if (st->cfg->mode == MODE_JIRA) {
				emit_newlines(st, 1);
				strbuf_puts(&st->outbuf, "{code}\n");
				st->consecutive_newlines = 1;
				st->at_line_start = 1;
			}
			emit_newlines(st, 2);
		}
	} else if (tag_is(name, "code")) {
		if (st->code_depth > 0) {
			st->code_depth--;
			if (!st->pre_depth) {
				if (st->cfg->mode == MODE_MARKDOWN || st->cfg->mode == MODE_SLACK)
					emit_text(st, "`");
				else if (st->cfg->mode == MODE_JIRA)
					emit_text(st, "}}");
			}
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
			else if ((st->cfg->mode == MODE_SLACK || st->cfg->mode == MODE_JIRA) && !st->current_table)
				emit_text(st, "*");
			else if (st->cfg->mode == MODE_TERMINAL)
				emit_text(st, "\033[0m");
		}
	} else if (tag_is(name, "i") || tag_is(name, "em")) {
		if (st->italic_depth > 0) {
			st->italic_depth--;
			if (st->cfg->mode == MODE_MARKDOWN && !st->current_table)
				emit_text(st, "*");
			else if ((st->cfg->mode == MODE_SLACK || st->cfg->mode == MODE_JIRA) && !st->current_table)
				emit_text(st, "_");
			else if (st->cfg->mode == MODE_TERMINAL)
				emit_text(st, "\033[0m");
		}
	} else if (tag_is(name, "s") || tag_is(name, "del") || tag_is(name, "strike")) {
		if (st->strike_depth > 0) {
			st->strike_depth--;
			if (st->cfg->mode == MODE_MARKDOWN && !st->current_table)
				emit_text(st, "~~");
			else if (st->cfg->mode == MODE_SLACK && !st->current_table)
				emit_text(st, "~");
			else if (st->cfg->mode == MODE_JIRA && !st->current_table)
				emit_text(st, "-");
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

/* Main parsing loop for HTML input buffer into strbuf */
int
unipaste_process_to_strbuf(const char *input, size_t len, struct strbuf *out, const struct config *cfg)
{
	struct parser_state st;
	const char *p, *end;
	char entity_buf[16];
	size_t consumed;
	int in_tag = 0;
	int in_script_or_style = 0;
	char char_buf[8];
	int i;
	char *sanitized = NULL;
	char *alloc_input = NULL;
	size_t j;

	if (!input || !out || !cfg)
		return 1;

	/* Plugin hook: sanitize input before formatting if compiled in */
	sanitized = sanitize_html(input, len);
	if (sanitized) {
		alloc_input = sanitized;
		input = alloc_input;
		len = strlen(alloc_input);
	} else {
		alloc_input = malloc(len + 1);
		if (!alloc_input)
			return 1;
		memcpy(alloc_input, input, len);
		alloc_input[len] = '\0';
		input = alloc_input;
	}

	memset(&st, 0, sizeof(st));
	st.cfg = cfg;
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
		for (j = 0; j < st.outbuf.len; j++) {
			if (st.outbuf.data[j] == '\n' && (j == 0 || st.outbuf.data[j - 1] != '\r'))
				strbuf_putc(out, '\r');
			strbuf_putc(out, st.outbuf.data[j]);
		}
	} else {
		strbuf_append(out, st.outbuf.data, st.outbuf.len);
	}

	/* Clean up */
	strbuf_free(&st.outbuf);
	strbuf_free(&st.linebuf);
	strbuf_free(&st.tagbuf);
	strbuf_free(&st.textbuf);
	strbuf_free(&st.link_text);
	strbuf_free(&st.heading_text);
	free(alloc_input);

	return 0;
}

/* Process input string and write to stream */
int
unipaste_process_string(const char *input, size_t len, FILE *out, const struct config *cfg)
{
	struct strbuf sb;
	int ret;

	if (!input || !out || !cfg)
		return 1;

	strbuf_init(&sb, len ? len * 2 : 4096);
	ret = unipaste_process_to_strbuf(input, len, &sb, cfg);
	if (ret == 0 && sb.len > 0) {
		fwrite(sb.data, 1, sb.len, out);
	}
	strbuf_free(&sb);
	return ret;
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
