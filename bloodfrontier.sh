#!/bin/sh
# BF_DATA should refer to the directory in which Blood Frontier data files are placed.
#BF_DATA=~/bloodfrontier
#BF_DATA=/usr/local/bloodfrontier
BF_DATA=.

# BF_BIN should refer to the directory in which Blood Frontier executable files are placed.
BF_BIN=${BF_DATA}/bin

# BF_OPTIONS contains any command line options you would like to start Blood Frontier with.
#BF_OPTIONS="-f"
BF_OPTIONS="-r"

# SYSTEM_NAME should be set to the name of your operating system.
#SYSTEM_NAME=Linux
SYSTEM_NAME=`uname -s`

# MACHINE_NAME should be set to the name of your processor.
#MACHINE_NAME=i686
MACHINE_NAME=`uname -m`

case ${SYSTEM_NAME} in
Linux)
	SYSTEM_SUFFIX=_linux
	;;
*)
	SYSTEM_SUFFIX=_unknown
	;;
esac

case ${MACHINE_NAME} in
i486|i586|i686)
	MACHINE_SUFFIX=
	;;
x86_64)
	MACHINE_SUFFIX=_64
	;;
*)
	if [ ${SYSTEM_SUFFIX} != _native ]
	then
		SYSTEM_SUFFIX=_native
	fi
	MACHINE_SUFFIX=
	;;
esac

if [ -x ${BF_BIN}/bfclient_native ]
then
	SYSTEM_SUFFIX=_native
	MACHINE_SUFFIX=
fi


if [ -x ${BF_BIN}/bfclient${SYSTEM_SUFFIX}${MACHINE_SUFFIX} ]; 
then
	cd ${BF_DATA}
	exec ${BF_BIN}/bfclient${SYSTEM_SUFFIX}{$MACHINE_SUFFIX} ${BF_OPTIONS} "$@"
else
	echo "Your platform does not have a pre-compiled Blood Frontier client."
	echo -n "Would you like to build one now? [Yn] "
	read CC
	if [ "${CC}" != "n" ]; then
		cd ${BF_DATA}/src
		make clean install
		echo "Build complete, please try running the script again."
	else
		echo "Please follow the following steps to build:"
		echo "1) Ensure you have the SDL, SDL image, SDL mixer, zlib, and OpenGL *DEVELOPMENT* libraries installed."
		echo "2) Change directory to src/ and type \"make clean install\"."
		echo "3) If the build succeeds, return to this directory and run this script again."
		exit 1
	fi
fi

