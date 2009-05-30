include(LibFindMacros)

find_path(Sox_INCLUDE_DIR NAMES sox.h PATHS c:\\mingw\\include )
find_library(Sox_LIBRARY NAMES sox PATHS c:\\mingw\\lib )

set(Sox_PROCESS_INCLUDES Sox_INCLUDE_DIR)
set(Sox_PROCESS_LIBS Sox_LIBRARY)

libfind_process(Sox)
