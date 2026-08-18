/* See LICENSE file for copyright and license details. */
/*
 * Default no-op plugin: sanitize_html() returns NULL, meaning the
 * input passes through to the formatting engine unchanged.
 *
 * This file is compiled when no SANITIZE= flag is specified,
 * preserving the zero-dependency suckless default build.
 */
#include "plugin.h"

char *
sanitize_html(const char *input, size_t len)
{
	(void)input;
	(void)len;
	return NULL; /* passthrough: no sanitization */
}
