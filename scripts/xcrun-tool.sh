#!/bin/bash

if [ `basename ${0}` == "xcrun-tool" ]; then
	echo "xcrun-tool: error: this tool must not be called directly."
	exit 1
fi

TARGET_TRIPLE=`xcrun llvm-config --host-target`
TOOL=`/usr/bin/xcrun -find ${0/${TARGET_TRIPLE}-/}`

${TOOL} ${@}

exit ${?}
