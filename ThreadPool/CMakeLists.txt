file(GLOB srcs *.cpp)

add_library(ThreadPool SHARED ${srcs})
find_package(Threads REQUIRED)
target_include_directories(ThreadPool PUBLIC .)