add_executable(tagger_node_test
    tagger_node_test.c
    ${CMAKE_SOURCE_DIR}/nodes/src/tagger_node.c
    ${CMAKE_SOURCE_DIR}/pipeline/node.c
)
target_include_directories(tagger_node_test PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/nodes/include
    ${CMAKE_SOURCE_DIR}/pipeline
    ${CMAKE_SOURCE_DIR}/queue
    ${CMAKE_SOURCE_DIR}/logger
)
target_link_libraries(tagger_node_test PRIVATE queue logger Threads::Threads)
add_test(NAME tagger_node_test COMMAND tagger_node_test)
