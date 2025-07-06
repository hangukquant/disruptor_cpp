#pragma once 

#include <array>
#include <vector>

#include "sequencer.h"

namespace disruptor {

template <
    typename T, 
    size_t N, 
    SequencerConcept Sequencer, 
    typename EventFactory
>
class RingBuffer {
    static_assert(
        (N & (N - 1)) == 0, 
        "Buffer size must be power of 2"
    );
    
    public:
        RingBuffer(
            Sequencer& sequencer,
            const EventFactory& factory
        ) : sequencer_(sequencer) {
            for (size_t i = 0; i < N; ++i) {
                buffer_[i] = factory();
            }
        }

        int64_t next() {
            return sequencer_.next();
        }

        void publish(int64_t sequence) {
            sequencer_.publish(sequence);
        }
        
        T& get(int64_t sequence) {
            return buffer_[sequence & (N - 1)];
        }

        const T& get(int64_t sequence) const {
            return buffer_[sequence & (N - 1)];
        }
        
        void setGatingSequences(const std::vector<Sequence*>& sequences) {
            sequencer_.setGatingSequences(sequences);
        }

        int64_t getCursor() const {
            return sequencer_.getCursor();
        }
        
        int64_t getMinimumGatingSequence() const {
            return sequencer_.getMinimumGatingSequence();
        }

        RingBuffer(const RingBuffer&) = delete;
        RingBuffer& operator=(const RingBuffer&) = delete;
        RingBuffer(RingBuffer&&) = delete;
        RingBuffer& operator=(RingBuffer&&) = delete;

    private:
        std::array<T, N> buffer_;
        Sequencer& sequencer_;
        
};

};