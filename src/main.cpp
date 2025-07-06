#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

#include "disruptor/ring_buffer.h"
#include "disruptor/event_handler.h"
#include "disruptor/event_processor.h"
#include "disruptor/sequencer.h"
#include "disruptor/wait_strategies.h"

using namespace disruptor;

struct MyEvent {
    int64_t value;
};

auto myEventFactory = []() -> MyEvent {
    return MyEvent{0};
};

class MyEventHandler : public EventHandler<MyEvent> {
public:
    void onEvent(MyEvent& event, int64_t sequence, bool endOfBatch) override {
        std::cout << "Processing event at sequence " << sequence
                  << " with value: " << event.value << std::endl;
        if (sequenceCallback_) {
            sequenceCallback_->set(sequence);
        }
    }

    void onStart() override {
        std::cout << "[Handler] Started.\n";
    }

    void onShutdown() override {
        std::cout << "[Handler] Shutdown.\n";
    }

    void setSequenceCallback(Sequence& sequenceCallback) override {
        sequenceCallback_ = &sequenceCallback;
    }
private:
    Sequence* sequenceCallback_ = nullptr;
};

int main() {
    constexpr size_t bufferSize = 1024;

    BusySpinWaitStrategy waitStrategy;
    SingleProducerSequencer<bufferSize, BusySpinWaitStrategy> sequencer(waitStrategy);

    // Set up ring buffer
    RingBuffer<MyEvent, bufferSize, decltype(sequencer), decltype(myEventFactory)>
        ringBuffer(sequencer, myEventFactory);

    // Gating sequence
    Sequence consumerSequence;
    ringBuffer.setGatingSequences({&consumerSequence});

    // Create barrier
    auto barrier = sequencer.newBarrier({}); //only wait on cursor

    // Event handler and processor
    MyEventHandler handler;
    EventProcessor<MyEvent, decltype(ringBuffer), decltype(barrier), MyEventHandler>
        processor(ringBuffer, barrier, handler);

    // Set the sequence callback so the processor updates the gating sequence
    handler.setSequenceCallback(consumerSequence);

    // Start processor thread
    std::thread consumerThread([&]() {
        processor.run();
    });

    // Simulate producer
    for (int i = 0; i < 10; ++i) {
        int64_t seq = ringBuffer.next();
        MyEvent& evt = ringBuffer.get(seq);
        evt.value = i;
        ringBuffer.publish(seq);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop processor
    std::this_thread::sleep_for(std::chrono::seconds(1));
    processor.halt();
    consumerThread.join();

    return 0;
}
