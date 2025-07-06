#pragma once 

#include <atomic>
#include <stdexcept>
#include <vector>
#include <memory>

#include "sequence.h"
#include "sequencer.h"
#include "wait_strategies.h"

namespace disruptor {

class AlertException : public std::exception {
public:
    const char* what() const noexcept override {
        return "Barrier alert triggered.";
    }
};

template <SequencerConcept Sequencer, WaitStrategyConcept WaitStrategy>
class SequenceBarrier {
public:
    SequenceBarrier(
        Sequencer& sequencer_,
        const WaitStrategy& waitStrategy_,
        const Sequence& cursor_,
        const std::vector<Sequence*>& dependents_
    ) : sequencer_(sequencer_),
        waitStrategy_(waitStrategy_),
        cursor_(cursor_),
        dependents_(dependents_),
        alerted_(false) {}

    //Wait for the given sequence to be available for consumption.
    int64_t waitFor(int64_t sequence) {
        checkAlert();

        int64_t available = waitStrategy_.waitFor(sequence, cursor_, dependents_, *this);

        if (available < sequence) {
            return available;
        }

        return sequencer_.getHighestPublishedSequence(sequence, available);
    }

    int64_t getCursor() const {
        return dependents_get(cursor_, dependents_);
    }

    void alert() {
        alerted_.store(true, std::memory_order_release);
        waitStrategy_.signalAllWhenBlocking();
    }

    void clearAlert() {
        alerted_.store(false, std::memory_order_release);
    }

    bool isAlerted() const {
        return alerted_.load(std::memory_order_acquire);
    }

    void checkAlert() const {
        if (isAlerted()) {
            throw AlertException();
        }
    }

private:
    Sequencer& sequencer_;
    const WaitStrategy& waitStrategy_;
    const Sequence& cursor_;
    std::vector<Sequence*> dependents_;
    std::atomic<bool> alerted_;

};

};

