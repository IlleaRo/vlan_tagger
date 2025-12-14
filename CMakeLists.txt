cmake_minimum_required(VERSION 3.25)  # CMake version check
project(vlan_tagger C)               # Create project "vlan_tagger"

set(CMAKE_C_STANDARD 17)

set(DEBUG_EN ON)

#static library folders
add_subdirectory(logger)
add_subdirectory(config_parser)
add_subdirectory(queue)

set(SOURCE_FILES
        main.c
        pipeline/node.c
        pipeline/pipeline.c
        pipeline_patterns/patterns.c
        nodes/src/sniffer_node.c
        nodes/src/tagger_node.c
        nodes/src/sender_node.c
        nodes/src/ip_modifier_node.c
        nodes/src/packet_counter_node.c
        nodes/src/logger_node.c
        nodes/src/collector_node.c
        nodes/src/dispatcher_node.c
        nodes/src/socket_utils.c
)

add_executable(vlan_tagger ${SOURCE_FILES})
if(DEBUG_EN)
    target_compile_definitions(vlan_tagger PUBLIC DEBUG)
endif()

target_link_libraries(vlan_tagger PRIVATE logger config_parser queue)
