include(LibFindMacros)
libfind_pkg_check_modules(Glib_PKGCONF glib-2.0)

find_path(Glib_INCLUDE_DIR NAMES glib.h HINTS ${Glib_PKGCONF_INCLUDE_DIRS})
find_path(Glib_Config_INCLUDE_DIR NAMES glibconfig.h HINTS ${Glib_PKGCONF_INCLUDE_DIRS})
find_library(Glib_LIBRARY NAMES glib-2.0 HINTS ${Glib_PKGCONF_LIBRARY_DIRS})

set(Glib_PROCESS_INCLUDES Glib_INCLUDE_DIR Glib_Config_INCLUDE_DIR)
set(Glib_PROCESS_LIBS Glib_LIBRARY)

libfind_process(Glib)
