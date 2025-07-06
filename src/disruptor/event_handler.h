#pragma once 

#include "sequence.h"

namespace disruptor {

template <typename T>
class EventHandler {

public:
    virtual ~EventHandler() = default;

    virtual void onEvent(T& event, int64_t sequence, bool endOfBatch) = 0;

    virtual void onBatchStart(int64_t batchSize, int64_t queueDepth) {}
    virtual void onStart() {}
    virtual void onShutdown() {}
    virtual void onTimeout(int64_t sequence) {}

    // optional callback to allow early set of sequence counter in batch processing 
    virtual void setSequenceCallback(Sequence& sequenceCallback) {}

};


};