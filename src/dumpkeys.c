/*
 * dumpkeys.c
 *
 * derived from version 0.81 - aeb@cwi.nl
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include "ksyms.h"
#include "getfd.h"
#include "modifiers.h"
#include "nls.h"
#include "version.h"

static int fd;
static int verbose;

static void attr_noreturn
usage(void) {
	fprintf(stderr, _("dumpkeys version %s"), PACKAGE_VERSION);
	fprintf(stderr, _("\
\n\
usage: dumpkeys [options...]\n\
\n\
valid options are:\n\
\n\
	-h --help	    display this help text\n\
	-i --short-info	    display information about keyboard driver\n\
	-l --long-info	    display above and symbols known to loadkeys\n\
	-n --numeric	    display keytable in hexadecimal notation\n\
	-f --full-table	    don't use short-hand notations, one row per keycode\n\
	-1 --separate-lines one line per (modifier,keycode) pair\n\
	   --funcs-only	    display only the function key strings\n\
	   --keys-only	    display only key bindings\n\
	   --compose-only   display only compose key combinations\n\
	-c --charset="));
	list_charsets(stderr);
	fprintf(stderr, _("\
			    interpret character action codes to be from the\n\
			    specified character set\n\
"));
	exit(1);
}

int
main (int argc, char *argv[]) {
	const char *short_opts = "hilvsnf1tkdS:c:V";
	const struct option long_opts[] = {
		{ "help",	no_argument,		NULL, 'h' },
		{ "short-info",	no_argument,		NULL, 'i' },
		{ "long-info",	no_argument,		NULL, 'l' },
		{ "numeric",	no_argument,		NULL, 'n' },
		{ "full-table",	no_argument,		NULL, 'f' },
		{ "separate-lines",no_argument,		NULL, '1' },
		{ "shape",	required_argument,	NULL, 'S' },
		{ "funcs-only",	no_argument,		NULL, 't' },
		{ "keys-only",	no_argument,		NULL, 'k' },
		{ "compose-only",no_argument,		NULL, 'd' },
		{ "charset",	required_argument,	NULL, 'c' },
		{ "verbose",	no_argument,		NULL, 'v' },
		{ "version",	no_argument,		NULL, 'V' },
		{ NULL,	0, NULL, 0 }
	};
	int c, rc;
	int kbd_mode;

	char long_info = 0;
	char short_info = 0;
	char numeric = 0;
	char table_shape = 0;
	char funcs_only = 0;
	char keys_only = 0;
	char diac_only = 0;

	struct keymap kmap;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	while ((c = getopt_long(argc, argv,
		short_opts, long_opts, NULL)) != -1) {
		switch (c) {
			case 'i':
				short_info = 1;
				break;
			case 's':
			case 'l':
				long_info = 1;
				break;
			case 'n':
				numeric = 1;
				break;
			case 'f':
				table_shape = FULL_TABLE;
				break;
			case '1':
				table_shape = SEPARATE_LINES;
				break;
			case 'S':
				table_shape = atoi(optarg);
				break;
			case 't':
				funcs_only = 1;
				break;
			case 'k':
				keys_only = 1;
				break;
			case 'd':
				diac_only = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'c':
				if ((set_charset(optarg)) != 0) {
					fprintf(stderr, _("unknown charset %s - ignoring charset request\n"),
						optarg);
					usage();
				}
				printf("charset \"%s\"\n", optarg);
				break;
			case 'V':
				print_version_and_exit();
			case 'h':
			case '?':
				usage();
		}
	}

	if (optind < argc)
		usage();

	keymap_init(&kmap);

	fd = getfd(NULL);

	/* check whether the keyboard is in Unicode mode */
	if (ioctl(fd, KDGKBMODE, &kbd_mode)) {
		fprintf(stderr, _("%s: error reading keyboard mode: %m\n"),
			progname);
		exit(EXIT_FAILURE);
	}

	if (kbd_mode == K_UNICODE) {
		kmap.prefer_unicode = 1;
	}

	if ((rc = get_keymap(&kmap, fd)) < 0)
		goto fail;

	if (short_info || long_info) {
		dump_summary(&kmap, stdout, fd);

		if (long_info) {
			printf(_("Symbols recognized by %s:\n(numeric value, symbol)\n\n"),
				progname);
			dump_symbols(stdout);
		}
		exit(EXIT_SUCCESS);
	}

#ifdef KDGKBDIACR
	if (!diac_only) {
#endif
	if (!funcs_only) {
		dump_keymap(&kmap, stdout, table_shape, numeric);
	}
#ifdef KDGKBDIACR
	}

	if (!funcs_only && !keys_only)
		dump_diacs(&kmap, stdout);
#endif

 fail:	keymap_free(&kmap);
	close(fd);

	if (rc < 0)
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}
