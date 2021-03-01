# File manipulation command. GLOB_RECURSE will generate a list similar to the regular GLOB, except it will traverse all the subdirectories of the matched directory and match the files
file(GLOB_RECURSE LIBQTILS_SRCS *.cpp)

# Add "Qtils" library to the project using the specified source files.
add_library (Qtils ${LIBQTILS_SRCS})

# Specify rules to run at install time.
#install (TARGETS Qtils LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
