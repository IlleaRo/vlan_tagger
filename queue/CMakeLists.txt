option(ENABLE_DEBUG "Enable debug features" OFF)

if(ENABLE_DEBUG)
    add_compile_definitions(DEBUG)
endif()

add_library(queue STATIC queue.c queue.h)