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
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <errno.h>

#define TOOL_VERSION "0.0.1"
#define DARWINSDK_CFG ".darwinsdk.dat"

/* Output mode flags */
static int logging_mode = 0;
static int verbose_mode = 0;

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
 * @prog -- name of this program
 */
static void usage(char *prog)
{
	fprintf(stderr, "Usage: %s <program>\n", prog);
	exit(1);
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
		fprintf(stderr, "xcode-select: error: failed to read HOME variable.\n");
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
		fprintf(stderr, "xcode-select: error: unable to read configuration file. (errno=%s)\n", strerror(errno));
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
			call_command(cmd, argc, argv);
			/* NOREACH */
			fprintf(stderr, "xcrun: error: can't exec \'%s\' (errno=%s)\n", cmd, strerror(errno));
			return NULL;
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
			call_command(cmd, argc, argv);
			/* NOREACH */
			fprintf(stderr, "xcrun: error: can't exec \'%s\' (errno=%s)\n", cmd, strerror(errno));
			return NULL;
		}
	}

	/* We have searched everywhere, but we haven't found our program. State why. */
	fprintf(stderr, "xcrun: error: can't exec \'%s\' (errno=%s)\n", name, strerror(errno));

	/* Search has failed, return NULL. */
	return NULL;
}

static int xcrun_main(int argc, char *argv[])
{
	int retval = 1;
	char *tool_called = NULL;

	/* Print usage if no argument is supplied. */
	if (argc < 2)
		usage(argv[0]);

	/* Strip out any path name that may have been passed into argv[1]. */
	tool_called = basename(argv[1]);

	/* Search for program. */
	if (find_command(tool_called, --argc,  ++argv) != NULL)
		retval = 0;
	else {
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
