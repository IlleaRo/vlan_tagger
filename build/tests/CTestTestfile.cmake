# CMake generated Testfile for 
# Source directory: /home/illearo/repo/cmn/vlan_tagger/tests
# Build directory: /home/illearo/repo/cmn/vlan_tagger/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[queue_test]=] "/home/illearo/repo/cmn/vlan_tagger/build/tests/queue_test")
set_tests_properties([=[queue_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/illearo/repo/cmn/vlan_tagger/tests/CMakeLists.txt;7;add_test;/home/illearo/repo/cmn/vlan_tagger/tests/CMakeLists.txt;0;")
add_test([=[config_parser_test]=] "/home/illearo/repo/cmn/vlan_tagger/build/tests/config_parser_test")
set_tests_properties([=[config_parser_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/illearo/repo/cmn/vlan_tagger/tests/CMakeLists.txt;12;add_test;/home/illearo/repo/cmn/vlan_tagger/tests/CMakeLists.txt;0;")
add_test([=[logger_test]=] "/home/illearo/repo/cmn/vlan_tagger/build/tests/logger_test")
set_tests_properties([=[logger_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/illearo/repo/cmn/vlan_tagger/tests/CMakeLists.txt;17;add_test;/home/illearo/repo/cmn/vlan_tagger/tests/CMakeLists.txt;0;")
subdirs("node_tests")
