add_library(queue STATIC queue.c queue.h)
target_link_libraries(queue PUBLIC Threads::Threads)
