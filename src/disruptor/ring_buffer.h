/**
 * @file ring_buffer.h
 * @brief Defines the RingBuffer class for the disruptor pattern.
 */

#pragma once

#include <array>
#include <vector>

#include "sequencer.h"

namespace disruptor
{

    /**
     * @brief Template class implementing a ring buffer for the disruptor.
     *
     * The ring buffer stores events and manages publication through a sequencer.
     * Buffer size must be a power of 2.
     *
     * @tparam T The type of event stored in the buffer.
     * @tparam N The size of the buffer (must be power of 2).
     * @tparam Sequencer The sequencer type used for managing sequences.
     * @tparam EventFactory Factory for creating initial events.
     */
    template <
        typename T,
        size_t N,
        SequencerConcept Sequencer,
        typename EventFactory>
    class RingBuffer
    {
        static_assert(
            (N & (N - 1)) == 0,
            "Buffer size must be power of 2");

    public:
        /**
         * @brief Constructs a RingBuffer.
         *
         * Initializes the buffer with events created by the factory.
         *
         * @param sequencer Reference to the sequencer.
         * @param factory The event factory.
         */
        RingBuffer(
            Sequencer &sequencer,
            const EventFactory &factory)
            : sequencer_(sequencer)
        {
            for (size_t i = 0; i < N; ++i)
            {
                buffer_[i] = factory();
            }
        }

        /**
         * @brief Claims the next n sequences for publication.
         *
         * @param n Number of sequences to claim (default 1).
         * @return The last claimed sequence number.
         */
        int64_t next(int64_t n = 1)
        {
            return sequencer_.next(n);
        }

        /**
         * @brief Publishes an event at the given sequence.
         *
         * @param sequence The sequence to publish.
         */
        void publish(int64_t sequence)
        {
            sequencer_.publish(sequence);
        }

        /**
         * @brief Gets a reference to the event at the given sequence.
         *
         * @param sequence The sequence number.
         * @return Reference to the event.
         */
        T &get(int64_t sequence)
        {
            return buffer_[sequence & (N - 1)];
        }

        /**
         * @brief Gets a const reference to the event at the given sequence.
         *
         * @param sequence The sequence number.
         * @return Const reference to the event.
         */
        const T &get(int64_t sequence) const
        {
            return buffer_[sequence & (N - 1)];
        }

        /**
         * @brief Gets a pointer to the slot at the given sequence.
         *
         * @param sequence The sequence number.
         * @return Pointer to the event.
         */
        T *get_ptr(int64_t sequence)
        {
            return &buffer_[sequence & (N - 1)];
        }

        /**
         * @brief Gets a const pointer to the event at the given sequence.
         *
         * @param sequence The sequence number.
         * @return Const pointer to the event.
         */
        const T *get_ptr(int64_t sequence) const
        {
            return &buffer_[sequence & (N - 1)];
        }

        /**
         * @brief Sets the gating sequences for the sequencer.
         *
         * @param sequences Vector of pointers to gating sequences.
         */
        void setGatingSequences(const std::vector<Sequence *> &sequences)
        {
            sequencer_.setGatingSequences(sequences);
        }

        /**
         * @brief Gets the current cursor position.
         *
         * @return The cursor sequence.
         */
        int64_t getCursor() const
        {
            return sequencer_.getCursor();
        }

        /**
         * @brief Gets the minimum gating sequence.
         *
         * @return The minimum gating sequence.
         */
        int64_t getMinimumGatingSequence() const
        {
            return sequencer_.getMinimumGatingSequence();
        }

        /**
         * @brief Non-copyable and non-movable.
         */
        RingBuffer(const RingBuffer &) = delete;
        RingBuffer &operator=(const RingBuffer &) = delete;
        RingBuffer(RingBuffer &&) = delete;
        RingBuffer &operator=(RingBuffer &&) = delete;

    private:
        std::array<T, N> buffer_;
        Sequencer &sequencer_;
    };

} // namespace disruptor