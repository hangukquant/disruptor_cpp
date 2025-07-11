#pragma once

#include <vector>
#include <thread>
#include <limits>
#include "sequence.h"

#if defined(__aarch64__)
#include <arm_acle.h>
inline void cpu_relax() {
    __yield();
}
#elif defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h> 
inline void cpu_relax() {
    _mm_pause();
}
#else
inline void cpu_relax() {
    std::this_thread::yield();
}
#endif

namespace disruptor {

[[nodiscard]] inline int64_t dependents_get(
    const Sequence& cursor,
    const std::vector<Sequence*>& dependents, 
    int64_t minimum = std::numeric_limits<int64_t>::max()
) {
    if (dependents.empty()) {
        return cursor.get();
    }
    for (const auto* seq : dependents) {
        if (seq) {
            minimum = std::min(minimum,seq->get());
        }
    }
    return minimum;
}

template <typename W>
concept WaitStrategyConcept = requires(W w, int64_t seq, Sequence cursor, std::vector<Sequence*> dependents) {
    { w.signalAllWhenBlocking() };
    { w.producerWait() };
};


class BusySpinWaitStrategy {
public:
    // returns sequence available, possibly larger than requested sequence
    template <typename Barrier>
    int64_t waitFor(
        int64_t sequence, 
        const Sequence& cursor,
        const std::vector<Sequence*>& dependents,
        Barrier& barrier
    ) const {
        int64_t available_sequence;
        while((available_sequence = dependents_get(cursor,dependents)) < sequence) {
            barrier.checkAlert();
            cpu_relax();
        }
        return available_sequence;        
    }

    void signalAllWhenBlocking() const {}

    void producerWait() const {
        cpu_relax();
    }

};

};