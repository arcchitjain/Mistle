# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.18.4/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.18.4/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build

# Include any dependencies generated for this target.
include croupier/CMakeFiles/Croupier.dir/depend.make

# Include the progress variables for this target.
include croupier/CMakeFiles/Croupier.dir/progress.make

# Include the compile flags for this target's objects.
include croupier/CMakeFiles/Croupier.dir/flags.make

croupier/CMakeFiles/Croupier.dir/Croupier.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/Croupier.cpp.o: ../croupier/Croupier.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object croupier/CMakeFiles/Croupier.dir/Croupier.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/Croupier.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/Croupier.cpp

croupier/CMakeFiles/Croupier.dir/Croupier.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/Croupier.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/Croupier.cpp > CMakeFiles/Croupier.dir/Croupier.cpp.i

croupier/CMakeFiles/Croupier.dir/Croupier.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/Croupier.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/Croupier.cpp -o CMakeFiles/Croupier.dir/Croupier.cpp.s

croupier/CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.o: ../croupier/afopt/AFOPTMine.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object croupier/CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/AFOPTMine.cpp

croupier/CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/AFOPTMine.cpp > CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.i

croupier/CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/AFOPTMine.cpp -o CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.s

croupier/CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.o: ../croupier/afopt/AfoptCroupier.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object croupier/CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/AfoptCroupier.cpp

croupier/CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/AfoptCroupier.cpp > CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.i

croupier/CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/AfoptCroupier.cpp -o CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.s

croupier/CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.o: ../croupier/afopt/AfoptMiner.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object croupier/CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/AfoptMiner.cpp

croupier/CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/AfoptMiner.cpp > CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.i

croupier/CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/AfoptMiner.cpp -o CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.s

croupier/CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.o: ../croupier/afopt/ArrayMine.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object croupier/CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/ArrayMine.cpp

croupier/CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/ArrayMine.cpp > CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.i

croupier/CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/ArrayMine.cpp -o CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.s

croupier/CMakeFiles/Croupier.dir/afopt/Global.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/afopt/Global.cpp.o: ../croupier/afopt/Global.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object croupier/CMakeFiles/Croupier.dir/afopt/Global.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/afopt/Global.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/Global.cpp

croupier/CMakeFiles/Croupier.dir/afopt/Global.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/afopt/Global.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/Global.cpp > CMakeFiles/Croupier.dir/afopt/Global.cpp.i

croupier/CMakeFiles/Croupier.dir/afopt/Global.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/afopt/Global.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/Global.cpp -o CMakeFiles/Croupier.dir/afopt/Global.cpp.s

croupier/CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.o: ../croupier/afopt/ScanDBMine.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object croupier/CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/ScanDBMine.cpp

croupier/CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/ScanDBMine.cpp > CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.i

croupier/CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/ScanDBMine.cpp -o CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.s

croupier/CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.o: ../croupier/afopt/closed/cfptree_outbuf.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object croupier/CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/closed/cfptree_outbuf.cpp

croupier/CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/closed/cfptree_outbuf.cpp > CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.i

croupier/CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/closed/cfptree_outbuf.cpp -o CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.s

croupier/CMakeFiles/Croupier.dir/afopt/data.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/afopt/data.cpp.o: ../croupier/afopt/data.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building CXX object croupier/CMakeFiles/Croupier.dir/afopt/data.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/afopt/data.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/data.cpp

croupier/CMakeFiles/Croupier.dir/afopt/data.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/afopt/data.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/data.cpp > CMakeFiles/Croupier.dir/afopt/data.cpp.i

croupier/CMakeFiles/Croupier.dir/afopt/data.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/afopt/data.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/data.cpp -o CMakeFiles/Croupier.dir/afopt/data.cpp.s

croupier/CMakeFiles/Croupier.dir/afopt/fsout.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/afopt/fsout.cpp.o: ../croupier/afopt/fsout.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building CXX object croupier/CMakeFiles/Croupier.dir/afopt/fsout.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/afopt/fsout.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/fsout.cpp

croupier/CMakeFiles/Croupier.dir/afopt/fsout.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/afopt/fsout.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/fsout.cpp > CMakeFiles/Croupier.dir/afopt/fsout.cpp.i

croupier/CMakeFiles/Croupier.dir/afopt/fsout.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/afopt/fsout.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/fsout.cpp -o CMakeFiles/Croupier.dir/afopt/fsout.cpp.s

croupier/CMakeFiles/Croupier.dir/afopt/hash_map.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/afopt/hash_map.cpp.o: ../croupier/afopt/hash_map.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building CXX object croupier/CMakeFiles/Croupier.dir/afopt/hash_map.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/afopt/hash_map.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/hash_map.cpp

croupier/CMakeFiles/Croupier.dir/afopt/hash_map.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/afopt/hash_map.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/hash_map.cpp > CMakeFiles/Croupier.dir/afopt/hash_map.cpp.i

croupier/CMakeFiles/Croupier.dir/afopt/hash_map.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/afopt/hash_map.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/hash_map.cpp -o CMakeFiles/Croupier.dir/afopt/hash_map.cpp.s

croupier/CMakeFiles/Croupier.dir/afopt/parameters.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/afopt/parameters.cpp.o: ../croupier/afopt/parameters.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building CXX object croupier/CMakeFiles/Croupier.dir/afopt/parameters.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/afopt/parameters.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/parameters.cpp

croupier/CMakeFiles/Croupier.dir/afopt/parameters.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/afopt/parameters.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/parameters.cpp > CMakeFiles/Croupier.dir/afopt/parameters.cpp.i

croupier/CMakeFiles/Croupier.dir/afopt/parameters.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/afopt/parameters.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/afopt/parameters.cpp -o CMakeFiles/Croupier.dir/afopt/parameters.cpp.s

croupier/CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.o: ../croupier/apriori/AprioriCroupier.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building CXX object croupier/CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriCroupier.cpp

croupier/CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriCroupier.cpp > CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.i

croupier/CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriCroupier.cpp -o CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.s

croupier/CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.o: ../croupier/apriori/AprioriMiner.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Building CXX object croupier/CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriMiner.cpp

croupier/CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriMiner.cpp > CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.i

croupier/CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriMiner.cpp -o CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.s

croupier/CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.o: ../croupier/apriori/AprioriNode.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_15) "Building CXX object croupier/CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriNode.cpp

croupier/CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriNode.cpp > CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.i

croupier/CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriNode.cpp -o CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.s

croupier/CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.o: croupier/CMakeFiles/Croupier.dir/flags.make
croupier/CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.o: ../croupier/apriori/AprioriTree.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_16) "Building CXX object croupier/CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.o"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.o -c /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriTree.cpp

croupier/CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.i"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriTree.cpp > CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.i

croupier/CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.s"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && /usr/local/Cellar/gcc/9.3.0/bin/g++-9 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier/apriori/AprioriTree.cpp -o CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.s

# Object files for target Croupier
Croupier_OBJECTS = \
"CMakeFiles/Croupier.dir/Croupier.cpp.o" \
"CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.o" \
"CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.o" \
"CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.o" \
"CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.o" \
"CMakeFiles/Croupier.dir/afopt/Global.cpp.o" \
"CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.o" \
"CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.o" \
"CMakeFiles/Croupier.dir/afopt/data.cpp.o" \
"CMakeFiles/Croupier.dir/afopt/fsout.cpp.o" \
"CMakeFiles/Croupier.dir/afopt/hash_map.cpp.o" \
"CMakeFiles/Croupier.dir/afopt/parameters.cpp.o" \
"CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.o" \
"CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.o" \
"CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.o" \
"CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.o"

# External object files for target Croupier
Croupier_EXTERNAL_OBJECTS =

croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/Croupier.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/afopt/AFOPTMine.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/afopt/AfoptCroupier.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/afopt/AfoptMiner.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/afopt/ArrayMine.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/afopt/Global.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/afopt/ScanDBMine.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/afopt/closed/cfptree_outbuf.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/afopt/data.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/afopt/fsout.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/afopt/hash_map.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/afopt/parameters.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/apriori/AprioriCroupier.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/apriori/AprioriMiner.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/apriori/AprioriNode.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/apriori/AprioriTree.cpp.o
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/build.make
croupier/libCroupier.a: croupier/CMakeFiles/Croupier.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_17) "Linking CXX static library libCroupier.a"
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && $(CMAKE_COMMAND) -P CMakeFiles/Croupier.dir/cmake_clean_target.cmake
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Croupier.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
croupier/CMakeFiles/Croupier.dir/build: croupier/libCroupier.a

.PHONY : croupier/CMakeFiles/Croupier.dir/build

croupier/CMakeFiles/Croupier.dir/clean:
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier && $(CMAKE_COMMAND) -P CMakeFiles/Croupier.dir/cmake_clean.cmake
.PHONY : croupier/CMakeFiles/Croupier.dir/clean

croupier/CMakeFiles/Croupier.dir/depend:
	cd /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/croupier /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier /Users/arcchit/Docs/Mistle/Resources/Krimp/trunk/build/croupier/CMakeFiles/Croupier.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : croupier/CMakeFiles/Croupier.dir/depend
