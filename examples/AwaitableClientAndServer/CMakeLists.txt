# CMakeLists.txt
# David Brill
#
# Copyright (c) 2023
# Distributed under the Apache License 2.0 (see accompanying
# file LICENSE or copy at http://www.apache.org/licenses/)

cmake_minimum_required(VERSION 3.24)

#lib requires c++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
add_compile_options(-g -O0 -Wall)

set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)

project(AwaitableClientAndServer)

find_package(OpenSSL REQUIRED)

add_executable(AwaitableClientAndServer)

target_include_directories(AwaitableClientAndServer 
    PUBLIC
    ../../include
    ../../../boost_1_81_0
)

target_sources(AwaitableClientAndServer
    PUBLIC
    main.cpp
    TcpExample.cpp
    UdpExample.cpp
    SslExample.cpp
    HttpExample.cpp
    HttpsExample.cpp
)

target_link_libraries(AwaitableClientAndServer
    OpenSSL::Crypto
    OpenSSL::SSL
)