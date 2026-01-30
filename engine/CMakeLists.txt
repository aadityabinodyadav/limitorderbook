cmake_minimum_required(VERSION 3.15)
project(TradingEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/order_book.cpp
    src/http_server.cpp
)

add_executable(engine ${SOURCES})
target_link_libraries(engine pthread)

if(WIN32)
    # On Windows/MinGW, we need ws2_32 for networking
    target_link_libraries(engine ws2_32 pthread)
else()
    # On Linux/macOS, we just need pthread
    target_link_libraries(engine pthread)
endif()