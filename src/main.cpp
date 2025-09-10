#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <string>

#include "disruptor/ring_buffer.h"
#include "disruptor/event_handler.h"
#include "disruptor/event_processor.h"
#include "disruptor/sequencer.h"
#include "disruptor/wait_strategies.h"
#include "disruptor/exception_handler.h"

using namespace disruptor;

inline int64_t now_ns()
{
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return duration_cast<nanoseconds>(steady_clock::now() - start).count();
}

inline void log(const std::string &tag, int64_t seq, int64_t val)
{
    int64_t t_ns = now_ns();
    printf("[%12lld ns] [%s] Sequence %lld Value %lld\n", t_ns, tag.c_str(), seq, val);
}

// ================================================
// Common Event and Factory
// ================================================

struct MyEvent
{
    int64_t value;
};

auto myEventFactory = []() -> MyEvent
{
    return MyEvent{0};
};

// ================================================
// Simple Handler
// ================================================

class SimpleHandler : public EventHandler<MyEvent>
{
public:
    void onEvent(MyEvent &event, int64_t sequence, bool endOfBatch) override
    {
        log("Simple", sequence, event.value);
    }
    void onStart() override { std::cout << "[Simple] Started.\n"; }
    void onShutdown() override { std::cout << "[Simple] Shutdown.\n"; }
};

// ================================================
// Diamond Handlers
// ================================================

class HandlerA : public EventHandler<MyEvent>
{
public:
    void onEvent(MyEvent &event, int64_t sequence, bool) override
    {
        log("A", sequence, event.value);
    }
};

class HandlerB : public EventHandler<MyEvent>
{
public:
    void onEvent(MyEvent &event, int64_t sequence, bool) override
    {
        log("B", sequence, event.value);
    }
};

class HandlerC : public EventHandler<MyEvent>
{
public:
    void onEvent(MyEvent &event, int64_t sequence, bool) override
    {
        log("C", sequence, event.value);
    }
};

// ================================================
// Simple Example
// ================================================

void simple()
{
    std::cout << "\n===== Running Simple Example =====\n";
    constexpr size_t bufferSize = 1024;
    BusySpinWaitStrategy waitStrategy;
    SingleProducerSequencer<bufferSize, BusySpinWaitStrategy> sequencer(waitStrategy);

    RingBuffer<MyEvent, bufferSize, decltype(sequencer), decltype(myEventFactory)>
        ringBuffer(sequencer, myEventFactory);

    auto barrier = sequencer.newBarrier({});
    SimpleHandler handler;

    DefaultExceptionHandler<MyEvent> exHandler;
    EventProcessor<MyEvent, decltype(ringBuffer), decltype(barrier), SimpleHandler>
        processor(ringBuffer, barrier, handler, exHandler);

    Sequence &consumerSeq = processor.getSequence();
    ringBuffer.setGatingSequences({&consumerSeq});

    std::thread consumer([&]
                         { processor.run(); });

    for (int i = 0; i < 5; ++i)
    {
        int64_t seq = ringBuffer.next();
        ringBuffer.get(seq).value = i;
        ringBuffer.publish(seq);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    processor.halt();
    consumer.join();
}

// ================================================
// Diamond Example
// ================================================

void diamond()
{
    std::cout << "\n===== Running Diamond Example =====\n";

    constexpr size_t bufferSize = 1024;
    BusySpinWaitStrategy waitStrategy;
    SingleProducerSequencer<bufferSize, BusySpinWaitStrategy> sequencer(waitStrategy);

    RingBuffer<MyEvent, bufferSize, decltype(sequencer), decltype(myEventFactory)>
        ringBuffer(sequencer, myEventFactory);

    // Create handlers
    HandlerA handlerA;
    HandlerB handlerB;
    HandlerC handlerC;

    // Barrier for A and B with no dependents
    auto barrierA = sequencer.newBarrier({});
    auto barrierB = sequencer.newBarrier({});

    // Create processors for A and B
    DefaultExceptionHandler<MyEvent> exHandler;

    EventProcessor<MyEvent, decltype(ringBuffer), decltype(barrierA), HandlerA>
        processorA(ringBuffer, barrierA, handlerA, exHandler);
    EventProcessor<MyEvent, decltype(ringBuffer), decltype(barrierB), HandlerB>
        processorB(ringBuffer, barrierB, handlerB, exHandler);

    // Get references to their sequences
    Sequence &seqA = processorA.getSequence();
    Sequence &seqB = processorB.getSequence();

    // Barrier for C, depending on sequences of A and B
    auto barrierC = sequencer.newBarrier({&seqA, &seqB});

    // Create processor for C
    EventProcessor<MyEvent, decltype(ringBuffer), decltype(barrierC), HandlerC>
        processorC(ringBuffer, barrierC, handlerC, exHandler);

    // Get reference to C's sequence
    Sequence &seqC = processorC.getSequence();
    // Set gating sequences for the ring buffer
    ringBuffer.setGatingSequences({&seqC});

    // Start threads
    std::thread threadA([&]
                        { processorA.run(); });
    std::thread threadB([&]
                        { processorB.run(); });
    std::thread threadC([&]
                        { processorC.run(); });

    // Publish events
    for (int i = 0; i < 5; ++i)
    {
        int64_t seq = ringBuffer.next();
        ringBuffer.get(seq).value = i;
        ringBuffer.publish(seq);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Allow some time for processing
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Halt processors
    processorA.halt();
    processorB.halt();
    processorC.halt();

    // Join threads
    threadA.join();
    threadB.join();
    threadC.join();
}

// ================================================
// Main
// ================================================
int main()
{
    simple();
    diamond();
    return 0;
}
