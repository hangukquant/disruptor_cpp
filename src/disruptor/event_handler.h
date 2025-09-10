/**
 * @file event_handler.h
 * @brief Defines the EventHandler interface for processing events in the disruptor pattern.
 */

#pragma once

#include "sequence.h"

namespace disruptor
{

    /**
     * @brief Template interface for handling events in the disruptor.
     *
     * This class defines callbacks for processing events, batch starts, lifecycle events, and timeouts.
     * Users should implement this interface to define custom event processing logic.
     *
     * @tparam T The type of event being processed.
     */
    template <typename T>
    class EventHandler
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~EventHandler() = default;

        /**
         * @brief Called when an event is available for processing.
         *
         * @param event The event to process.
         * @param sequence The sequence number of the event.
         * @param endOfBatch True if this is the last event in the current batch.
         */
        virtual void onEvent(T &event, int64_t sequence, bool endOfBatch) = 0;

        /**
         * @brief Called at the start of a batch of events.
         *
         * @param batchSize The number of events in the batch.
         * @param queueDepth The current depth of the queue.
         */
        virtual void onBatchStart(int64_t batchSize, int64_t queueDepth) {}

        /**
         * @brief Called when the event processor starts.
         */
        virtual void onStart() {}

        /**
         * @brief Called when the event processor shuts down.
         */
        virtual void onShutdown() {}

        /**
         * @brief Called when a timeout occurs while waiting for events.
         *
         * @param sequence The sequence number at the time of timeout.
         */
        virtual void onTimeout(int64_t sequence) {}

        /**
         * @brief Optional callback to set a sequence callback for early sequence updates in batch processing.
         *
         * @param sequenceCallback The sequence object to use for callbacks.
         */
        // optional callback to allow early set of sequence counter in batch processing
        virtual void setSequenceCallback(Sequence &sequenceCallback) {}
    };

} // namespace disruptor
;