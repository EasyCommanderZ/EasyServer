cmake_minimum_required(VERSION 3.12)

project(EasyServer VERSION 0.1 DESCRIPTION "Easy Web Server" LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 在linux上指定 g++ 为编译器，通过设置编译选项解决一些编译问题，并忽略一些编译警告
if(NOT APPLE)
    set(CMAKE_CXX_COMPILER "g++")
    set(CXX_FLAGS
    -g
    -Wall
    -fPIC
    -std=c++17
    -pthread
    -Wno-unused-parameter
    -Wno-varargs
    -Wno-reorder
    )
else()
    set(CXX_FLAGS
    -Wno-unused-parameter
    -Wno-varargs
    -Wno-reorder
    )
endif()

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

# CMAKE 配置参考了很多 ZLToolKit 的写法 

#设置库文件路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
#设置可执行程序路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
#设置子目录
# Thread 的顺序要放在最后 不然会导致错误 原因暂时未知 主要问题是 ThreadInfo 这里出现 redifinition， 和链接的顺序可能有一定关系
set(SUB_DIR_LIST "Reactor" "Server" "Util" "Http" "Log" "Thread")
foreach(SUB_DIR ${SUB_DIR_LIST})
    # 遍历源文件
    aux_source_directory(src/${SUB_DIR} SRC_LIST)
    # 安装头文件至系统目录
    # install(DIRECTORY src/${SUB_DIR} DESTINATION ${INSTALL_PATH_INCLUDE} FILES_MATCHING PATTERN "*.h")
endforeach()


set(LINK_LIB_LIST)
find_package(Threads REQUIRED)
# set(ENABLE_OPENSSL ON CACHE BOOL "enable openssl")
# set(ENABLE_MYSQL ON CACHE BOOL "enable mysql")


#查找openssl是否安装
# find_package(OpenSSL QUIET)
# if(OPENSSL_FOUND AND ENABLE_OPENSSL)
#     message(STATUS "找到openssl库:\"${OPENSSL_INCLUDE_DIR}\",ENABLE_OPENSSL宏已打开")
#     include_directories(${OPENSSL_INCLUDE_DIR})
#     add_definitions(-DENABLE_OPENSSL)
#     list(APPEND  LINK_LIB_LIST ${OPENSSL_LIBRARIES})
# endif()

#查找mysql是否安装
# find_package(MYSQL QUIET)
# if(MYSQL_FOUND AND ENABLE_MYSQL)
#     message(STATUS "找到mysqlclient库:\"${MYSQL_INCLUDE_DIR}\",ENABLE_MYSQL宏已打开")
#     include_directories(${MYSQL_INCLUDE_DIR})
#     include_directories(${MYSQL_INCLUDE_DIR}/mysql)
#     add_definitions(-DENABLE_MYSQL)
#     list(APPEND  LINK_LIB_LIST ${MYSQL_LIBRARIES})
# endif()

# 设置子模块的安装目录
# set(INSTALL_PATH_LIB lib)
# set(INSTALL_PATH_INCLUDE include)

#添加子模块
# foreach(SUB_DIR ${SUB_DIR_LIST})
#     aux_source_directory(${SUB_DIR} SRC_LIST)
#     #安装头文件至系统目录
#     install(DIRECTORY src/${SUB_DIR} DESTINATION ${INSTALL_PATH_INCLUDE} FILES_MATCHING PATTERN "*.h")
# endforeach()

message(STATUS "Linking dependencies : ${LINK_LIB_LIST}")

# 编译动态库
add_library(${PROJECT_NAME}_shared SHARED ${SRC_LIST})
target_include_directories(${PROJECT_NAME}_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME}_shared)
# set_target_properties(${PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
# install(TARGETS ${PROJECT_NAME}_shared  ARCHIVE DESTINATION ${INSTALL_PATH_LIB} LIBRARY DESTINATION ${INSTALL_PATH_LIB})

#编译静态库
add_library(${PROJECT_NAME}_static STATIC ${SRC_LIST})
#引用头文件路径
target_include_directories(${PROJECT_NAME}_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# set_target_properties(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
# #安装静态库至系统目录
# install(TARGETS ${PROJECT_NAME}_static ARCHIVE DESTINATION ${INSTALL_PATH_LIB})

# 测试程序
include(CTest)
enable_testing()
add_subdirectory(tests)

file(GLOB sources main.cpp)
add_executable(main ${sources})
target_link_libraries(main PUBLIC ${PROJECT_NAME}_shared)