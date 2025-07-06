#pragma once 

#include <atomic>

#include "sequence.h"
#include "sequencer.h"
#include "sequence_barrier.h"

namespace disruptor {

enum ProcessorState {
    IDLE = 0,
    HALTED = 1,
    RUNNING = 2,
};

template <typename T, typename DataProvider, typename SequenceBarrier, typename EventHandler>
class EventProcessor {

public:
    explicit EventProcessor(
        DataProvider& dataProvider,
        SequenceBarrier& sequenceBarrier,
        EventHandler& eventHandler,
        int64_t batchSize = 64
    ) : dataProvider_(dataProvider),
        sequenceBarrier_(sequenceBarrier),
        eventHandler_(eventHandler),
        running_(IDLE),
        sequence_(-1),
        batchSizeOffset_(batchSize-1) {
            eventHandler_.setSequenceCallback(sequence_);
        }
    
    void run() {
        ProcessorState expected = IDLE;
        if (!running_.compare_exchange_strong(expected, RUNNING)) {
            throw std::runtime_error("EventProcessor already running");
        }
        sequenceBarrier_.clearAlert();
        notifyStart();

        try {
            processEvents();
        } catch (const AlertException&) {
            if (running_.load(std::memory_order_acquire) != RUNNING) {
                // called halt > barrier alert >> graceful shutdown
            }
            else { 
                throw; 
            }
        } catch(...) {
            notifyShutdown();
            running_.store(IDLE, std::memory_order_release);
            throw;
        }

        notifyShutdown();
        running_.store(IDLE, std::memory_order_release);
            
    }

    void halt() {
        running_.store(HALTED, std::memory_order_release);
        sequenceBarrier_.alert();
    }

    bool isRunning() {
        return running_.load(std::memory_order_acquire) != IDLE;
    }
    
    Sequence& getSequence() {
        return sequence_;
    }

private:
    DataProvider& dataProvider_;
    SequenceBarrier& sequenceBarrier_;
    EventHandler& eventHandler_;
    std::atomic<ProcessorState> running_;
    Sequence sequence_;
    int64_t batchSizeOffset_;

    void processEvents() {
        int64_t nextSequence = sequence_.get() + 1;
        while (running_.load(std::memory_order_acquire) == RUNNING) {
            try {
                const int64_t availableSequence = sequenceBarrier_.waitFor(nextSequence);
                const int64_t endOfBatch = std::min(nextSequence + batchSizeOffset_, availableSequence);
                
                if (nextSequence <= endOfBatch) {
                    eventHandler_.onBatchStart(endOfBatch - nextSequence + 1, availableSequence - nextSequence + 1);
                }

                while (nextSequence <= endOfBatch) {
                    T& event = dataProvider_.get(nextSequence);
                    eventHandler_.onEvent(event, nextSequence, nextSequence == endOfBatch);
                    ++nextSequence;
                }
                sequence_.set(endOfBatch);
            } catch (const AlertException&) {
                if (running_.load(std::memory_order_acquire) != RUNNING) {
                    break;
                } else {
                    throw;
                }
            } catch (const std::exception& ex) {
                handleEventException(ex, nextSequence, dataProvider_.get(nextSequence));
                sequence_.set(nextSequence);
                ++nextSequence;
            }
        }
    }

    void handleEventException(const std::exception& ex, int64_t sequence, T& event) {
        throw std::runtime_error(
            std::string("Fatal exception at sequence ") + std::to_string(sequence) +
            ": " + ex.what()
        );
    }

    void notifyTimeout(int64_t sequence) {
        eventHandler_.onTimeout(sequence);
    }

    void notifyStart() {
        eventHandler_.onStart();
    }

    void notifyShutdown() {
        eventHandler_.onShutdown();
    }
};


};