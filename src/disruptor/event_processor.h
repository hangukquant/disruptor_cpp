/**
 * @file event_processor.h
 * @brief Defines the EventProcessor class for consuming events from a disruptor ring buffer.
 */

#pragma once

#include <atomic>
#include <algorithm>
#include <stdexcept>

#include "sequence.h"
#include "sequencer.h"
#include "sequence_barrier.h"
#include "exception_handler.h"

namespace disruptor
{

    /**
     * @brief Enum representing the states of the event processor.
     */
    enum ProcessorState
    {
        IDLE = 0,
        HALTED = 1,
        RUNNING = 2,
    };

    /**
     * @brief Template class for processing events in the disruptor pattern.
     *
     * This class manages the consumption of events from a data provider using a sequence barrier
     * and invokes the appropriate event handler methods.
     *
     * @tparam T The type of event.
     * @tparam DataProvider The type providing access to events (e.g., RingBuffer).
     * @tparam SequenceBarrier The type of barrier used for waiting on sequences.
     * @tparam EventHandler The type of handler for processing events.
     * @tparam ExceptionHandlerType The type of exception handler (default: DefaultExceptionHandler).
     */
    template <typename T, typename DataProvider, typename SequenceBarrier, typename EventHandler,
              typename ExceptionHandlerType = DefaultExceptionHandler<T>>
    class EventProcessor
    {
    public:
        /**
         * @brief Constructs an EventProcessor.
         *
         * @param dataProvider Reference to the data provider (e.g., ring buffer).
         * @param sequenceBarrier Reference to the sequence barrier.
         * @param eventHandler Reference to the event handler.
         * @param exceptionHandler Reference to the exception handler.
         * @param batchSize The batch size for processing events (default: 64).
         */
        explicit EventProcessor(
            DataProvider &dataProvider,
            SequenceBarrier &sequenceBarrier,
            EventHandler &eventHandler,
            ExceptionHandlerType &exceptionHandler,
            int64_t batchSize = 64)
            : dataProvider_(dataProvider),
              sequenceBarrier_(sequenceBarrier),
              eventHandler_(eventHandler),
              exceptionHandler_(exceptionHandler),
              running_(IDLE),
              sequence_(-1),
              batchSizeOffset_(batchSize - 1)
        {
            eventHandler_.setSequenceCallback(sequence_);
        }

        /**
         * @brief Non-copyable and non-movable.
         */
        EventProcessor(const EventProcessor &) = delete;
        EventProcessor &operator=(const EventProcessor &) = delete;
        EventProcessor(EventProcessor &&) = delete;
        EventProcessor &operator=(EventProcessor &&) = delete;

        /**
         * @brief Starts the event processing loop.
         *
         * This method transitions the processor to RUNNING state and begins processing events.
         * It handles exceptions and ensures proper shutdown.
         */
        void run()
        {
            ProcessorState expected = IDLE;
            if (!running_.compare_exchange_strong(expected, RUNNING))
            {
                throw std::runtime_error("EventProcessor already running");
            }
            sequenceBarrier_.clearAlert();
            notifyStart();

            try
            {
                processEvents();
            }
            catch (const AlertException &)
            {
                if (running_.load(std::memory_order_acquire) != RUNNING)
                {
                    // called halt > barrier alert >> graceful shutdown
                }
                else
                {
                    throw;
                }
            }
            catch (...)
            {
                notifyShutdown();
                running_.store(IDLE, std::memory_order_release);
                throw;
            }

            notifyShutdown();
            running_.store(IDLE, std::memory_order_release);
        }

        /**
         * @brief Halts the event processor.
         *
         * Sets the state to HALTED and alerts the sequence barrier.
         */
        void halt()
        {
            running_.store(HALTED, std::memory_order_release);
            sequenceBarrier_.alert();
        }

        /**
         * @brief Checks if the processor is running.
         *
         * @return True if the processor is not IDLE, false otherwise.
         */
        bool isRunning()
        {
            return running_.load(std::memory_order_acquire) != IDLE;
        }

        /**
         * @brief Gets the sequence tracker for this processor.
         *
         * @return Reference to the sequence object.
         */
        Sequence &getSequence()
        {
            return sequence_;
        }

    private:
        DataProvider &dataProvider_;
        SequenceBarrier &sequenceBarrier_;
        EventHandler &eventHandler_;
        ExceptionHandlerType &exceptionHandler_;
        std::atomic<ProcessorState> running_;
        Sequence sequence_;
        int64_t batchSizeOffset_;

        /**
         * @brief Main loop for processing events.
         *
         * Waits for available sequences, processes batches of events, and updates the sequence.
         */
        void processEvents()
        {
            int64_t nextSequence = sequence_.get() + 1;
            while (running_.load(std::memory_order_acquire) == RUNNING)
            {
                try
                {
                    const int64_t availableSequence = sequenceBarrier_.waitFor(nextSequence);
                    const int64_t endOfBatch = std::min(nextSequence + batchSizeOffset_, availableSequence);

                    if (nextSequence <= endOfBatch)
                    {
                        eventHandler_.onBatchStart(endOfBatch - nextSequence + 1, availableSequence - nextSequence + 1);
                    }

                    while (nextSequence <= endOfBatch)
                    {
                        T &event = dataProvider_.get(nextSequence);
                        eventHandler_.onEvent(event, nextSequence, nextSequence == endOfBatch);
                        ++nextSequence;
                    }
                    sequence_.set(endOfBatch);
                }
                catch (const AlertException &)
                {
                    if (running_.load(std::memory_order_acquire) != RUNNING)
                    {
                        break;
                    }
                    else
                    {
                        throw;
                    }
                }
                catch (const std::exception &ex)
                {
                    exceptionHandler_.handleEventException(ex, nextSequence, dataProvider_.get(nextSequence));
                    sequence_.set(nextSequence);
                    ++nextSequence;
                }
            }
        }

        /**
         * @brief Notifies the event handler of a timeout.
         *
         * @param sequence The sequence at timeout.
         */
        void notifyTimeout(int64_t sequence)
        {
            eventHandler_.onTimeout(sequence);
        }

        /**
         * @brief Notifies the event handler that processing has started.
         */
        void notifyStart()
        {
            try
            {
                eventHandler_.onStart();
            }
            catch (const std::exception &ex)
            {
                exceptionHandler_.handleOnStartException(ex);
            }
        }

        /**
         * @brief Notifies the event handler that processing is shutting down.
         */
        void notifyShutdown()
        {
            try
            {
                eventHandler_.onShutdown();
            }
            catch (const std::exception &ex)
            {
                exceptionHandler_.handleOnShutdownException(ex);
            }
        }
    };

} // namespace disruptor