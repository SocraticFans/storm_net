set(APP Registry)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/source)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST_REGISTRY)

add_executable(${APP} ${SRC_LIST_REGISTRY})

target_link_libraries(${APP} pthread protobuf storm dl rt /usr/lib/libmysqlclient.a)

install(TARGETS ${APP} RUNTIME DESTINATION bin)
