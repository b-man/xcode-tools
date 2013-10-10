/* xcrun - clone of apple's xcode xcrun utility
 *
 * Copyright (c) 2013, Brian McKenzie <mckenzba@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the organization nor the names of its contributors may
 *     be used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <errno.h>

/* General stuff */
#define TOOL_VERSION "0.0.1"
#define DARWINSDK_CFG ".darwinsdk.dat"

/* Output mode flags */
static int logging_mode = 0;
static int verbose_mode = 0;
static int finding_mode = 0;

/* Ways that this tool may be called */
static const char *multicall_tool_names[4] = {
	"xcrun",
	"xcrun_log",
	"xcrun_verbose",
	"xcrun_nocache"
};

/**
 * @func verbose_printf -- Print output to fp in verbose mode.
 * @arg fp - pointer to file (file, stderr, or stdio)
 * @arg str - string to print
 * @arg ... - additional arguments used
 */
static void verbose_printf(FILE *fp, const char *str, ...)
{
	va_list args;

	if (verbose_mode == 1) {
		va_start(args, str);
		vfprintf(fp, str, args);
		va_end(args);
	}
}

/**
 * @func logging_printf -- Print output to fp in logging mode.
 * @arg fp - pointer to file (file, stderr, or stdio)
 * @arg str - string to print
 * @arg ... - additional arguments used
 */
static void logging_printf(FILE *fp, const char *str, ...)
{
	va_list args;

	if (logging_mode == 1) {
		va_start(args, str);
		vfprintf(fp, str, args);
		va_end(args);
	}
}

/**
 * @func usage -- Print helpful information about this program.
 */
static void usage(void)
{
	fprintf(stderr, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		"Usage: xcrun [options] <tool name> ... arguments ...\n",
		"\n",
		"Find and execute the named command line tool from the active developer directory.\n",
		"\n",
		"The active developer directory can be set using `xcode-select`, or via the\n",
		"DEVELOPER_DIR environment variable. See the xcrun and xcode-select manual\n",
		"pages for more information.\n",
		"\n",
		"Options:\n",
		"  -h, --help                  show this help message and exit\n",
		"  --version                   show the xcrun version\n",
		"  -v, --verbose               show verbose logging output\n",
		"  --sdk <sdk name>            find the tool for the given SDK name	(!!!NOT SUPPORTED YET!!!)\n",
		"  --toolchain <name>          find the tool for the given toolchain	(!!!NOT SUPPORTED YET!!!)\n",
		"  -l, --log                   show commands to be executed (with --run)\n",
		"  -f, --find                  only find and print the tool path\n",
		"  -r, --run                   find and execute the tool (the default behavior)\n",
		"  -n, --no-cache              do not use the lookup cache		(!!!NOT SUPPORTED YET!!!)\n",
		"  -k, --kill-cache            invalidate all existing cache entries	(!!!NOT SUPPORTED YET!!!)\n",
		"  --show-sdk-path             show selected SDK install path		(!!!NOT SUPPORTED YET!!!)\n",
		"  --show-sdk-version          show selected SDK version			(!!!NOT SUPPORTED YET!!!)\n",
		"  --show-sdk-platform-path    show selected SDK platform path		(!!!NOT SUPPORTED YET!!!)\n",
		"  --show-sdk-platform-version show selected SDK platform version	(!!!NOT SUPPORTED YET!!!)\n\n"
		);

	exit(0);
}

/**
 * @func version -- print out version info for this tool
 */
static void version(void)
{
	fprintf(stdout, "xcrun version %s\n", TOOL_VERSION);
	exit(0);
}

/**
 * @func get_sdk_path -- retrieve current developer path
 * @return: string of current path on success, NULL string on failure
 */
static char *get_sdk_path(void)
{
	FILE *fp = NULL;
	char devpath[PATH_MAX - 1];
	char *pathtocfg = NULL;
	char *darwincfg_path = NULL;
	char *value = NULL;

	verbose_printf(stdout, "xcrun: info: attempting to retrieve developer path from DEVELOPER_DIR...\n");

	if ((value = getenv("DEVELOPER_DIR")) != NULL) {
		verbose_printf(stdout, "xcrun: info: using developer path \'%s\' from DEVELOPER_DIR.\n", value);
		return value;
	}

	memset(devpath, 0, sizeof(devpath));

	verbose_printf(stdout, "xcrun: info: attempting to retrieve developer path from configuration file...\n");
	if ((pathtocfg = getenv("HOME")) == NULL) {
		fprintf(stderr, "xcrun: error: failed to read HOME variable.\n");
		return NULL;
	}

	darwincfg_path = (char *)malloc((strlen(pathtocfg) + sizeof(DARWINSDK_CFG)));

	strcat(pathtocfg, "/");
	strcat(darwincfg_path, strcat(pathtocfg, DARWINSDK_CFG));

	if ((fp = fopen(darwincfg_path, "r")) != NULL) {
		fseek(fp, 0, SEEK_SET);
		fread(devpath, (PATH_MAX), 1, fp);
		value = devpath;
		fclose(fp);
	} else {
		fprintf(stderr, "xcrun: error: unable to read configuration file. (errno=%s)\n", strerror(errno));
		return NULL;
	}

	verbose_printf(stdout, "xcrun: info: using developer path \'%s\' from configuration file.\n", value);

	free(darwincfg_path);

	return value;
}

/**
 * @func call_command -- Execute new process to replace this one.
 * @arg cmd - program's absolute path
 * @arg argc - number of arguments to be passed to new process
 * @arg argv - arguments to be passed to new process
 * @return: -1 on error, otherwise no return
 */
static int call_command(const char *cmd, int argc, char *argv[])
{
	int i;

	if (logging_mode == 1) {
		logging_printf(stdout, "xcrun: info: invoking command:\n\t\"%s", cmd);
		for (i = 1; i < argc; i++)
			logging_printf(stdout, " %s", argv[i]);
		logging_printf(stdout, "\"\n");
	}

	return execv(cmd, argv);
}

/**
 * @func find_command - Search for a program.
 * @arg name -- name of program
 * @arg argv -- arguments to be passed if program found
 * @return: NULL on failed search or execution
 */
static char *find_command(const char *name, int argc, char *argv[])
{
	char *cmd = NULL;		/* command's absolute path */
	char *env_path = NULL;		/* contents of PATH env variable */
	char *absl_path = NULL;		/* path entry in PATH env variable */
	char *this_path = NULL;		/* the path that might point to the program */
	char delimiter[2] = ":";	/* delimiter for path enteries in PATH env variable */

	/* Read our PATH environment variable. */
	if ((env_path = getenv("PATH")) != NULL)
		absl_path = strtok(env_path, delimiter);
	else {
		fprintf(stderr, "xcrun: error: failed to read PATH variable.\n");
		return NULL;
	}

	/* Search each path entry in PATH until we find our program. */
	while (absl_path != NULL) {
		this_path = (char *)malloc((PATH_MAX - 1));

		verbose_printf(stdout, "xcrun: info: checking directory \'%s\' for command \'%s\'...\n", absl_path, name);

		/* Construct our program's absolute path. */
		strncpy(this_path, absl_path, strlen(absl_path));
		cmd = strncat(strcat(this_path, "/"), name, strlen(name));

		/* Does it exist? Is it an executable? */
		if (access(cmd, X_OK) == 0) {
			verbose_printf(stdout, "xcrun: info: found command's absolute path: \'%s\'\n", cmd);
			if (finding_mode != 1) {
				call_command(cmd, argc, argv);
				/* NOREACH */
				fprintf(stderr, "xcrun: error: can't exec \'%s\' (errno=%s)\n", cmd, strerror(errno));
				return NULL;
			} else {
				fprintf(stdout, "%s\n", cmd);
				exit(0);
			}
		}

		/* If not, move onto the next entry.. */
		absl_path = strtok(NULL, delimiter);
	}

	/* We have searched PATH, but we haven't found our program yet. Try looking at the SDK folder */
	this_path = (char *)malloc((PATH_MAX - 1));
	if ((this_path = get_sdk_path()) != NULL) {
		verbose_printf(stdout, "xcrun: info: checking directory \'%s/usr/bin\' for command \'%s\'...\n", this_path, name);
		cmd = strncat(strcat(this_path, "/usr/bin/"), name, strlen(name));
		/* Does it exist? Is it an executable? */
		if (access(cmd, X_OK) == 0) {
			verbose_printf(stdout, "xcrun: info: found command's absolute path: \'%s\'\n", cmd);
			if (finding_mode != 1) {
				call_command(cmd, argc, argv);
				/* NOREACH */
				fprintf(stderr, "xcrun: error: can't exec \'%s\' (errno=%s)\n", cmd, strerror(errno));
				return NULL;
			} else {
				fprintf(stdout, "%s\n", cmd);
				exit(0);
			}
		}
	}

	/* We have searched everywhere, but we haven't found our program. State why. */
	fprintf(stderr, "xcrun: error: can't stat \'%s\' (errno=%s)\n", name, strerror(errno));

	/* Search has failed, return NULL. */
	return NULL;
}

/**
 * @func xcrun_main -- xcrun's main routine
 * @arg argc - number of arguments passed by user
 * @arg argv - array of arguments passed by user
 * @return: 0 (or none) on success, 1 on failure
 */
static int xcrun_main(int argc, char *argv[])
{
	int ch;
	int retval = 1;
	int optindex = 0;
	int argc_offset = 0;
	char *tool_called = NULL;
	char *sdk = NULL;
	char *toolchain = NULL;

	static int help_f, verbose_f, log_f, find_f, run_f, nocache_f, killcache_f, version_f, sdk_f, toolchain_f, ssdkp_f, ssdkv_f, ssdkpp_f, ssdkpv_f;
	help_f = verbose_f = log_f = find_f = run_f = nocache_f = killcache_f = version_f = sdk_f = toolchain_f = ssdkp_f = ssdkv_f = ssdkpp_f = ssdkpv_f = 0;

	/* Supported options */
	static struct option options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, &version_f, 1 },
		{ "verbose", no_argument, 0, 'v' },
		{ "sdk", required_argument, &sdk_f, 1 },
		{ "toolchain", required_argument, &toolchain_f, 1 },
		{ "log", no_argument, 0, 'l' },
		{ "find", required_argument, 0, 'f' },
		{ "run", required_argument, 0, 'r' },
		{ "no-cache", no_argument, 0, 'n' },
		{ "kill-cache", no_argument, 0, 'k' },
		{ "show-sdk-path", no_argument, &ssdkp_f, 1 },
		{ "show-sdk-version", no_argument, &ssdkv_f, 1 },
		{ "show-sdk-platform-path", no_argument, &ssdkpp_f, 1 },
		{ "show-sdk-platform-version", no_argument, &ssdkpv_f, 1 },
		{ NULL, 0, 0, 0 }
	};

	/* Print help if nothing is specified */
	if (argc < 2)
		usage();

	/* Only parse arguments if they are given */
	if (*(*(argv + 1)) == '-') {
		if (strcmp(argv[1], "-") == 0 || strcmp(argv[1], "--") == 0)
			usage();
		while ((ch = getopt_long_only(argc, argv, "+hvlr:f:nk", options, &optindex)) != (-1)) {
			switch (ch) {
				case 'h':
					help_f = 1;
					break;
				case 'v':
					verbose_f = 1;
					break;
				case 'l':
					log_f = 1;
					break;
				case 'r':
					run_f = 1;
					tool_called = basename(optarg);
					++argc_offset;
					break;
				case 'f':
					find_f = 1;
					tool_called = basename(optarg);
					++argc_offset;
					break;
				case 'n':
					nocache_f = 1;
					break;
				case 'k':
					killcache_f = 1;
					break;
				case 0: /* long-only options */
					switch (optindex) {
						case 1: /* --version */
							break;
						case 3: /* --sdk */
							if (*optarg != '-') {
								++argc_offset;
								sdk = optarg;
							} else {
								fprintf(stderr, "xcrun: error: sdk flag requires an argument.\n");
								exit(1);
							}
							break;
						case 4: /* --toolchain */
							if (*optarg != '-') {
								++argc_offset;
								toolchain = optarg;
							} else {
								fprintf(stderr, "xcrun: error: toolchain flag requires an argument.\n");
								exit(1);
							}
							break;
						case 10: /* --show-sdk-path */
							break;
						case 11: /* --show-sdk-version */
							break;
						case 12: /* --show-sdk-platform-path */
							break;
						case 13: /* --show-sdk-platform-version */
							break;
					}
					break;
				case '?':
				default:
					help_f = 1;
					break;
			}

			++argc_offset;

			/* We don't want to parse any more arguments after these are set. */
			if (ch == 'f' || ch == 'r')
				break;
		}
	} else { /* We are just executing a program. */
		tool_called = basename(argv[1]);
		++argc_offset;
	}

	/* The last non-option argument may be the command called. */
	if (optind < argc && ((run_f == 0 || find_f == 0) && tool_called == NULL)) {
		tool_called = basename(argv[optind++]);
		++argc_offset;
	}

	/* Don't continue if we are missing arguments. */
	if ((verbose_f == 1 || log_f == 1) && tool_called == NULL) {
		fprintf(stderr, "xcrun: error: specified arguments require -r or -f arguments.\n");
		exit(1);
	}

	/* Print help? */
	if (help_f == 1 || argc < 2)
		usage();

	/* Print version? */
	if (version_f == 1)
		version();

	/* Show SDK path? */
	if (ssdkp_f == 1) {
		fprintf(stderr, "xcrun: error: option not supported yet.\n");
		exit(1);
	}

	/* Show SDK version? */
	if (ssdkv_f == 1) {
		fprintf(stderr, "xcrun: error: option not supported yet.\n");
		exit(1);
	}

	/* Show SDK platform path? */
	if (ssdkpp_f == 1) {
		fprintf(stderr, "xcrun: error: option not supported yet.\n");
		exit(1);
	}

	/* Show SDK platform version? */
	if (ssdkpv_f == 1) {
		fprintf(stderr, "xcrun: error: option not supported yet.\n");
		exit(1);
	}

	/* Clear the lookup cache? */
	if (killcache_f == 1) {
		fprintf(stderr, "xcrun: error: option not supported yet.\n");
		exit(1);
	}

	/* Don't use the lookup cache? */
	if (nocache_f == 1) {
		fprintf(stderr, "xcrun: error: option not supported yet.\n");
		exit(1);
	}

	/* Turn on verbose mode? */
	if (verbose_f == 1)
		verbose_mode = 1;

	/* Turn on logging mode? */
	if (log_f == 1)
		logging_mode = 1;

	/* Search for program? */
	if (find_f == 1) {
		finding_mode = 1;
		if (find_command(tool_called, 0, NULL) != NULL)
			retval = 0;
		else
			fprintf(stderr, "xcrun: error: unable to locate command \'%s\'.\n", tool_called);
			exit(1);
	}

	/* Search and execute program. (default behavior) */
	if (find_command(tool_called, ((argc - argc_offset) - (argc - argc_offset)),  (argv += ((argc - argc_offset) - (argc - argc_offset) + (argc_offset)))) != NULL) {
		/* NOREACH */
		retval = 0;
	} else {
		fprintf(stderr, "xcrun: error: failed to execute command \'%s\'. aborting.\n", tool_called);
	}

	return retval;
}

/**
 * @func get_multicall_state -- Return a number that is associated to a given multicall state.
 * @arg cmd - command that binary is being called
 * @arg state - char array containing a set of possible "multicall states"
 * @arg state_size - number of elements in state array
 * @return: a number from 1 to state_size (first enrty to last entry found in state array), or -1 if one isn't found
 */
static int get_multicall_state(const char *cmd, const char *state[], int state_size)
{
	int i;

	for (i = 0; i < (state_size - 1); i++) {
		if (strcmp(cmd, state[i]) == 0)
			return (i + 1);
	}

	return -1;
}

int main(int argc, char *argv[])
{
	int retval = 1;
	int call_state;
	char *this_tool = NULL;

	/* Strip out any path name that may have been passed into argv[0]. */
	this_tool = basename(argv[0]);

	/* Check if we are being treated as a multi-call binary. */
	call_state = get_multicall_state(this_tool, multicall_tool_names, 4);

	/* Execute based on the state that we were callen in. */
	switch (call_state) {
		case 1: /* xcrun */
			retval = xcrun_main(argc, argv);
			break;
		case 2: /* xcrun_log */
			logging_mode = 1;
			retval = xcrun_main(argc, argv);
			break;
		case 3: /* xcrun_verbose */
			verbose_mode = 1;
			retval = xcrun_main(argc, argv);
			break;
		case 4: /* xcrun_nocache */
			retval = xcrun_main(argc, argv);
			break;
		case -1:
		default: /* called as tool name */
			if (find_command(this_tool, argc, argv) != NULL)
				retval = 0;
			else {
				fprintf(stderr, "xcrun: error: failed to execute command \'%s\'. aborting.\n", this_tool);
			}
			break;
	}

	return retval;
}
