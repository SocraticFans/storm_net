set(APP rpc2cpp)

ADD_CUSTOM_COMMAND(
	OUTPUT lex.yy.hpp lex.yy.cpp
    COMMAND flex -o lex.yy.cpp --header-file=lex.yy.hpp ${CMAKE_CURRENT_SOURCE_DIR}/lexical.l
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lexical.l
)

ADD_CUSTOM_COMMAND(
    OUTPUT syntax.tab.hpp syntax.tab.cpp
    COMMAND bison -d -o syntax.tab.cpp ${CMAKE_CURRENT_SOURCE_DIR}/syntax.y
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/syntax.y
)


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/source)

ADD_EXECUTABLE(${APP} lex.yy.hpp lex.yy.cpp syntax.tab.hpp syntax.tab.cpp parser.cpp)

target_link_libraries(${APP} pthread storm)

install(TARGETS ${APP} DESTINATION bin)

