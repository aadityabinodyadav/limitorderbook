cmake_minimum_required(VERSION 3.15)
project(TradingEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/order_book.cpp
)

add_executable(engine ${SOURCES})
target_link_libraries(engine pthread)