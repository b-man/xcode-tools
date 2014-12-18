#!/bin/bash

if [ `basename ${0}` == "xcrun-tool" ]; then
	echo "xcrun-tool: error: this tool must not be called directly."
	exit 1
fi

TARGET_TRIPLE=`/usr/bin/xcrun --show-sdk-target-triple`
TOOL=`/usr/bin/xcrun -find ${0/${TARGET_TRIPLE}-/}`

${TOOL} "${@}"

exit ${?}
