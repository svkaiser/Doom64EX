# - Find fmt (fmtlib.net)
# Find the native fmt includes and library
#
#  Fmt_INCLUDE_DIR - where to find fmt directory
#  Fmt_LIBRARY   - List of libraries when using fmt
#  Fmt_FOUND       - True if fmt was found.

if(Fmt_INCLUDE_DIR AND Fmt_LIBRARY)
    # Already in cache, be silent
    set(Fmt_FIND_QUIETLY TRUE)
endif(Fmt_INCLUDE_DIR AND Fmt_LIBRARY)

find_path(Fmt_INCLUDE_DIR fmt/format.h)

find_library(Fmt_LIBRARY NAMES fmt)
mark_as_advanced(Fmt_LIBRARY Fmt_INCLUDE_DIR )

# handle the QUIETLY and REQUIRED arguments and set Fmt_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Fmt DEFAULT_MSG Fmt_LIBRARY Fmt_INCLUDE_DIR)
