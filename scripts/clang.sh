#!/bin/bash

##
# clang/clang++ wrapper for cross-compiling.
# This script is invoked by xcrun upon calling clang.
##

TOOL=`/usr/bin/xcrun -sdk / -find ${0}`
TOOLCHAIN_DIR=`/usr/bin/xcrun --show-sdk-toolchain-path`

${TOOL} -target ${TARGET_TRIPLE} -isysroot ${SDKROOT} -B${TOOLCHAIN_DIR}/usr/bin ${@}

exit ${?}
