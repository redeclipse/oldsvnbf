#!/bin/bash
# BF_DIR should refer to the directory in which Blood Frontier is placed.
#BF_DIR=~/bloodfrontier
#BF_DIR=/usr/local/bloodfrontier
BF_DIR=.

# BF_OPTIONS contains any command line options you would like to start Blood Frontier with.
#BF_OPTIONS="-f"
BF_OPTIONS="-q${HOME}/.bloodfrontier -r"

if [ -e "bin/bloodfrontier_client" ]; then
	exec bin/bloodfrontier_client ${BF_OPTIONS} $@
else
	if [ -e "${BF_DIR}/bin/bloodfrontier_client" ]; then
		pushd ${BF_DIR}
		exec bin/bloodfrontier_client ${BF_OPTIONS} $@
		popd
	else
		echo "Your platform does not have a pre-compiled Blood Frontier client."
		echo "Please follow the following steps to build a native client:"
		echo "1) Ensure you have the SDL, SDL-image, fmodex, and OpenGL *DEVELOPMENT* libraries installed."
		echo "2) Change directory to mod/src/ and type \"make clean all\"."
		echo "3) If the build succeeds, return to this directory and run this script again."
		exit 1
	fi
fi

