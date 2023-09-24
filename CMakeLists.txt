cmake_minimum_required(VERSION 3.25)  # CMake version check
project(vlan_tagger C)               # Create project "vlan_tagger"

set(CMAKE_C_STANDARD 23) # Enable c 23 standard

#set(SOURCE_FILES main.c pthread_init.c sniffer.c sender.c vlan_tagger.c queue/queue.c
#    logger/logger.c config_parser/config_parser.c)

# Add executable target with source files listed in SOURCE_FILES variable
#add_executable(vlan_tagger ${SOURCE_FILES})

add_subdirectory(logger)
add_subdirectory(config_parser)
add_subdirectory(queue)

set(SOURCE_FILES main.c pthread_init.c sniffer.c sender.c vlan_tagger.c)

# Создайте исполняемую цель и свяжите ее с библиотеками
add_executable(vlan_tagger ${SOURCE_FILES})

# Свяжите библиотеки с исполняемой целью
target_link_libraries(vlan_tagger PRIVATE logger config_parser queue)