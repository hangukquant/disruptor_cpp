#pragma once 

#include "sequence.h"
#include "wait_strategies.h"

namespace disruptor {

template <typename S>
concept SequencerConcept = requires(S s, int64_t seq, std::vector<Sequence*> gates) {
    { s.next() } -> std::same_as<int64_t>;
    { s.publish(seq) };
    { s.getCursor() } -> std::same_as<int64_t>;
    { s.setGatingSequences(gates) };
    { s.getMinimumGatingSequence() } -> std::same_as<int64_t>;
    { s.isAvailable(seq) } -> std::same_as<bool>;
    { s.getHighestPublishedSequence(seq, seq) } -> std::same_as<int64_t>;
};

//forward declaration
template <SequencerConcept Sequencer, WaitStrategyConcept WaitStrategy>
class SequenceBarrier;

template <size_t N, typename WaitStrategy>
class SingleProducerSequencer {
    static_assert(
        (N & (N - 1)) == 0, 
        "Buffer size must be power of 2"
    );

public:
    explicit SingleProducerSequencer(const WaitStrategy& waitStrategy)
        : waitStrategy_(waitStrategy),
          nextValue_(-1),
          cachedValue_(-1) {
        cursor_.set(-1); 
    }

    int64_t next(int64_t n = 1) {
        if (n < 1 || n > N) {
            throw std::invalid_argument("Invalid n in next()");
        }

        int64_t nextSeq = nextValue_ + n;
        int64_t wrapPoint = nextSeq - N;
        int64_t cachedGating = cachedValue_;
        
        if (wrapPoint > cachedGating || cachedGating > nextValue_) {
            int64_t minSeq;
            while (wrapPoint > (minSeq = getMinimumGatingSequence(nextValue_))) {
                waitStrategy_.producerWait();  // spin/yield/block
            }
            cachedValue_ = minSeq;
        }

        nextValue_ = nextSeq;
        return nextSeq;
    }

    void publish(int64_t sequence) {
        cursor_.set(sequence);
        waitStrategy_.signalAllWhenBlocking();   
    }

    int64_t getCursor() const {
        return cursor_.get();
    }

    void setGatingSequences(const std::vector<Sequence*>& sequences) {
        gatingSequences_ = sequences;
    }

    int64_t getMinimumGatingSequence(
        int64_t minimum = std::numeric_limits<int64_t>::max()
    ) const {
        int64_t min = minimum;
        for (const auto* seq : gatingSequences_) {
            int64_t seqValue = seq->get();
            min = std::min(min, seqValue);
        }
        return min;
    }

    bool isAvailable(int64_t sequence) const {
        return sequence <= cursor_.get();
    }
    
    int64_t getHighestPublishedSequence(int64_t lowerBound, int64_t available) const {
        return available;
    }

    auto newBarrier(const std::vector<Sequence*>& dependents) {
        return SequenceBarrier<SingleProducerSequencer<N, WaitStrategy>, WaitStrategy>(
            *this, waitStrategy_, cursor_, dependents
        );
    }
    
private:
    Sequence cursor_;
    WaitStrategy& waitStrategy_;
    int64_t nextValue_; //holds the last sequence number claimed by the producer
    int64_t cachedValue_; //holds the last known minimum consumer sequence, slowest gating sequence
    std::vector<Sequence*> gatingSequences_;
};

};

