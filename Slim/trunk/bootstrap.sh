if [ $(uname) == 'Linux' ]
then
if [ -e /etc/gentoo-release ]
then
	export LDFLAGS="-nopie"
elif [ -e /etc/redhat-release ]
then
	CMAKE_DEFS="-DCMAKE_CXX_COMPILER=g++44"
fi
elif [ $(uname) == 'Darwin' ]
then
	# sudo ports install gcc44
	CMAKE_DEFS="-DCMAKE_CXX_COMPILER=/opt/local/bin/g++-mp-4.4"
fi
CMAKE_DEFS="-DCMAKE_INSTALL_PREFIX=${PWD} $CMAKE_DEFS"
if [ $# == 1 ]
then
	case $1 in
	'debug')
		CMAKE_DEFS="-DCMAKE_BUILD_TYPE=Debug $CMAKE_DEFS"
		;;
	'release')
		CMAKE_DEFS="-DCMAKE_BUILD_TYPE=Release $CMAKE_DEFS"
		;;
	'static')
		CMAKE_DEFS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS=-static $CMAKE_DEFS"
		;;	
	esac
fi

rm -rf build

mkdir build

cd build && cmake .. ${CMAKE_DEFS}
