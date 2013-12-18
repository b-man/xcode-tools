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
#include <sys/stat.h>
#include <sys/types.h>

#include "ini.h"

/* General stuff */
#define TOOL_VERSION "0.0.1"
#define DARWINSDK_CFG ".darwinsdk.dat"
#define XCRUN_DEFAULT_CFG "/etc/xcrun.ini"

/* Toolchain configuration struct */
typedef struct {
	const char *name;
	const char *version;
} toolchain_config;

/* SDK configuration struct */
typedef struct {
	const char *name;
	const char *version;
	const char *toolchain;
} sdk_config;

/* xcrun default configuration struct */
typedef struct {
	const char *sdkname;
	const char *sdkvers;
	const char *toolchname;
	const char *toolchvers;
} default_config;

/* Output mode flags */
static int logging_mode = 0;
static int verbose_mode = 0;
static int finding_mode = 0;

/* Behavior mode flags */
static int explicit_sdk_mode = 0;
static int explicit_toolchain_mode = 0;

/* Runtime info */
static char *developer_dir = NULL;
static char *current_sdk = NULL;
static char *current_toolchain = NULL;

/* Ways that this tool may be called */
static const char *multicall_tool_names[4] = {
	"xcrun",
	"xcrun_log",
	"xcrun_verbose",
	"xcrun_nocache"
};

/* Our program's name as called by the user */
static char *progname;

/* helper function to strip file extensions */
static char *stripext(char *dst, const char *src)
{
	int len;
	char *s;

	if ((s = strchr(src, '.')) != NULL)
		len = (s - src);
	else
		len = strlen(src);

	strncpy(dst, src, len);

	return dst;
}

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
	fprintf(stderr,
		"Usage: %s [options] <tool name> ... arguments ...\n"
		"\n"
		"Find and execute the named command line tool from the active developer directory.\n"
		"\n"
		"The active developer directory can be set using `xcode-select`, or via the\n"
		"DEVELOPER_DIR environment variable. See the xcrun and xcode-select manual\n"
		"pages for more information.\n"
		"\n"
		"Options:\n"
		"  -h, --help                  show this help message and exit\n"
		"  --version                   show the xcrun version\n"
		"  -v, --verbose               show verbose logging output\n"
		"  --sdk <sdk name>            find the tool for the given SDK name\n"
		"  --toolchain <name>          find the tool for the given toolchain\n"
		"  -l, --log                   show commands to be executed (with --run)\n"
		"  -f, --find                  only find and print the tool path\n"
		"  -r, --run                   find and execute the tool (the default behavior)\n"
		"  -n, --no-cache              do not use the lookup cache (not implemented yet - does nothing)\n"
		"  -k, --kill-cache            invalidate all existing cache entries (not implemented yet - does nothing)\n"
		"  --show-sdk-path             show selected SDK install path\n"
		"  --show-sdk-version          show selected SDK version\n"
		"  --show-sdk-platform-path    show selected SDK platform path\n"
		"  --show-sdk-platform-version show selected SDK platform version\n\n"
		, progname);

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
 * @func validate_directory_path -- validate if requested directory path exists
 * @arg dir - directory to validate
 * @return: 0 on success, -1 on failure
 */
static int validate_directory_path(const char *dir)
{
	struct stat fstat;
	int retval = -1;

	if (stat(dir, &fstat) != 0)
		fprintf(stderr, "xcrun: error: unable to validate path \'%s\' (errno=%s)\n", dir, strerror(errno));
	else {
		if (S_ISDIR(fstat.st_mode) == 0)
			fprintf(stderr, "xcrun: error: \'%s\' is not a valid path\n", dir);
		else
			retval = 0;
	}

	return retval;
}

/**
 * @func toolchain_cfg_handler -- handler used to process toolchain info.ini contents
 * @arg user - ini user pointer (see ini.h)
 * @arg section - ini section name (see ini.h)
 * @arg name - ini variable name (see ini.h)
 * @arg value - ini variable value (see ini.h)
 * @return: 1 on success, 0 on failure
 */
/*
static int toolchain_cfg_handler(void *user, const char *section, const char *name, const char *value)
{
	toolchain_config *config = (toolchain_config *)user;

	if (MATCH_INI_STON("TOOLCHAIN", "name"))
		config->name = strdup(value);
	else if (MATCH_INI_STON("TOOLCHAIN", "version"))
		config->version = strdup(value);
	else
		return 0;

	return 1;
}
*/
/**
 * @func sdk_cfg_handler -- handler used to process sdk info.ini contents
 * @arg user - ini user pointer (see ini.h)
 * @arg section - ini section name (see ini.h)
 * @arg name - ini variable name (see ini.h)
 * @arg value - ini variable value (see ini.h)
 * @return: 1 on success, 0 on failure
 */
static int sdk_cfg_handler(void *user, const char *section, const char *name, const char *value)
{
	sdk_config *config = (sdk_config *)user;

	if (MATCH_INI_STON("SDK", "name"))
		config->name = strdup(value);
	else if (MATCH_INI_STON("SDK", "version"))
		config->version = strdup(value);
	else if (MATCH_INI_STON("SDK", "toolchain"))
		config->toolchain = strdup(value);
	else
		return 0;

	return 1;
}

/**
 * @func default_cfg_handler -- handler used to process xcrun's xcrun.ini contents
 * @arg user - ini user pointer (see ini.h)
 * @arg section - ini section name (see ini.h)
 * @arg name - ini variable name (see ini.h)
 * @arg value - ini variable value (see ini.h)
 * @return: 1 on success, 0 on failure
 */
static int default_cfg_handler(void *user, const char *section, const char *name, const char *value)
{
	default_config *config = (default_config *)user;

	if (MATCH_INI_STON("SDK", "name"))
		config->sdkname = strdup(value);
	else if (MATCH_INI_STON("SDK", "version"))
		config->sdkvers = strdup(value);
	else if (MATCH_INI_STON("TOOLCHAIN", "name"))
		config->toolchname = strdup(value);
	else if (MATCH_INI_STON("TOOLCHAIN", "version"))
		config->toolchvers = strdup(value);
	else
		return 0;

	return 1;
}

/**
 * @func get_toolchain_info -- fetch config info from a toolchain's info.ini
 * @arg path - path to toolchain's info.ini
 * @return: struct containing toolchain config info
 */
/*
static toolchain_config get_toolchain_info(const char *path)
{
	toolchain_config config;

	if (ini_parse(path, toolchain_cfg_handler, &config) != (-1))
		return config;
	else {
		fprintf(stderr, "xcrun: error: failed to retrieve toolchain info from '\%s\'. (errno=%s)\n", path, strerror(errno));
		exit(1);
	}
}
*/
/**
 * @func get_sdk_info -- fetch config info from a toolchain's info.ini
 * @arg path - path to sdk's info.ini
 * @return: struct containing sdk config info
 */
static sdk_config get_sdk_info(const char *path)
{
	sdk_config config;

	if (ini_parse(path, sdk_cfg_handler, &config) != (-1))
		return config;
	else {
		fprintf(stderr, "xcrun: error: failed to retrieve sdk info from '\%s\'. (errno=%s)\n", path, strerror(errno));
		exit(1);
	}
}
/**
 * @func get_default_info -- fetch default configuration for xcrun
 * @arg path - path to xcrun.ini
 * @return: struct containing default config info
 */
static default_config get_default_info(const char *path)
{
	default_config config;

	if (ini_parse(path, default_cfg_handler, &config) != (-1))
		return config;
	else {
		fprintf(stderr, "xcrun: error: failed to retrieve default info from '\%s\'. (errno=%s)\n", path, strerror(errno));
		exit(1);
	}
}

/**
 * @func get_developer_path -- retrieve current developer path
 * @return: string of current path on success, NULL string on failure
 */
static char *get_developer_path(void)
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

	bzero(devpath, (PATH_MAX - 1));

	verbose_printf(stdout, "xcrun: info: attempting to retrieve developer path from configuration cache...\n");
	if ((pathtocfg = getenv("HOME")) == NULL) {
		fprintf(stderr, "xcrun: error: failed to read HOME variable.\n");
		return NULL;
	}

	darwincfg_path = (char *)malloc((strlen(pathtocfg) + sizeof(DARWINSDK_CFG)));

	sprintf(darwincfg_path, "%s/%s", pathtocfg, DARWINSDK_CFG);

	if ((fp = fopen(darwincfg_path, "r")) != NULL) {
		fseek(fp, 0, SEEK_SET);
		fread(devpath, (PATH_MAX), 1, fp);
		value = devpath;
		fclose(fp);
	} else {
		fprintf(stderr, "xcrun: error: unable to read configuration cache. (errno=%s)\n", strerror(errno));
		return NULL;
	}

	verbose_printf(stdout, "xcrun: info: using developer path \'%s\' from configuration cache.\n", value);

	free(darwincfg_path);

	return value;
}

/**
 * @func get_toolchain_path -- Return the specified toolchain path
 * @arg name - name of the toolchain
 * @return: absolute path of toolchain on success, exit on failure
 */
static char *get_toolchain_path(const char *name)
{
	char *path = NULL;
	char *devpath = NULL;

	devpath = developer_dir;

	path = (char *)malloc(PATH_MAX - 1);

	if (devpath != NULL) {
		sprintf(path, "%s/Toolchains/%s.toolchain", devpath, name);
		if (validate_directory_path(path) != (-1))
			return path;
		else {
			fprintf(stderr, "xcrun: error: \'%s\' is not a valid toolchain path.\n", path);
			free(path);
			exit(1);
		}
	} else {
		fprintf(stderr, "xcrun: error: failed to retrieve developer path, do you have it set?\n");
		free(path);
		exit(1);
	}
}

/**
 * @func get_sdk_path -- Return the specified sdk path
 * @arg name - name of the sdk
 * @return: absolute path of sdk on success, exit on failure
 */
static char *get_sdk_path(const char *name)
{
	char *path = NULL;
	char *devpath = NULL;

	devpath = developer_dir;

	path = (char *)malloc(PATH_MAX - 1);

	if (devpath != NULL) {
		sprintf(path, "%s/SDKs/%s.sdk", devpath, name);
		if (validate_directory_path(path) != (-1))
			return path;
		else {
			fprintf(stderr, "xcrun: error: \'%s\' is not a valid sdk path.\n", path);
			free(path);
			exit(1);
		}
	} else {
		fprintf(stderr, "xcrun: error: failed to retrieve developer path, do you have it set?\n");
		free(path);
		exit(1);
	}
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
		for (i = 0; i < argc; i++)
			logging_printf(stdout, " %s", argv[i]);
		logging_printf(stdout, "\"\n");
	}

	return execv(cmd, argv);
}

/**
 * @func search_command -- Search a set of directories for a given command
 * @arg name - program's name
 * @arg dirs - set of directories to search, seperated by colons
 * @return: the program's absolute path on success, NULL on failure
 */

static char *search_command(const char *name, char *dirs)
{
	char *cmd = NULL;			/* command's absolute path */
	char *absl_path = NULL;		/* path entry in PATH env variable */
	char delimiter[2] = ":";	/* delimiter for directories in dirs argument */

	/* Allocate space for the program's absolute path */
	cmd = (char *)malloc(PATH_MAX - 1);

	/* Search each path entry in PATH until we find our program. */
	absl_path = strtok(dirs, delimiter);
	while (absl_path != NULL) {
		verbose_printf(stdout, "xcrun: info: checking directory \'%s\' for command \'%s\'...\n", absl_path, name);

		/* Construct our program's absolute path. */
		sprintf(cmd, "%s/%s", absl_path, name);

		/* Does it exist? Is it an executable? */
		if (access(cmd, X_OK) == 0) {
			verbose_printf(stdout, "xcrun: info: found command's absolute path: \'%s\'\n", cmd);
			break;
		}

		/* If not, move onto the next entry.. */
		absl_path = strtok(NULL, delimiter);
	}

	return cmd;
}

/**
 * @func request_command - Request a program.
 * @arg name -- name of program
 * @arg argv -- arguments to be passed if program found
 * @return: -1 on failed search, 0 on successful search, no return on execute
 */
static int request_command(const char *name, int argc, char *argv[])
{
	char *toolch_name = NULL;				/* toolchain name to be used with sdk */
	char *env_path = NULL;					/* used to hold our PATH env variable */
	char *cmd = NULL;						/* used to hold our command's absolute path */
	char search_string[PATH_MAX * 1024];	/* our search string */
	char sdkinfo_path[PATH_MAX - 1];		/* path to sdk's info.ini */

	/* Read our PATH environment variable. */
	if ((env_path = getenv("PATH")) == NULL) {
		fprintf(stderr, "xcrun: error: failed to read PATH variable.\n");
		return -1;
	}

	/* If we specified an sdk, search the sdk and it's associated toolchain */
	if (explicit_sdk_mode == 1) {
		sprintf(sdkinfo_path, "%s/info.ini", get_sdk_path(current_sdk));
		toolch_name = strdup(get_sdk_info(sdkinfo_path).toolchain);
		sprintf(search_string, "%s/usr/bin:%s/usr/bin", get_sdk_path(current_sdk), get_toolchain_path(toolch_name));
		goto do_search;
	}

	/* If we specified a toolchain, only search the toolchain */
	if (explicit_toolchain_mode == 1) {
		sprintf(search_string, "%s/usr/bin", get_toolchain_path(current_toolchain));
		goto do_search;
	}

	/* By default, we search our rootfs and developer dir only */
	sprintf(search_string, "%s:%s/usr/bin", env_path, developer_dir);

	/* Search each path entry in PATH until we find our program. */
do_search:
	if ((cmd = search_command(name, search_string)) != NULL) {
			if (finding_mode == 1) {
				fprintf(stdout, "%s\n", cmd);
				free(cmd);
				return 0;
			} else {
				call_command(cmd, argc, argv);
				/* NOREACH */
				fprintf(stderr, "xcrun: error: can't exec \'%s\' (errno=%s)\n", cmd, strerror(errno));
				free(cmd);
				return -1;
			}
	}

	/* We have searched everywhere, but we haven't found our program. State why. */
	fprintf(stderr, "xcrun: error: can't stat \'%s\' (errno=%s)\n", name, strerror(errno));

	return -1;
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
	char *sdk = NULL;
	char *toolchain = NULL;
	char *tool_called = NULL;
	char sdkinfo_path[PATH_MAX - 1];

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
								explicit_sdk_mode = 1;
								sdk = optarg;
								/* we support absolute paths and short names */
								if (*sdk == '/') {
									current_sdk = (char *)malloc(strlen(basename(sdk)));
									(void)stripext(current_sdk, basename(sdk));
								} else {
									current_sdk = (char *)malloc(strlen(basename(sdk)));
									(void)stripext(current_sdk, sdk);
								}
							} else {
								fprintf(stderr, "xcrun: error: sdk flag requires an argument.\n");
								exit(1);
							}
							break;
						case 4: /* --toolchain */
							if (*optarg != '-') {
								++argc_offset;
								explicit_toolchain_mode = 1;
								toolchain = optarg;
								/* we support absolute paths and short names */
								if (*toolchain == '/') {
									current_toolchain = (char *)malloc(strlen(basename(toolchain)));
									(void)stripext(current_toolchain, basename(toolchain));
								} else {
									current_toolchain = (char *)malloc(strlen(basename(toolchain)));
									(void)stripext(current_toolchain, toolchain);
								}
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

	/* If we get here, we need developer dir info */
	developer_dir = get_developer_path();

	/* Show SDK path? */
	if (ssdkp_f == 1) {
		if (sdk == NULL)
			printf("%s\n", get_sdk_path(get_default_info(XCRUN_DEFAULT_CFG).sdkname));
		else
			printf("%s\n", get_sdk_path(sdk));
		exit(0);
	}

	/* Show SDK version? */
	if (ssdkv_f == 1) {
		if (sdk == NULL)
			printf("%s SDK version %s\n", get_default_info(XCRUN_DEFAULT_CFG).sdkname, get_default_info(XCRUN_DEFAULT_CFG).sdkvers);
		else {
			memset(sdkinfo_path, 0, sizeof(sdkinfo_path));

			/* -sdk flag supports both relative names and absolute paths. */
			if (*sdk != '/')
				sprintf(sdkinfo_path, "%s/info.ini", get_sdk_path(sdk));
			else {
				if (validate_directory_path(sdk) != (-1))
					sprintf(sdkinfo_path, "%s/info.ini", sdk);
				else {
					fprintf(stderr, "xcrun: error: \'%s\' is not a valid path\n", sdk);
					exit(1);
				}
			}

			printf("%s SDK version %s\n", get_sdk_info(sdkinfo_path).name, get_sdk_info(sdkinfo_path).version);
		}
		exit(0);
	}

	/* Show SDK platform path? */
	if (ssdkpp_f == 1) {
		printf("%s\n", developer_dir);
		exit(0);
	}

	/* Show SDK platform version? */
	if (ssdkpv_f == 1) {
		if (sdk == NULL)
			printf("%s SDK Platform version %s\n", get_default_info(XCRUN_DEFAULT_CFG).sdkname, get_default_info(XCRUN_DEFAULT_CFG).sdkvers);
		else
			printf("%s SDK Platform version %s\n", get_sdk_info(get_sdk_path(sdk)).name, get_sdk_info(get_sdk_path(sdk)).version);
		exit(0);
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


	/* If toolchain isn't set, use default. */
	if (toolchain == NULL)
		toolchain = (char *)get_default_info(XCRUN_DEFAULT_CFG).toolchname;

	/* If sdk isn't set, use default. */
	if (sdk == NULL)
		sdk = (char *)get_default_info(XCRUN_DEFAULT_CFG).sdkname;

	/* Search for program? */
	if (find_f == 1) {
		finding_mode = 1;
		if (request_command(tool_called, 0, NULL) != -1)
			retval = 0;
		else
			fprintf(stderr, "xcrun: error: unable to locate command \'%s\'.\n", tool_called);
			exit(1);
	}

	/* Search and execute program. (default behavior) */
	if (request_command(tool_called, ((argc - argc_offset) - (argc - argc_offset)),  (argv += ((argc - argc_offset) - (argc - argc_offset) + (argc_offset)))) != -1) {
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

	/* Strip out any path name that may have been passed into argv[0] */
	this_tool = basename(argv[0]);
	progname = this_tool;

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
			/* Retrieve developer dir info */
			developer_dir = get_developer_path();

			/* Locate and execute the command */
			if (request_command(this_tool, argc, argv) != -1)
				retval = 0;
			else {
				fprintf(stderr, "xcrun: error: failed to execute command \'%s\'. aborting.\n", this_tool);
			}
			break;
	}

	return retval;
}
