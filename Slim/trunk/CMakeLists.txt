# Set the minimum required version of cmake for a project.
cmake_minimum_required (VERSION 2.8.0)

# Set a name for the entire project.
project (fic)

IF(UNIX)
	# C++0x is the unofficial name of the planned new standard for the C++ programming language.
	# --include is GCCs alternative to Visual Studios ForcedIncludeFiles.
#	set (CMAKE_CXX_FLAGS "-m32 -std=c++0x -fopenmp --include ${PROJECT_SOURCE_DIR}/LocalFic.h")
	set (CMAKE_CXX_FLAGS "--std=c++0x -fopenmp --include ${PROJECT_SOURCE_DIR}/LocalFic.h")
	IF (EXISTS /etc/gentoo-release)
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nopie")
	ENDIF(EXISTS /etc/gentoo-release)
	# Otherwise we run into problems using "/usr/include/stdlib.h"
	add_definitions (-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)
ENDIF(UNIX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include (AddCompilerFlag)
include (OptimizeForArchitecture)
#OptimizeForArchitecture() # doesn't work properly, so let's do it mannualy

IF(NOT CMAKE_BUILD_TYPE MATCHES Debug)
AddCompilerFlag(-m64)
AddCompilerFlag(-mavx)
AddCompilerFlag(-march=native)
#AddCompilerFlag(-march=corei7-avx)
#AddCompilerFlag(-mtune=native)
#AddCompilerFlag(-mtune=corei7-avx)
AddCompilerFlag(-msse2)
AddCompilerFlag(-mno-sse3)
AddCompilerFlag(-mno-sse4.1)
AddCompilerFlag(-mno-sse4.2)
AddCompilerFlag(-mno-sse4a)
AddCompilerFlag(-mno-avx)
#AddCompilerFlag(-O0)
#AddCompilerFlag(-O1)
#AddCompilerFlag(-O2)
#AddCompilerFlag(-O3)
#AddCompilerFlag(-Os)
AddCompilerFlag(-Ofast)
ENDIF(NOT CMAKE_BUILD_TYPE MATCHES Debug)

# Add subdirectories of the libraries to the build.
add_subdirectory (qtils)
add_subdirectory (bass)
add_subdirectory (croupier)

# Specify source files for the 'fic' binary.
# Note: GLOB will generate a list of all files that match the globbing expressions and store it into the variable.
file (GLOB_RECURSE FIC_SRCS
	blocks/*.cpp
	tasks/*.cpp
)
set (FIC_SRCS ${FIC_SRCS}
	FicConf.cpp
	FicMain.cpp
	ficmkii.cpp
)

# Add include directories to the build.
include_directories (qtils)
include_directories (bass)
include_directories (croupier)

# Define executable and libraries.
set (FIC_BIN fic)
set (FIC_LIBS Qtils Bass Croupier)

# Add an executable to the project using the specified source files.
add_executable (${FIC_BIN} ${FIC_SRCS}) 

# Link a target to given libraries.
target_link_libraries (${FIC_BIN} ${FIC_LIBS})


# Relax PaX for debug.
IF (EXISTS /etc/gentoo-release)
	ADD_CUSTOM_COMMAND (TARGET ${FIC_BIN} POST_BUILD COMMAND /sbin/paxctl -m ${FIC_BIN})
ENDIF(EXISTS /etc/gentoo-release)

# Specify rules to run at install time.
#install (TARGETS ${FIC_BIN} RUNTIME DESTINATION bin)
install (TARGETS ${FIC_BIN} RUNTIME DESTINATION .)

