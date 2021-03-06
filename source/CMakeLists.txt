
#set(CMAKE_INCLUDE_CURRENT_DIR on)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${PROTOBUF_INCLUDE_DIRS})

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/net SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/util SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/framework SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/proto SRC_LIST)


add_library(storm ${SRC_LIST})

install(TARGETS storm DESTINATION lib)

file(GLOB headers1 *.h)
file(GLOB headers2 net/*.h)
file(GLOB headers3 util/*.h)
file(GLOB headers4 framework/*.h)

install(FILES ${headers1} DESTINATION include)
install(FILES ${headers2} DESTINATION include/net)
install(FILES ${headers3} DESTINATION include/util)
install(FILES ${headers4} DESTINATION include/framework)
install(FILES ${headers5} DESTINATION include/proto)

add_subdirectory(rpc_generator)
add_subdirectory(storm_servers/storm_registry)
