#!/bin/sh
# BF_DIR should refer to the directory in which Blood Frontier is placed.
#BF_DIR=~/bloodfrontier
#BF_DIR=/usr/local/bloodfrontier
BF_DIR=.

# BF_OPTIONS contains any command line options you would like to start Blood Frontier with.
#BF_OPTIONS="-f"
BF_OPTIONS="-h${HOME}/.bloodfrontier -rinit.cfg"

if [ -e "bin/bfclient" ]; then
	./bin/bfclient ${BF_OPTIONS} "$@"
else
	if [ -e "${BF_DIR}/bin/bfclient" ]; then
		cd ${BF_DIR}
		./bin/bfclient ${BF_OPTIONS} "$@"
	else
		echo "Your platform does not have a pre-compiled Blood Frontier client."
		echo -n "Would you like to build one now? [Yn] "
		read CC
		if [ "${CC}" != "n" ]; then
			cd src
			make clean install
			echo "Build complete, please try running the script again."
		else
			echo "Please follow the following steps to build:"
			echo "1) Ensure you have the SDL, SDL image, SDL mixer, zlib, and OpenGL *DEVELOPMENT* libraries installed."
			echo "2) Change directory to src/ and type \"make clean install\" or \"gmake clean client client-install\"."
			echo "3) If the build succeeds, return to this directory and run this script again."
			exit 1
		fi
	fi
fi

