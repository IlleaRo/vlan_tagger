cmake_minimum_required(VERSION 3.25)  # CMake version check
project(vlan_tagger C)               # Create project "vlan_tagger"

set(CMAKE_C_STANDARD 23) # Enable c 23 standard

#static library folders
add_subdirectory(logger)
add_subdirectory(config_parser)
add_subdirectory(queue)

set(SOURCE_FILES
    main.c
    pipeline/node.c
    pipeline/pipeline.c
    nodes/sniffer_node.c
    nodes/tagger_node.c
    nodes/sender_node.c
    nodes/ip_modifier_node.c
    nodes/packet_counter_node.c
)

add_executable(vlan_tagger ${SOURCE_FILES})
target_link_libraries(vlan_tagger PRIVATE logger config_parser queue)