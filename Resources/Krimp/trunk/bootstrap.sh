CMAKE_DEFS="-DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=${PWD} $CMAKE_DEFS"

rm -rf build

if [ ! -d build ]; then
	mkdir build
fi

cd build && cmake .. ${CMAKE_DEFS}
