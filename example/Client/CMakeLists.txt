set(APP Client)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/source)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../protocol)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../protocol SRC_LIST)

add_executable(${APP} ${SRC_LIST})

target_link_libraries(${APP} pthread protobuf storm)
