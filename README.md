# disruptor_cpp

A lightweight, header-only C++ port of the LMAX Disruptor (v3) pattern for high-performance, lock-free event processing. Project targets an efficient, canonical port of the LMAX Java implementation for SPSC/MPMC bounded 'queues' for low latency data-sharing between threads.

## Features
- Header-only, C++20
- Lock-free ring buffer
- Pluggable wait strategies (both producer and consumer/processor)
- Sequence barriers for complex dependency graphs (diamond, pipeline, etc.)
- Consumer-producer architecture.
- Simple, matching APIs.
- Example code for SPSC and diamond dependency patterns under `src/main.cpp`.

## Build & Installation

### Prerequisites
- C++20 compatible compiler
- CMake 3.10+

### Build
```sh
# Build using CMake
$ ./build.sh
```
This will build the `disruptor_cpp` executable in the `build/` directory.

## Running Examples

After building, run the main example:
```sh
$ ./build/disruptor_cpp
```
This will execute both the simple SPSC and the diamond dependency examples, printing event flow and timing to the console. Example output: C <- {A,B} in our diamond configuration.

```
===== Running Simple Example =====
[Simple] Started.
[           0 ns] [Simple] Sequence 0 Value 0
[    54993334 ns] [Simple] Sequence 1 Value 1
[   110004375 ns] [Simple] Sequence 2 Value 2
[   160841167 ns] [Simple] Sequence 3 Value 3
[   215857500 ns] [Simple] Sequence 4 Value 4
[Simple] Shutdown.

===== Running Diamond Example =====
[  1271055667 ns] [A] Sequence 0 Value 0
[  1271056500 ns] [B] Sequence 0 Value 0
[  1271102292 ns] [C] Sequence 0 Value 0
[  1326052084 ns] [A] Sequence 1 Value 1
[  1326053417 ns] [B] Sequence 1 Value 1
[  1326077667 ns] [C] Sequence 1 Value 1
[  1381066875 ns] [B] Sequence 2 Value 2
[  1381066959 ns] [A] Sequence 2 Value 2
[  1381087625 ns] [C] Sequence 2 Value 2
[  1436082292 ns] [A] Sequence 3 Value 3
[  1436082625 ns] [B] Sequence 3 Value 3
[  1436105084 ns] [C] Sequence 3 Value 3
[  1486719084 ns] [B] Sequence 4 Value 4
[  1486719292 ns] [A] Sequence 4 Value 4
[  1486742125 ns] [C] Sequence 4 Value 4
```

## Architectural Diagram

```
                         ┌────────────────────┐
                         │  Application /     │
                         │  Driver (Producer) │
                         └────────┬───────────┘
                                  │
                                  │  creates
                                  ▼
                    ┌──────────────────────────────┐
                    │  [Single]ProducerSequencer   │◄────────┐
                    │  (owns cursor_, tracks claim)│         │
                    └────────────┬─────────────────┘         │
                                 │                           │
        passed to ctor           │                           │
                                 ▼                           │
                    ┌──────────────────────────────┐         │
                    │         RingBuffer           │         │
                    │ [owns T[N], holds &Sequencer]│         │
                    └────────────┬─────────────────┘         │
                                 │                           │
      Application calls          ▼                           │
     ┌────────────────────────────────────────────────────┐  │
     │ next()        ────────────────────────────────────►│  │
     │ get(seq)      ────────────────────────────────────►│  │
     │ publish(seq)  ────────────────────────────────────►│  │
     └────────────────────────────────────────────────────┘  │
                                                             │
               ┌─────────────────────────────────────────────┘
               ▼
 ┌────────────────────────────────────────────────────────────┐
 │        SequenceBarrier (built from Sequencer)              │
 │ - wraps wait strategy & dependent sequences                │
 └────────────┬───────────────────────────────────────────────┘
              │
              │
              ▼
      ┌─────────────────────────────┐     owns     ┌──────────────┐
      │       EventProcessor        │─────────────►│  Sequence    │
      │  (consumes from buffer)     │              │ (consumer)   │
      └────────────┬────────────────┘              └─────┬────────┘
                   │                                     │
                   ▼                                     │
         calls barrier.waitFor(seq)                      │
         gets events via buffer.get(seq)                 │
         passes to eventHandler.onEvent(...)             │
                   │                                     │
                   ▼                                     │
      ┌───────────────────────────────┐                  │
      │        EventHandler           │◄─────────────────┘
      │  - onEvent(event, seq, eob)   │
      └───────────────────────────────┘
```

### PRs:
There is a number of low hanging fruits we are working on/would gladly accept 
PRs for. They should be fairly trivial extensions matching the Java implementations.

- add `MultiProducerSequencer`
- add microbenchmarks, tests
- support DSL
- support more wait strategies
- support exception handling strategies
- support more thread management options

## License

MIT License. See [LICENSE](LICENSE) for details.