#pragma once

#include <atomic>

constexpr std::size_t kSizeOfCacheLine = 64;
constexpr std::size_t kSequencePaddingLength = (kSizeOfCacheLine - sizeof(std::atomic<int64_t>)) / sizeof(int64_t);

namespace disruptor {

class alignas(kSizeOfCacheLine) Sequence {
    public:
        Sequence(const Sequence&) = delete;
        Sequence(Sequence&&) = delete;
        Sequence& operator=(const Sequence&) = delete;
        Sequence& operator=(Sequence&&) = delete;

        explicit Sequence(int64_t initial = -1) noexcept 
            : sequence_(initial) {}

        [[nodiscard]] int64_t get() const noexcept {
            return sequence_.load(std::memory_order_acquire);
        }

        void set(int64_t value) noexcept {
            sequence_.store(value, std::memory_order_release);
        }

        [[nodiscard]] int64_t incrementAndGet(int64_t inc = 1) {
            return sequence_.fetch_add(inc, std::memory_order_release) + inc;
        }

        bool compareAndSet(int64_t& expected, int64_t desired) {
            return sequence_.compare_exchange_strong(expected, desired, std::memory_order_acq_rel);
        }

    private:
        std::atomic<int64_t> sequence_;
        int64_t pad[kSequencePaddingLength];
};

};

/*
memory barriers C++11:

memory_order_acquire

Ensures that all memory reads/writes after this load cannot be 
reordered before it. When a thread loads a value with acquire, 
it sees all the writes that happened-before the corresponding release 
store in another thread. Used for reading shared data after 
synchronization. 

memory_order_release

Ensures that all memory reads/writes before this store cannot be 
reordered after it. When a thread stores a value with release,
all previous writes in that thread are visible to any thread that 
does an acquire load of the same variable. Used for publishing 
data to other threads.

memory_order_acq_rel (used in compare_exchange_strong):

Combines both acquire and release semantics.
Ensures that operations before the atomic operation in program order 
happen-before operations after the atomic operation in another thread.
*/