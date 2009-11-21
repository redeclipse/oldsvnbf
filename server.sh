#!/bin/sh
# BF_DIR should refer to the directory in which Blood Frontier is placed.
#BF_DIR=~/bloodfrontier
#BF_DIR=/usr/local/bloodfrontier
BF_DIR=.

# BF_OPTIONS contains any command line options you would like to start Blood Frontier with.
BF_OPTIONS="-sc8"

if [ -e "bin/bfserver" ]; then
	exec bin/bfserver ${BF_OPTIONS} "$@"
else
	if [ -e "${BF_DIR}/bin/bfserver" ]; then
		cd ${BF_DIR}
		exec bin/bfserver ${BF_OPTIONS} "$@"
	else
		echo "Your platform does not have a pre-compiled Blood Frontier server."
		echo -n "Would you like to build one now? [Yn] "
		read CC
		if [ "${CC}" != "n" ]; then
			cd src
			make clean install
			echo "Build complete, please try running the script again."
		else
			echo "Please follow the following steps to build:"
			echo "1) Ensure you have the zlib *DEVELOPMENT* libraries installed."
			echo "2) Change directory to src/ and type \"make clean install\" or \"gmake clean server server-install\"."
			echo "3) If the build succeeds, return to this directory and run this script again."
			exit 1
		fi
	fi
fi

