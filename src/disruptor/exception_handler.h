/**
 * @file exception_handler.h
 * @brief Defines the ExceptionHandler interface for handling exceptions in the disruptor pattern.
 */

#pragma once

#include <exception>
#include <string>
#include <stdexcept>

namespace disruptor
{

    /**
     * @brief Template interface for handling exceptions in the event processor.
     *
     * Users can implement this to customize exception handling during event processing,
     * startup, and shutdown.
     *
     * @tparam T The type of event being processed.
     */
    template <typename T>
    class ExceptionHandler
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~ExceptionHandler() = default;

        /**
         * @brief Handles exceptions thrown during event processing.
         *
         * @param ex The exception caught.
         * @param sequence The sequence where the exception occurred.
         * @param event The event being processed.
         */
        virtual void handleEventException(const std::exception &ex, int64_t sequence, T &event) = 0;

        /**
         * @brief Handles exceptions thrown during processor startup.
         *
         * @param ex The exception caught.
         */
        virtual void handleOnStartException(const std::exception &ex) = 0;

        /**
         * @brief Handles exceptions thrown during processor shutdown.
         *
         * @param ex The exception caught.
         */
        virtual void handleOnShutdownException(const std::exception &ex) = 0;
    };

    /**
     * @brief Default exception handler that rethrows exceptions as runtime_errors.
     *
     * Matches the current behavior in EventProcessor.
     *
     * @tparam T The type of event.
     */
    template <typename T>
    class DefaultExceptionHandler : public ExceptionHandler<T>
    {
    public:
        void handleEventException(const std::exception &ex, int64_t sequence, T &event) override
        {
            throw std::runtime_error(
                std::string("Fatal exception at sequence ") + std::to_string(sequence) +
                ": " + ex.what());
        }

        void handleOnStartException(const std::exception &ex) override
        {
            throw std::runtime_error("Exception during onStart: " + std::string(ex.what()));
        }

        void handleOnShutdownException(const std::exception &ex) override
        {
            throw std::runtime_error("Exception during onShutdown: " + std::string(ex.what()));
        }
    };

} // namespace disruptor