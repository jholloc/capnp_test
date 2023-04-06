cmake_minimum_required( VERSION 3.25 )
project( capnp_test )

set( CMAKE_CXX_STANDARD 17 )

find_package( CapnProto REQUIRED )

capnp_generate_cpp( CAPNP_SRCS CANPNP_HDRS schema.capnp )

add_executable( capnp_test main.cpp ${CAPNP_SRCS} )
target_include_directories( capnp_test PUBLIC ${CMAKE_CURRENT_BINARY_DIR} )
target_link_libraries( capnp_test PUBLIC CapnProto::capnp )