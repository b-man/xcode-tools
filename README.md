xcode-tools
===========
An open source reimplementation of Apple's command-line xcode utilities
-----------------------------------------------------------------------

Currently implemented utilities:
* xcrun
* xcode-select

How to use these utilities
--------------------------

[xcode-select]:
---------------

xcode-select is a simple utility used to specify the location of a Developer folder.

* What is a Developer folder? and why do we need to select one?
  A Developer folder is a special folder that contains your SDK and it's associated Toolchain.
  By selecting a Developer folder, tools like ```xcrun``` will be able to locate resources found in the Developer folder.

* How does this tool work?
  When you select a Develoepr folder, it's absolute path will be written to a configuration file called ~/.xcdev.dat for other utilities to use.
  NOTE: It is not recommended that you modify this file directly!

* How do I use this tool?
  The functionality of xcode-select is almost identical to that of Apple's xcode-select utility.

  To select a Developer folder, simply run ```xcode-select --switch /path/to/DevFolder```.
  To display the absolute path of a Developer folder, simply run ```xcode-select --print-path```.
  Other options are ```--version``` to display version information and ```--help``` to display usage information.

  If you only wish to temporarily override the Developer folder path, set the ```DEVELOPER_DIR``` environment variable 
  in your shell to point to the absolute path of your desired Developer folder.

[xcrun]:
--------

xcrun is a simple utility used to locate and/or execute Developer, Toolchain, and SDK tools.

* How does this tool work?
  xcrun works by searching and optionally executing tools located within the Developer folder.
  When xcrun is invoked, it will search paths in the following order:

	```
	/<DevFolder>/usr/bin
	/<DevFolder>/SDKs/<specified sdk>.sdk/usr/bin
	/<DevFolder>/Toolchains/<associated toolchain>.toolchain/usr/bin
	```
	
  Where <DevFolder> represents the Developer folder, and <specified sdk> and <associated toolchain> represents the
  specified SDK and associated toolchain.

  If no SDK is specified while running xcrun, a default SDK which is specified in /etc/xcrun.ini will be used.

  When xcrun is told to use an sdk that is either specified by the user or specified by /etc/xcrun.ini, xcrun will read
  a configuration file called ```/<DevFolder>/SDKs/<specified sdk>.sdk/info.ini``` which is used to resolve the toolchain name and the deployment target used.

  Below is an example of an info.ini file found in an SDK folder. For this example, our SDK name is DarwinARM, and the file name combined with it's
  absolute path will be /Developer/SDKs/DarwnARM.sdk/info.ini:

	```
	[SDK]				; this is an sdk configuration file
	name = DarwinARM		; this is the name of our SDK. This MUST match the folder name that is postfixed with '.sdk'.
	version = 1.0.0			; this is the version number for the SDK.
	toolchain = DarwinARM		; this is the specified toolchain name to be used with the SDK.
	deployment_target = 10.7	; this is the deployment target to be used with the SDK.
					; (currently only supports OSX versioning, but can be overridden as explained later.
	```

  NOTE: All information written into info.ini is case-sensitive (except comments), so take extra care when writing your own.

  After xcrun has retrieved the information found in the SDK's info.ini, it will then validate the toolchain to use for the SDK by
  parsing the Toolchain's info.ini, which is assumed to be located in ```/<DevPath>/Toolchains/<associated toolchain>.toolchain.

  Below is an example of an info.ini file found in the Toolchain folder. For this example, our Toolchain name is DarwinARM, and the file name combined with it's
  absolute path will be /Developer/Toolchains/DarwinARM.toolchain/info.ini:

	```
	[TOOLCHAIN]		; this is a toolchain configuration file
	name = DarwinARM	; this is the name of our Toolchain. This MUST match the folder name that is postfixed with '.toolchain'.
	version = 1.0.0		; this is the version number for the toolchain
	```

  After the SDK and Toolchain paths have been resolved, xcrun will then proceed to search the resolved paths for the tool called.
  Assuming that our SDK and Toolchain information matches the examples shown above, xcrun will search the following paths for the tool:

	```
  	/Developer/usr/bin
  	/Developer/SDKs/DarwinARM.sdk/usr/bin
  	/Developer/Toolchains/DarwinARM.toolchain/usr/bin
	```

  By default, xcrun will locate and execute tools that are passed to it. If you only wish to find a tool's path, you must use the ```--find```
  option followed by the tool name when invoking xcrun. This is further explained in the next section.

  Once xcrun has located a tool to be executed, it will pass the following environment variables to the called tool:

	```PATH```				- This contains all resolved paths for the Developer folder, the SDK, and the Toolchain.
	```IOS_DEPLOYMENT_TARGET```		- If MACOSX_DEPLOYMENT_TARGET isn't specified and this is, this will be passed to the called tool.
	```MACOSX_DEPLOYMENT_TARGET```	- If IOS_DEPLOYMENT_TARGET isn't specified and this is, this will be passed to the called tool.

  If ```IOS_DEPLOYMENT_TARGET``` or ```MACOSX_DEPLOYMENT_TARGET``` are set in your shell, the deployment target specified by the SDK will be overridden.
  NOTE: Ensure that only one of these variables are set at a time if they are used, otherwise things may break!

* How do I use this tool?

  Below is a list of all supported options:

  ```
  -h, --help                  show this help message and exit
  --version                   show the xcrun version
  -v, --verbose               show verbose logging output
  --sdk <sdk name>            find the tool for the given SDK name
  --toolchain <name>          find the tool for the given toolchain
  -l, --log                   show commands to be executed (with --run)
  -f, --find                  only find and print the tool path
  -r, --run                   find and execute the tool (the default behavior)
  --show-sdk-path             show selected SDK install path
  --show-sdk-version          show selected SDK version
  --show-sdk-platform-path    show selected SDK platform path
  --show-sdk-platform-version show selected SDK platform version
  ```

  Examples:
  ---------

  * Running the Developer tool ```image3maker```:

	```xcrun image3maker -f mach_kernel -t krnl -o mach.img3```

  * Running the Toolchain tool ```otool``` with the default SDK (implying it's associated Toolchain):

	```xcrun otool -L /Developer/SDKs/DarwinARM.sdk/usr/lib/system/libsystem_kernel.dylib```

  * Running the Toolchain tool ```clang``` with the DarwinARM SDK:

	```xcrun -sdk DarwinARM clang -arch armv7 -x c -g -O2 hello.c -o hello -lSystem```

  * Running the Toolcain tool ```clang``` with the DarwinARM Toolchain:

	```xcrun -toolchain DarwinARM clang -arch armv7 -x c -g -O2 hello.c -o hello -lSystem```

  xcrun also supports multicall behavior. Below is a small list of symbolic links to xcrun that exhibit special behavior:

	```xcrun_verbose```	- calls xcrun in verbose mode, like passing the --verbose option to xcrun

	```xcrun_logging```	- calls xcrun in logging mode, like passing the --log option to xcrun

  You may also create a symbolic link to xcrun that is the same name as a tool that may be found in the Developer folder or default SDK or Toolchain folders.
  For example:

	```
	$ ln -s /usr/bin/xcrun ./image3maker 
	$ ./image3maker
	No input file
	Usage: image3maker [options]

	Generate an Image3 file.

	  -c, --certificateBlob [file]        Use file as a certificate to add to the image.
	  -f, --dataFile [file]               Use file as an input. (required)
	...
	```

  NOTE: If this is your first time using this version of xcrun and you run into an error starting with ```xcrun: error: unable to validate path```,
  ensure that xcrun is searching the developer folder by running ```xcode-select --switch <DevPath>```, where <DevPath> is the absolute path to your
  developer folder. If you still run into problems, open an issue report and maybe I can help you. :)

