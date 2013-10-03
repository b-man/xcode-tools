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
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

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
 * @func call_command - Execute new process to replace this one.
 * @arg cmd -- program's absolute path
 * @arg argv -- arguments to be passed to new process
 * @return: -1 on error, otherwise no return
 */
static int call_command(const char *cmd, char *argv[])
{
	return execv(cmd, argv);
}

/**
 * @func find_command - Search for a program.
 * @arg name -- name of program
 * @arg argv -- arguments to be passed if program found
 * @return: NULL on failed search or execution
 */
static char *find_command(const char *name, char *argv[])
{
	char *cmd = NULL;		/* command's absolute path */
	char *env_path = NULL;		/* contents of PATH env variable */
	char *absl_path = NULL;		/* path entry in PATH env variable */
	char *this_path = NULL;		/* the path that might point to the program */
	char delimiter[2] = ":";	/* delimiter for path enteries in PATH env variable */
	int has_searched = 0;		/* flag set to 1 if search is performed */

	/* Read our PATH environment variable. */
	if ((env_path = getenv("PATH")) != NULL)
		absl_path = strtok(env_path, delimiter);
	else {
		fprintf(stderr, "xcrun: error: failed to read PATH variable.\n");
		return NULL;
	}

	/* Search each path entry in PATH until we find our program. */
	while (absl_path != NULL) {
		has_searched = 1;
		this_path = (char *)malloc((PATH_MAX - 1));

		/* Construct our program's absolute path. */
		strncpy(this_path, absl_path, strlen(absl_path));
		cmd = strncat(strcat(this_path, "/"), name, strlen(name));

		/* Does it exist? Is it an executable? */
		if (access(cmd, X_OK) == 0) {
			free(this_path);
			call_command(cmd, argv);
			/* NOREACH */
			fprintf(stderr, "xcrun: error: can't exec \'%s\' (errno=%s)\n", cmd, strerror(errno));
			return NULL;
		}

		/* If not, move onto the next entry.. */
		absl_path = strtok(NULL, delimiter);
	}

	/* We have searched PATH, but we haven't found our program. State why. */
	if (has_searched == 1)
		fprintf(stderr, "xcrun: error: can't exec \'%s\' (errno=%s)\n", name, strerror(errno));

	/* Search has failed, return NULL. */
	return NULL;
}

int main(int argc, char *argv[])
{
	int retval = 1;

	/* Print usage if no argument is supplied. */
	if (argc < 2)
		usage(argv[0]);

	/* Search for program. */
	if (find_command(argv[1], ++argv) != NULL)
		retval = 0;
	else {
		fprintf(stderr, "xcrun: error: failed to execute command \'%s\'. aborting.\n", argv[0]);
	}

	return retval;
}
