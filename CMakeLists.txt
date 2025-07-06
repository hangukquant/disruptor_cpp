cmake_minimum_required(VERSION 3.10)
project(disruptor_cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(disruptor_cpp src/main.cpp)

target_include_directories(disruptor_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(
    DISRUPTOR_HEADERS
    src/disruptor/ring_buffer.h
    src/disruptor/sequence.h
    src/disruptor/sequencer.h
    src/disruptor/wait_strategies.h
    src/disruptor/sequence_barrier.h
    src/disruptor/event_handler.h
    src/disruptor/event_processor.h
)
target_sources(disruptor_cpp PRIVATE ${DISRUPTOR_HEADERS})
