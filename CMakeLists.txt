cmake_minimum_required(VERSION 3.16)
project(PendelClient)

set(CMAKE_CXX_STANDARD 17)

add_executable(PendelClient
        main.cpp
        PendelClient.cpp
)

target_link_libraries(PendelClient ws2_32)
