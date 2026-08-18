/* See LICENSE file for copyright and license details. */
#ifndef PLUGIN_H
#define PLUGIN_H

#include <stddef.h>

/*
 * Plugin hook: sanitize raw HTML input before the formatting engine
 * processes it.
 *
 * Returns a newly allocated sanitized string (caller must free),
 * or NULL if sanitization is unavailable or disabled (passthrough).
 *
 * The plugin architecture is compile-time only. No dynamic linking,
 * no dlopen(), no runtime overhead. The Makefile selects which
 * plugin_*.c source to compile:
 *
 *   make                      -> plugin_none.c   (no-op, zero deps)
 *   make SANITIZE=builtin     -> plugin_builtin.c (allowlist sanitizer)
 *
 * To write a custom plugin, implement this function in a new
 * plugin_yourlib.c and add a Makefile rule. See README for details.
 */
char *sanitize_html(const char *input, size_t len);

#endif /* PLUGIN_H */
