# Fic or Krimp?
The source tree uses 'fic' as the main name, as 'fic' encompasses much more than just Krimp. The belowmentioned compilation strategies hence resp. deliver 'fic(.exe)' which you yourself may decided to rename to 'krimp(.exe)'.

# Compilation instruction GNU/Linux, Mac OS X, ...
# - resulting executable can be found in the /bin/ directory
./bootstrap.sh
make -Cbuild install

# Compilation on Windows, using Visual C++ 2010 or 2008
# - simply open the correct solution (.sln) file, and compile the desired target (Release or Release64)
# - resulting executable can be found in the /src/ directory

