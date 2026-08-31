/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "arg.h"
#include "unipaste.h"

char *argv0;

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-urvKh] [-m mode] [-t table] [-l link] [file ...]\n", argv0);
	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr, "  -m mode    Output mode: plain (default), markdown, slack, jira, terminal\n");
	fprintf(stderr, "  -t table   Table format: grid (default), markdown, tsv, simple\n");
	fprintf(stderr, "  -l link    Link format: bracket (default), inline, text, footnote\n");
	fprintf(stderr, "  -u         Use Unicode box-drawing characters for tables\n");
	fprintf(stderr, "  -r         Emit Windows CRLF (\\r\\n) line endings\n");
	fprintf(stderr, "  -K         Keep URL tracking parameters (disabled by default)\n");
	fprintf(stderr, "  -v         Display version information\n");
	fprintf(stderr, "  -h         Display this help message\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct config cfg;
	char *arg;
	FILE *fp;
	int i;
	int ret = 0;

	/* Default configuration */
	memset(&cfg, 0, sizeof(cfg));
	cfg.mode = MODE_PLAIN;
	cfg.table_style = TABLE_STYLE_AUTO;
	cfg.link_style = LINK_STYLE_AUTO;
	cfg.wrap_width = 80;
	cfg.crlf = 0;
	cfg.unicode_tables = 0;

	ARGBEGIN {
	case 'm':
		arg = EARGF(usage());
		if (strcmp(arg, "plain") == 0)
			cfg.mode = MODE_PLAIN;
		else if (strcmp(arg, "markdown") == 0 || strcmp(arg, "md") == 0)
			cfg.mode = MODE_MARKDOWN;
		else if (strcmp(arg, "slack") == 0 || strcmp(arg, "mrkdwn") == 0)
			cfg.mode = MODE_SLACK;
		else if (strcmp(arg, "jira") == 0 || strcmp(arg, "confluence") == 0)
			cfg.mode = MODE_JIRA;
		else if (strcmp(arg, "terminal") == 0 || strcmp(arg, "ansi") == 0)
			cfg.mode = MODE_TERMINAL;
		else {
			fprintf(stderr, "%s: invalid mode '%s'\n", argv0, arg);
			usage();
		}
		break;
	case 't':
		arg = EARGF(usage());
		if (strcmp(arg, "grid") == 0 || strcmp(arg, "ascii") == 0)
			cfg.table_style = TABLE_STYLE_GRID;
		else if (strcmp(arg, "markdown") == 0 || strcmp(arg, "md") == 0)
			cfg.table_style = TABLE_STYLE_MARKDOWN;
		else if (strcmp(arg, "tsv") == 0)
			cfg.table_style = TABLE_STYLE_TSV;
		else if (strcmp(arg, "simple") == 0)
			cfg.table_style = TABLE_STYLE_SIMPLE;
		else {
			fprintf(stderr, "%s: invalid table style '%s'\n", argv0, arg);
			usage();
		}
		break;
	case 'l':
		arg = EARGF(usage());
		if (strcmp(arg, "bracket") == 0)
			cfg.link_style = LINK_STYLE_BRACKET;
		else if (strcmp(arg, "inline") == 0)
			cfg.link_style = LINK_STYLE_INLINE;
		else if (strcmp(arg, "text") == 0 || strcmp(arg, "textonly") == 0)
			cfg.link_style = LINK_STYLE_TEXTONLY;
		else if (strcmp(arg, "footnote") == 0)
			cfg.link_style = LINK_STYLE_FOOTNOTE;
		else {
			fprintf(stderr, "%s: invalid link style '%s'\n", argv0, arg);
			usage();
		}
		break;
	case 'u':
		cfg.unicode_tables = 1;
		break;
	case 'r':
		cfg.crlf = 1;
		break;
	case 'K':
		cfg.keep_tracking = 1;
		break;
	case 'v':
		puts("unipaste-" VERSION);
		return 0;
	case 'h':
	default:
		usage();
	} ARGEND

	if (argc == 0) {
		/* Read from stdin */
		ret = unipaste_process_stream(stdin, stdout, &cfg);
	} else {
		/* Process files */
		for (i = 0; i < argc; i++) {
			if (strcmp(argv[i], "-") == 0) {
				fp = stdin;
			} else {
				fp = fopen(argv[i], "r");
				if (!fp) {
					fprintf(stderr, "%s: cannot open '%s': ", argv0, argv[i]);
					perror(NULL);
					ret = 1;
					continue;
				}
			}

			if (unipaste_process_stream(fp, stdout, &cfg) != 0)
				ret = 1;

			if (fp != stdin)
				fclose(fp);
		}
	}

	return ret;
}
