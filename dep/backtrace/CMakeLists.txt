
find_package(Backtrace REQUIRED)
add_library(backtrace SHARED IMPORTED GLOBAL)
set_target_properties(backtrace
        PROPERTIES
        IMPORTED_LOCATION
        "${Backtrace_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORI
        "${Backtrace_INCLUDE_DIR}")
