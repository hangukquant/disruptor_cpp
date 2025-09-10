A lightweight, header-only C++ port of the LMAX Disruptor (v3) pattern for high-performance, lock-free event processing. Project targets an efficient, canonical port of the LMAX Java implementation for SPSC/MPMC bounded 'queues' for low latency data-sharing between threads.

## Features
- Header-only, C++20
- Lock-free ring buffer
- Pluggable wait strategies (both producer and consumer/processor)
- Sequence barriers for complex dependency graphs (diamond, pipeline, etc.)
- Consumer-producer architecture.
- Simple, matching APIs.

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
- support DSL
- support more wait strategies
- support more thread management options