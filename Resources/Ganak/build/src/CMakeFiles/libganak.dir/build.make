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
include src/CMakeFiles/libganak.dir/depend.make

# Include the progress variables for this target.
include src/CMakeFiles/libganak.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/libganak.dir/flags.make

src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o: src/CMakeFiles/libganak.dir/flags.make
src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o: ../src/alt_component_analyzer.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dtai/Downloads/ganak-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o -c /home/dtai/Downloads/ganak-master/src/alt_component_analyzer.cpp

src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/libganak.dir/alt_component_analyzer.cpp.i"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dtai/Downloads/ganak-master/src/alt_component_analyzer.cpp > CMakeFiles/libganak.dir/alt_component_analyzer.cpp.i

src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/libganak.dir/alt_component_analyzer.cpp.s"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dtai/Downloads/ganak-master/src/alt_component_analyzer.cpp -o CMakeFiles/libganak.dir/alt_component_analyzer.cpp.s

src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o.requires:

.PHONY : src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o.requires

src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o.provides: src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/libganak.dir/build.make src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o.provides.build
.PHONY : src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o.provides

src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o.provides.build: src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o


src/CMakeFiles/libganak.dir/component_analyzer.cpp.o: src/CMakeFiles/libganak.dir/flags.make
src/CMakeFiles/libganak.dir/component_analyzer.cpp.o: ../src/component_analyzer.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dtai/Downloads/ganak-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/CMakeFiles/libganak.dir/component_analyzer.cpp.o"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/libganak.dir/component_analyzer.cpp.o -c /home/dtai/Downloads/ganak-master/src/component_analyzer.cpp

src/CMakeFiles/libganak.dir/component_analyzer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/libganak.dir/component_analyzer.cpp.i"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dtai/Downloads/ganak-master/src/component_analyzer.cpp > CMakeFiles/libganak.dir/component_analyzer.cpp.i

src/CMakeFiles/libganak.dir/component_analyzer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/libganak.dir/component_analyzer.cpp.s"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dtai/Downloads/ganak-master/src/component_analyzer.cpp -o CMakeFiles/libganak.dir/component_analyzer.cpp.s

src/CMakeFiles/libganak.dir/component_analyzer.cpp.o.requires:

.PHONY : src/CMakeFiles/libganak.dir/component_analyzer.cpp.o.requires

src/CMakeFiles/libganak.dir/component_analyzer.cpp.o.provides: src/CMakeFiles/libganak.dir/component_analyzer.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/libganak.dir/build.make src/CMakeFiles/libganak.dir/component_analyzer.cpp.o.provides.build
.PHONY : src/CMakeFiles/libganak.dir/component_analyzer.cpp.o.provides

src/CMakeFiles/libganak.dir/component_analyzer.cpp.o.provides.build: src/CMakeFiles/libganak.dir/component_analyzer.cpp.o


src/CMakeFiles/libganak.dir/component_cache.cpp.o: src/CMakeFiles/libganak.dir/flags.make
src/CMakeFiles/libganak.dir/component_cache.cpp.o: ../src/component_cache.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dtai/Downloads/ganak-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object src/CMakeFiles/libganak.dir/component_cache.cpp.o"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/libganak.dir/component_cache.cpp.o -c /home/dtai/Downloads/ganak-master/src/component_cache.cpp

src/CMakeFiles/libganak.dir/component_cache.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/libganak.dir/component_cache.cpp.i"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dtai/Downloads/ganak-master/src/component_cache.cpp > CMakeFiles/libganak.dir/component_cache.cpp.i

src/CMakeFiles/libganak.dir/component_cache.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/libganak.dir/component_cache.cpp.s"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dtai/Downloads/ganak-master/src/component_cache.cpp -o CMakeFiles/libganak.dir/component_cache.cpp.s

src/CMakeFiles/libganak.dir/component_cache.cpp.o.requires:

.PHONY : src/CMakeFiles/libganak.dir/component_cache.cpp.o.requires

src/CMakeFiles/libganak.dir/component_cache.cpp.o.provides: src/CMakeFiles/libganak.dir/component_cache.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/libganak.dir/build.make src/CMakeFiles/libganak.dir/component_cache.cpp.o.provides.build
.PHONY : src/CMakeFiles/libganak.dir/component_cache.cpp.o.provides

src/CMakeFiles/libganak.dir/component_cache.cpp.o.provides.build: src/CMakeFiles/libganak.dir/component_cache.cpp.o


src/CMakeFiles/libganak.dir/component_management.cpp.o: src/CMakeFiles/libganak.dir/flags.make
src/CMakeFiles/libganak.dir/component_management.cpp.o: ../src/component_management.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dtai/Downloads/ganak-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object src/CMakeFiles/libganak.dir/component_management.cpp.o"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/libganak.dir/component_management.cpp.o -c /home/dtai/Downloads/ganak-master/src/component_management.cpp

src/CMakeFiles/libganak.dir/component_management.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/libganak.dir/component_management.cpp.i"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dtai/Downloads/ganak-master/src/component_management.cpp > CMakeFiles/libganak.dir/component_management.cpp.i

src/CMakeFiles/libganak.dir/component_management.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/libganak.dir/component_management.cpp.s"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dtai/Downloads/ganak-master/src/component_management.cpp -o CMakeFiles/libganak.dir/component_management.cpp.s

src/CMakeFiles/libganak.dir/component_management.cpp.o.requires:

.PHONY : src/CMakeFiles/libganak.dir/component_management.cpp.o.requires

src/CMakeFiles/libganak.dir/component_management.cpp.o.provides: src/CMakeFiles/libganak.dir/component_management.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/libganak.dir/build.make src/CMakeFiles/libganak.dir/component_management.cpp.o.provides.build
.PHONY : src/CMakeFiles/libganak.dir/component_management.cpp.o.provides

src/CMakeFiles/libganak.dir/component_management.cpp.o.provides.build: src/CMakeFiles/libganak.dir/component_management.cpp.o


src/CMakeFiles/libganak.dir/instance.cpp.o: src/CMakeFiles/libganak.dir/flags.make
src/CMakeFiles/libganak.dir/instance.cpp.o: ../src/instance.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dtai/Downloads/ganak-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object src/CMakeFiles/libganak.dir/instance.cpp.o"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/libganak.dir/instance.cpp.o -c /home/dtai/Downloads/ganak-master/src/instance.cpp

src/CMakeFiles/libganak.dir/instance.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/libganak.dir/instance.cpp.i"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dtai/Downloads/ganak-master/src/instance.cpp > CMakeFiles/libganak.dir/instance.cpp.i

src/CMakeFiles/libganak.dir/instance.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/libganak.dir/instance.cpp.s"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dtai/Downloads/ganak-master/src/instance.cpp -o CMakeFiles/libganak.dir/instance.cpp.s

src/CMakeFiles/libganak.dir/instance.cpp.o.requires:

.PHONY : src/CMakeFiles/libganak.dir/instance.cpp.o.requires

src/CMakeFiles/libganak.dir/instance.cpp.o.provides: src/CMakeFiles/libganak.dir/instance.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/libganak.dir/build.make src/CMakeFiles/libganak.dir/instance.cpp.o.provides.build
.PHONY : src/CMakeFiles/libganak.dir/instance.cpp.o.provides

src/CMakeFiles/libganak.dir/instance.cpp.o.provides.build: src/CMakeFiles/libganak.dir/instance.cpp.o


src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o: src/CMakeFiles/libganak.dir/flags.make
src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o: ../src/new_component_analyzer.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dtai/Downloads/ganak-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/libganak.dir/new_component_analyzer.cpp.o -c /home/dtai/Downloads/ganak-master/src/new_component_analyzer.cpp

src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/libganak.dir/new_component_analyzer.cpp.i"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dtai/Downloads/ganak-master/src/new_component_analyzer.cpp > CMakeFiles/libganak.dir/new_component_analyzer.cpp.i

src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/libganak.dir/new_component_analyzer.cpp.s"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dtai/Downloads/ganak-master/src/new_component_analyzer.cpp -o CMakeFiles/libganak.dir/new_component_analyzer.cpp.s

src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o.requires:

.PHONY : src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o.requires

src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o.provides: src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/libganak.dir/build.make src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o.provides.build
.PHONY : src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o.provides

src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o.provides.build: src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o


src/CMakeFiles/libganak.dir/solver.cpp.o: src/CMakeFiles/libganak.dir/flags.make
src/CMakeFiles/libganak.dir/solver.cpp.o: ../src/solver.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dtai/Downloads/ganak-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object src/CMakeFiles/libganak.dir/solver.cpp.o"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/libganak.dir/solver.cpp.o -c /home/dtai/Downloads/ganak-master/src/solver.cpp

src/CMakeFiles/libganak.dir/solver.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/libganak.dir/solver.cpp.i"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dtai/Downloads/ganak-master/src/solver.cpp > CMakeFiles/libganak.dir/solver.cpp.i

src/CMakeFiles/libganak.dir/solver.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/libganak.dir/solver.cpp.s"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dtai/Downloads/ganak-master/src/solver.cpp -o CMakeFiles/libganak.dir/solver.cpp.s

src/CMakeFiles/libganak.dir/solver.cpp.o.requires:

.PHONY : src/CMakeFiles/libganak.dir/solver.cpp.o.requires

src/CMakeFiles/libganak.dir/solver.cpp.o.provides: src/CMakeFiles/libganak.dir/solver.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/libganak.dir/build.make src/CMakeFiles/libganak.dir/solver.cpp.o.provides.build
.PHONY : src/CMakeFiles/libganak.dir/solver.cpp.o.provides

src/CMakeFiles/libganak.dir/solver.cpp.o.provides.build: src/CMakeFiles/libganak.dir/solver.cpp.o


src/CMakeFiles/libganak.dir/statistics.cpp.o: src/CMakeFiles/libganak.dir/flags.make
src/CMakeFiles/libganak.dir/statistics.cpp.o: ../src/statistics.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dtai/Downloads/ganak-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object src/CMakeFiles/libganak.dir/statistics.cpp.o"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/libganak.dir/statistics.cpp.o -c /home/dtai/Downloads/ganak-master/src/statistics.cpp

src/CMakeFiles/libganak.dir/statistics.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/libganak.dir/statistics.cpp.i"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dtai/Downloads/ganak-master/src/statistics.cpp > CMakeFiles/libganak.dir/statistics.cpp.i

src/CMakeFiles/libganak.dir/statistics.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/libganak.dir/statistics.cpp.s"
	cd /home/dtai/Downloads/ganak-master/build/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dtai/Downloads/ganak-master/src/statistics.cpp -o CMakeFiles/libganak.dir/statistics.cpp.s

src/CMakeFiles/libganak.dir/statistics.cpp.o.requires:

.PHONY : src/CMakeFiles/libganak.dir/statistics.cpp.o.requires

src/CMakeFiles/libganak.dir/statistics.cpp.o.provides: src/CMakeFiles/libganak.dir/statistics.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/libganak.dir/build.make src/CMakeFiles/libganak.dir/statistics.cpp.o.provides.build
.PHONY : src/CMakeFiles/libganak.dir/statistics.cpp.o.provides

src/CMakeFiles/libganak.dir/statistics.cpp.o.provides.build: src/CMakeFiles/libganak.dir/statistics.cpp.o


# Object files for target libganak
libganak_OBJECTS = \
"CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o" \
"CMakeFiles/libganak.dir/component_analyzer.cpp.o" \
"CMakeFiles/libganak.dir/component_cache.cpp.o" \
"CMakeFiles/libganak.dir/component_management.cpp.o" \
"CMakeFiles/libganak.dir/instance.cpp.o" \
"CMakeFiles/libganak.dir/new_component_analyzer.cpp.o" \
"CMakeFiles/libganak.dir/solver.cpp.o" \
"CMakeFiles/libganak.dir/statistics.cpp.o"

# External object files for target libganak
libganak_EXTERNAL_OBJECTS =

src/liblibganak.a: src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o
src/liblibganak.a: src/CMakeFiles/libganak.dir/component_analyzer.cpp.o
src/liblibganak.a: src/CMakeFiles/libganak.dir/component_cache.cpp.o
src/liblibganak.a: src/CMakeFiles/libganak.dir/component_management.cpp.o
src/liblibganak.a: src/CMakeFiles/libganak.dir/instance.cpp.o
src/liblibganak.a: src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o
src/liblibganak.a: src/CMakeFiles/libganak.dir/solver.cpp.o
src/liblibganak.a: src/CMakeFiles/libganak.dir/statistics.cpp.o
src/liblibganak.a: src/CMakeFiles/libganak.dir/build.make
src/liblibganak.a: src/CMakeFiles/libganak.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dtai/Downloads/ganak-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Linking CXX static library liblibganak.a"
	cd /home/dtai/Downloads/ganak-master/build/src && $(CMAKE_COMMAND) -P CMakeFiles/libganak.dir/cmake_clean_target.cmake
	cd /home/dtai/Downloads/ganak-master/build/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/libganak.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/libganak.dir/build: src/liblibganak.a

.PHONY : src/CMakeFiles/libganak.dir/build

src/CMakeFiles/libganak.dir/requires: src/CMakeFiles/libganak.dir/alt_component_analyzer.cpp.o.requires
src/CMakeFiles/libganak.dir/requires: src/CMakeFiles/libganak.dir/component_analyzer.cpp.o.requires
src/CMakeFiles/libganak.dir/requires: src/CMakeFiles/libganak.dir/component_cache.cpp.o.requires
src/CMakeFiles/libganak.dir/requires: src/CMakeFiles/libganak.dir/component_management.cpp.o.requires
src/CMakeFiles/libganak.dir/requires: src/CMakeFiles/libganak.dir/instance.cpp.o.requires
src/CMakeFiles/libganak.dir/requires: src/CMakeFiles/libganak.dir/new_component_analyzer.cpp.o.requires
src/CMakeFiles/libganak.dir/requires: src/CMakeFiles/libganak.dir/solver.cpp.o.requires
src/CMakeFiles/libganak.dir/requires: src/CMakeFiles/libganak.dir/statistics.cpp.o.requires

.PHONY : src/CMakeFiles/libganak.dir/requires

src/CMakeFiles/libganak.dir/clean:
	cd /home/dtai/Downloads/ganak-master/build/src && $(CMAKE_COMMAND) -P CMakeFiles/libganak.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/libganak.dir/clean

src/CMakeFiles/libganak.dir/depend:
	cd /home/dtai/Downloads/ganak-master/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dtai/Downloads/ganak-master /home/dtai/Downloads/ganak-master/src /home/dtai/Downloads/ganak-master/build /home/dtai/Downloads/ganak-master/build/src /home/dtai/Downloads/ganak-master/build/src/CMakeFiles/libganak.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/libganak.dir/depend

