include(LibFindMacros)

find_path(Mad_INCLUDE_DIR NAMES mad.h PATHS c:\\mingw\\include )
find_library(Mad_LIBRARY NAMES mad PATHS c:\\mingw\\lib )

set(Mad_PROCESS_INCLUDES Mad_INCLUDE_DIR)
set(Mad_PROCESS_LIBS Mad_LIBRARY)

libfind_process(Mad)
