# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/dtai/Downloads/ganak-master

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/dtai/Downloads/ganak-master/build

# Include any dependencies generated for this target.
include CMakeFiles/ganak.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ganak.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ganak.dir/flags.make

CMakeFiles/ganak.dir/src/main.cpp.o: CMakeFiles/ganak.dir/flags.make
CMakeFiles/ganak.dir/src/main.cpp.o: ../src/main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dtai/Downloads/ganak-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/ganak.dir/src/main.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ganak.dir/src/main.cpp.o -c /home/dtai/Downloads/ganak-master/src/main.cpp

CMakeFiles/ganak.dir/src/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ganak.dir/src/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dtai/Downloads/ganak-master/src/main.cpp > CMakeFiles/ganak.dir/src/main.cpp.i

CMakeFiles/ganak.dir/src/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ganak.dir/src/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dtai/Downloads/ganak-master/src/main.cpp -o CMakeFiles/ganak.dir/src/main.cpp.s

CMakeFiles/ganak.dir/src/main.cpp.o.requires:

.PHONY : CMakeFiles/ganak.dir/src/main.cpp.o.requires

CMakeFiles/ganak.dir/src/main.cpp.o.provides: CMakeFiles/ganak.dir/src/main.cpp.o.requires
	$(MAKE) -f CMakeFiles/ganak.dir/build.make CMakeFiles/ganak.dir/src/main.cpp.o.provides.build
.PHONY : CMakeFiles/ganak.dir/src/main.cpp.o.provides

CMakeFiles/ganak.dir/src/main.cpp.o.provides.build: CMakeFiles/ganak.dir/src/main.cpp.o


# Object files for target ganak
ganak_OBJECTS = \
"CMakeFiles/ganak.dir/src/main.cpp.o"

# External object files for target ganak
ganak_EXTERNAL_OBJECTS =

ganak: CMakeFiles/ganak.dir/src/main.cpp.o
ganak: CMakeFiles/ganak.dir/build.make
ganak: src/liblibganak.a
ganak: /usr/lib/x86_64-linux-gnu/libgmp.so
ganak: /usr/lib/x86_64-linux-gnu/libgmpxx.so
ganak: src/clhash/libclhash.a
ganak: src/component_types/libcomponent_types.a
ganak: CMakeFiles/ganak.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dtai/Downloads/ganak-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ganak"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ganak.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ganak.dir/build: ganak

.PHONY : CMakeFiles/ganak.dir/build

CMakeFiles/ganak.dir/requires: CMakeFiles/ganak.dir/src/main.cpp.o.requires

.PHONY : CMakeFiles/ganak.dir/requires

CMakeFiles/ganak.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ganak.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ganak.dir/clean

CMakeFiles/ganak.dir/depend:
	cd /home/dtai/Downloads/ganak-master/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dtai/Downloads/ganak-master /home/dtai/Downloads/ganak-master /home/dtai/Downloads/ganak-master/build /home/dtai/Downloads/ganak-master/build /home/dtai/Downloads/ganak-master/build/CMakeFiles/ganak.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ganak.dir/depend

