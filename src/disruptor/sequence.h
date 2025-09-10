/**
 * @file sequence.h
 * @brief Defines the Sequence class for the disruptor pattern, providing atomic sequence management with cache-line alignment.
 */

#pragma once

#include <atomic>

constexpr std::size_t kSizeOfCacheLine = 64;
constexpr std::size_t kSequencePaddingLength = (kSizeOfCacheLine - sizeof(std::atomic<int64_t>)) / sizeof(int64_t);

namespace disruptor
{
    /**
     * @class Sequence
     * @brief Atomic, cache-line aligned sequence counter for the disruptor pattern.
     *
     * Provides atomic operations for sequence management, with padding to avoid false sharing.
     * Not copyable or movable.
     */
    class alignas(kSizeOfCacheLine) Sequence
    {
    public:
        /**
         * @brief Non-copyable and non-movable.
         */
        Sequence(const Sequence &) = delete;
        Sequence(Sequence &&) = delete;
        Sequence &operator=(const Sequence &) = delete;
        Sequence &operator=(Sequence &&) = delete;

        /**
         * @brief Construct a new Sequence object.
         * @param initial The initial value of the sequence (default: -1).
         */
        explicit Sequence(int64_t initial = -1) noexcept
            : sequence_(initial) {}

        /**
         * @brief Atomically gets the current value of the sequence.
         * @return The current value.
         * @note Uses std::memory_order_acquire.
         */
        [[nodiscard]] int64_t get() const noexcept
        {
            return sequence_.load(std::memory_order_acquire);
        }

        /**
         * @brief Atomically sets the sequence to a new value.
         * @param value The value to set.
         * @note Uses std::memory_order_release.
         */
        void set(int64_t value) noexcept
        {
            sequence_.store(value, std::memory_order_release);
        }

        /**
         * @brief Atomically increments the sequence by the given value.
         * @param inc The increment value (default: 1).
         * @return The new value after increment.
         * @note Uses std::memory_order_release.
         */
        [[nodiscard]] int64_t incrementAndGet(int64_t inc = 1)
        {
            return sequence_.fetch_add(inc, std::memory_order_release) + inc;
        }

        /**
         * @brief Atomically compares and sets the sequence value.
         * @param expected Reference to the expected value; updated if the comparison fails.
         * @param desired The value to set if the current value equals expected.
         * @return true if the value was set, false otherwise.
         * @note Uses std::memory_order_acq_rel.
         */
        bool compareAndSet(int64_t &expected, int64_t desired)
        {
            return sequence_.compare_exchange_strong(expected, desired, std::memory_order_acq_rel);
        }

    private:
        /**
         * @brief The atomic sequence value.
         */
        std::atomic<int64_t> sequence_;
        /**
         * @brief Padding to avoid false sharing between Sequence objects.
         */
        int64_t pad[kSequencePaddingLength];
    };

} // namespace disruptor