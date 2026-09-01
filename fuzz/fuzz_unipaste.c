/* See LICENSE file for copyright and license details. */
/*
 * fuzz_unipaste.c - LLVM LibFuzzer / OSS-Fuzz harness for unipaste
 *
 * Compiles with clang -fsanitize=fuzzer,address,undefined
 * to fuzz parser, table, entity, and strbuf engines against arbitrary inputs.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "unipaste.h"

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	struct config cfg;
	struct strbuf out;
	uint8_t flags;

	if (size == 0)
		return 0;

	memset(&cfg, 0, sizeof(cfg));

	/* First byte selects config permutation */
	flags = data[0];
	cfg.mode = (enum output_mode)(flags % 5);
	cfg.table_style = (enum table_style)((flags >> 3) % 5);
	cfg.link_style = (enum link_style)((flags >> 5) % 5);
	cfg.crlf = (flags >> 6) & 1;
	cfg.unicode_tables = (flags >> 7) & 1;
	cfg.keep_tracking = (flags >> 2) & 1;

	strbuf_init(&out, size * 2 + 128);

	/* Fuzz in-memory processor */
	unipaste_process_to_strbuf((const char *)(data + 1), size - 1, &out, &cfg);

	strbuf_free(&out);
	return 0;
}
