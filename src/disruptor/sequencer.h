/**
 * @file sequencer.h
 * @brief Defines sequencer classes for managing sequence publication in the disruptor pattern.
 */

#pragma once

#include "sequence.h"
#include "wait_strategies.h"

namespace disruptor
{

    /**
     * @brief Concept for sequencer types.
     */
    template <typename S>
    concept SequencerConcept = requires(S s, int64_t seq, std::vector<Sequence *> gates) {
        { s.next() } -> std::same_as<int64_t>;
        { s.publish(seq) };
        { s.getCursor() } -> std::same_as<int64_t>;
        { s.setGatingSequences(gates) };
        { s.getMinimumGatingSequence() } -> std::same_as<int64_t>;
        { s.isAvailable(seq) } -> std::same_as<bool>;
        { s.getHighestPublishedSequence(seq, seq) } -> std::same_as<int64_t>;
    };

    // forward declaration
    template <SequencerConcept Sequencer, WaitStrategyConcept WaitStrategy>
    class SequenceBarrier;

    /**
     * @brief Single producer sequencer for the disruptor.
     *
     * Manages sequence allocation and publication for a single producer.
     *
     * @tparam N Buffer size (power of 2).
     * @tparam WaitStrategy The wait strategy.
     */
    template <size_t N, typename WaitStrategy>
    class SingleProducerSequencer
    {
        static_assert(
            (N & (N - 1)) == 0,
            "Buffer size must be power of 2");

    public:
        /**
         * @brief Constructs a SingleProducerSequencer.
         *
         * @param waitStrategy The wait strategy.
         */
        explicit SingleProducerSequencer(const WaitStrategy &waitStrategy)
            : waitStrategy_(waitStrategy),
              nextValue_(-1),
              cachedValue_(-1)
        {
            cursor_.set(-1);
        }

        /**
         * @brief Non-copyable and non-movable.
         */
        SingleProducerSequencer(const SingleProducerSequencer &) = delete;
        SingleProducerSequencer &operator=(const SingleProducerSequencer &) = delete;
        SingleProducerSequencer(SingleProducerSequencer &&) = delete;
        SingleProducerSequencer &operator=(SingleProducerSequencer &&) = delete;

        /**
         * @brief Claims the next n sequences.
         *
         * @param n Number of sequences to claim (default 1).
         * @return The last claimed sequence.
         */
        int64_t next(int64_t n = 1)
        {
            if (n < 1 || n > N)
            {
                throw std::invalid_argument("Invalid n in next()");
            }

            int64_t nextSeq = nextValue_ + n;
            int64_t wrapPoint = nextSeq - N;
            int64_t cachedGating = cachedValue_;

            if (wrapPoint > cachedGating || cachedGating > nextValue_)
            {
                int64_t minSeq;
                while (wrapPoint > (minSeq = getMinimumGatingSequence(nextValue_)))
                {
                    waitStrategy_.producerWait(); // spin/yield/block
                }
                cachedValue_ = minSeq;
            }

            nextValue_ = nextSeq;
            return nextSeq;
        }

        /**
         * @brief Publishes a sequence.
         *
         * @param sequence The sequence to publish.
         */
        void publish(int64_t sequence)
        {
            cursor_.set(sequence);
            waitStrategy_.signalAllWhenBlocking();
        }

        /**
         * @brief Gets the cursor sequence.
         *
         * @return The cursor value.
         */
        int64_t getCursor() const noexcept
        {
            return cursor_.get();
        }

        /**
         * @brief Sets the gating sequences.
         *
         * @param sequences The gating sequences.
         */
        void setGatingSequences(const std::vector<Sequence *> &sequences)
        {
            gatingSequences_ = sequences;
        }

        /**
         * @brief Gets the minimum gating sequence.
         *
         * @param minimum Initial minimum (default max int64).
         * @return The minimum sequence.
         */
        int64_t getMinimumGatingSequence(
            int64_t minimum = std::numeric_limits<int64_t>::max()) const
        {
            int64_t min = minimum;
            for (const auto *seq : gatingSequences_)
            {
                int64_t seqValue = seq->get();
                min = std::min(min, seqValue);
            }
            return min;
        }

        /**
         * @brief Checks if a sequence is available.
         *
         * @param sequence The sequence.
         * @return True if available.
         */
        bool isAvailable(int64_t sequence) const
        {
            return sequence <= cursor_.get();
        }

        /**
         * @brief Gets the highest published sequence in a range.
         *
         * @param lowerBound Lower bound.
         * @param available Available sequence.
         * @return The available sequence.
         */
        int64_t getHighestPublishedSequence(int64_t lowerBound, int64_t available) const
        {
            return available;
        }

        /**
         * @brief Creates a new sequence barrier.
         *
         * @param dependents Dependent sequences.
         * @return The sequence barrier.
         */
        auto newBarrier(const std::vector<Sequence *> &dependents)
        {
            return SequenceBarrier<SingleProducerSequencer<N, WaitStrategy>, WaitStrategy>(
                *this, waitStrategy_, cursor_, dependents);
        }

    private:
        Sequence cursor_;
        const WaitStrategy &waitStrategy_;
        int64_t nextValue_;   // holds the last sequence number claimed by the producer
        int64_t cachedValue_; // holds the last known minimum consumer sequence, slowest gating sequence
        std::vector<Sequence *> gatingSequences_;
    };

} // namespace disruptor