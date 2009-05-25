include(LibFindMacros)

find_path(Mcrypt_INCLUDE_DIR NAMES mcrypt.h PATHS c:\\mingw\\include )
find_library(Mcrypt_LIBRARY NAMES mcrypt PATHS c:\\mingw\\lib )

set(Mcrypt_PROCESS_INCLUDES Mcrypt_INCLUDE_DIR)
set(Mcrypt_PROCESS_LIBS Mcrypt_LIBRARY)

libfind_process(Mcrypt)
