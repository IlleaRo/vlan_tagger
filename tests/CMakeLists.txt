add_subdirectory(node_tests)

add_executable(queue_test queue_test.c)
target_include_directories(queue_test PRIVATE ${CMAKE_SOURCE_DIR}/queue)
target_link_libraries(queue_test PRIVATE queue Threads::Threads)
target_compile_definitions(queue_test PRIVATE DEBUG)
add_test(NAME queue_test COMMAND queue_test)

add_executable(config_parser_test config_parser_test.c)
target_include_directories(config_parser_test PRIVATE ${CMAKE_SOURCE_DIR}/config_parser)
target_link_libraries(config_parser_test PRIVATE config_parser)
add_test(NAME config_parser_test COMMAND config_parser_test)

add_executable(logger_test logger_test.c)
target_include_directories(logger_test PRIVATE ${CMAKE_SOURCE_DIR}/logger)
target_link_libraries(logger_test PRIVATE logger Threads::Threads)
add_test(NAME logger_test COMMAND logger_test)
