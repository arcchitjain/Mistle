# File manipulation command. GLOB_RECURSE will generate a list similar to the regular GLOB, except it will traverse all the subdirectories of the matched directory and match the files
file(GLOB_RECURSE LIBCROUPIER_SRCS *.cpp)

# Add include directories to the build.
include_directories(../bass)
include_directories(../qtils)

# Add "Croupier" library to the project using the specified source files.
add_library (Croupier ${LIBCROUPIER_SRCS})

# Specify rules to run at install time.
#install (TARGETS Croupier LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
