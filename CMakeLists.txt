cmake_minimum_required(VERSION 3.10)
project(main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(main src/main.cpp)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(
    DISRUPTOR_HEADERS
    src/ring_buffer.h
    src/sequence.h
    src/sequencer.h
    src/wait_strategies.h
    src/sequence_barrier.h
    src/event_handler.h
    src/event_processor.h
)
target_sources(main PRIVATE ${DISRUPTOR_HEADERS})
