/**
 * @file sequence_barrier.h
 * @brief Defines the SequenceBarrier class for waiting on sequences in the disruptor pattern.
 */

#pragma once

#include <atomic>
#include <stdexcept>
#include <vector>
#include <memory>

#include "sequence.h"
#include "sequencer.h"
#include "wait_strategies.h"

namespace disruptor
{

    /**
     * @brief Exception thrown when the barrier is alerted.
     */
    class AlertException : public std::exception
    {
    public:
        const char *what() const noexcept override
        {
            return "Barrier alert triggered.";
        }
    };

    /**
     * @brief Template class for a sequence barrier in the disruptor.
     *
     * Manages waiting for sequences to become available, handling alerts and dependencies.
     *
     * @tparam Sequencer The sequencer type.
     * @tparam WaitStrategy The wait strategy type.
     */
    template <SequencerConcept Sequencer, WaitStrategyConcept WaitStrategy>
    class SequenceBarrier
    {
    public:
        /**
         * @brief Constructs a SequenceBarrier.
         *
         * @param sequencer_ The sequencer.
         * @param waitStrategy_ The wait strategy.
         * @param cursor_ The cursor sequence.
         * @param dependents_ Dependent sequences.
         */
        SequenceBarrier(
            Sequencer &sequencer_,
            const WaitStrategy &waitStrategy_,
            const Sequence &cursor_,
            const std::vector<Sequence *> &dependents_) : sequencer_(sequencer_),
                                                          waitStrategy_(waitStrategy_),
                                                          cursor_(cursor_),
                                                          dependents_(dependents_),
                                                          alerted_(false) {}

        /**
         * @brief Non-copyable but movable.
         */
        SequenceBarrier(const SequenceBarrier &) = delete;
        SequenceBarrier &operator=(const SequenceBarrier &) = delete;
        SequenceBarrier(SequenceBarrier &&) = default;
        SequenceBarrier &operator=(SequenceBarrier &&) = default;

        /**
         * @brief Waits for the given sequence to be available.
         *
         * @param sequence The sequence to wait for.
         * @return The available sequence.
         */
        // Wait for the given sequence to be available for consumption.
        int64_t waitFor(int64_t sequence)
        {
            checkAlert();

            int64_t available = waitStrategy_.waitFor(sequence, cursor_, dependents_, *this);

            if (available < sequence)
            {
                return available;
            }

            return sequencer_.getHighestPublishedSequence(sequence, available);
        }

        /**
         * @brief Gets the current cursor value.
         *
         * @return The minimum sequence from cursor and dependents.
         */
        int64_t getCursor() const
        {
            return dependents_get(cursor_, dependents_);
        }

        /**
         * @brief Alerts the barrier, waking waiting threads.
         */
        void alert()
        {
            alerted_.store(true, std::memory_order_release);
            waitStrategy_.signalAllWhenBlocking();
        }

        /**
         * @brief Clears the alert status.
         */
        void clearAlert()
        {
            alerted_.store(false, std::memory_order_release);
        }

        /**
         * @brief Checks if the barrier is alerted.
         *
         * @return True if alerted, false otherwise.
         */
        bool isAlerted() const
        {
            return alerted_.load(std::memory_order_acquire);
        }

        /**
         * @brief Throws AlertException if alerted.
         */
        void checkAlert() const
        {
            if (isAlerted())
            {
                throw AlertException();
            }
        }

    private:
        Sequencer &sequencer_;
        const WaitStrategy &waitStrategy_;
        const Sequence &cursor_;
        std::vector<Sequence *> dependents_;
        std::atomic<bool> alerted_;
    };

};