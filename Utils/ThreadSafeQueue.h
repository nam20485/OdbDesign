//
// Created by nam20485 on 5/25/22.
//

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <sstream>
#include <functional>
#include <chrono>


namespace Utils
{
    /** std::_queue wrapped in thread-safety mechanisms
     *
     * @tparam T: type of _queue's items
     */
    template <typename T>
    class ThreadSafeQueue
    {
    public:
        typedef std::function<void(const std::string&)> internalLogHandlerProc;

        enum class wait_result
        {
            timed_out,
            interrupted,
            notified
        };

        ThreadSafeQueue()
            : _interrupted(false)
        {
        }

        /** Push an object onto the _queue in a thread-safe manner
         *
         * @param item: item to push onto _queue
         */
        void push(T&& item);

        /** Get a reference to the head of the _queue in a type-safe manner
         *
         * @param wait: whether to wait for the _queue to become not empty and notified
         * @return reference to the item
         */
        T& front(bool wait);

        /** Remove last item off the _queue
         *
         */
        void pop();

        /** Set the interrupt and notify any wait()'s on the _queue without push();ing an item
         *
         */
        void interrupt();

        /**
         * Wait indefinitely for the _queue to be notified by either push() or interrupt()
         * @return wait_result specifying result of waiting, one of: { notified, interrupted }
         */
        wait_result wait();

        /**
         * Waits given duration for the _queue to be notified by either push() or interrupt(), or to time out
         * @param wait_ms std::chrono::duration specifying duration, in ms, to wait before timing out
         * @return wait_result specifying result of waiting, one of: { notified, interrupted, timed_out }
         * */
        wait_result wait(std::chrono::milliseconds wait_ms);

        /** Check if _queue is empty
         *
         * @return true if _queue is empty, false otherwise
         */
        bool empty();

        /**
         * Sets a function that is used to handle internal-level log messages. Invoked when logInternal() is called.
         * @param internal_log_handler: function to invoke when internal messages need to be logged
         */
        void registerInternalLogHandler(internalLogHandlerProc internalLogHandler);

    private:
        std::mutex _m;
        std::condition_variable _cv;
        std::queue<T> _queue;
        std::atomic_bool _interrupted;

        internalLogHandlerProc _internalLogHandler;

        void logInternal(const std::string& message);
    };


    template<typename T>
    void ThreadSafeQueue<T>::push(T&& item)
    {
        {
            logInternal("[ThreadSafeQueue<TResults>::push] enter, taking lock...");

            // unlock once scope is exited
            std::lock_guard lock(_m);

            std::stringstream ss;
            ss << "[ThreadSafeQueue<TResults>::push] lock taken, pushing item (_queue.size(): " << _queue.size() << ')';
            logInternal(ss.str());

            _queue.push(item);

            ss.str(""); // clear the stream out
            ss << "[ThreadSafeQueue<TResults>::push] item pushed, _queue.size(): " << _queue.size();
            logInternal(ss.str());
        }

        logInternal("[ThreadSafeQueue<TResults>::push] lock given up, notifying...");

        // unlock before notifying
        _cv.notify_one();

        logInternal("[ThreadSafeQueue<TResults>::push] _cv notified, exit");
    }

    template<typename T>
    T& ThreadSafeQueue<T>::front(bool wait)
    {
        std::unique_lock lock(_m);
        if (wait)
        {
            _cv.wait(lock,
                [&]
                {
                    return _queue.empty() == false;
                });
        }
        return _queue.front();
    }

    template<typename T>
    void ThreadSafeQueue<T>::pop()
    {
        std::lock_guard lock(_m);
        _queue.pop();
    }

    template<typename T>
    void ThreadSafeQueue<T>::interrupt()
    {
        // lock and notify
        {
            std::unique_lock lock(_m);

            // set interrupt to return from wait even if _queue is empty
            _interrupted.store(true);
        }
        _cv.notify_one();
    }

    template<typename T>
    typename ThreadSafeQueue<T>::wait_result ThreadSafeQueue<T>::wait()
    {
        logInternal("[ThreadSafeQueue<TResults>::wait] enter, taking lock...");

        // wait until _queue is not empty _OR_ interrupt has been set
        std::unique_lock lock(_m);

        logInternal("[ThreadSafeQueue<TResults>::wait] lock taken, begin waiting...");

        _cv.wait(lock,
            [&]
            {
                return _queue.empty() == false || _interrupted.load();
            });

        std::stringstream ss;
        ss << "[ThreadSafeQueue<TResults>::wait] wait returned, exiting (_queue.size() => " << _queue.size() << ", _interrupted = " << _interrupted.load() << ')';
        logInternal(ss.str());

        // return whether _interrupted was tripped
        if (_interrupted.load())
        {
            return ThreadSafeQueue<T>::wait_result::interrupted;
        }
        else
        {
            return ThreadSafeQueue<T>::wait_result::notified;
        }
    }

    template<typename T>
    typename ThreadSafeQueue<T>::wait_result ThreadSafeQueue<T>::wait(std::chrono::milliseconds wait_ms)
    {
        logInternal("[ThreadSafeQueue<TResults>::wait] enter, taking lock...");

        // wait until _queue is not empty _OR_ interrupt has been set
        std::unique_lock lock(_m);

        logInternal("[ThreadSafeQueue<TResults>::wait] lock taken, begin waiting...");

        auto expired = !_cv.wait_for(lock, wait_ms, // NOLINT(clion-misra-cpp2008-5-3-1)
            [&]
            {
                return _queue.empty() == false || _interrupted.load();
            });

        std::stringstream ss;
        ss << "[ThreadSafeQueue<TResults>::wait] wait returned, exiting (_queue.size() => " << _queue.size() << ", _interrupted = " << _interrupted.load() << ')';
        logInternal(ss.str());

        wait_result wait_result;
        if (expired)
        {
            wait_result = ThreadSafeQueue<T>::wait_result::timed_out;
        }
        else if (_interrupted.load())
        {
            wait_result = ThreadSafeQueue<T>::wait_result::interrupted;
        }
        else
        {
            wait_result = ThreadSafeQueue<T>::wait_result::notified;
        }
        return wait_result;
    }

    template<typename T>
    bool ThreadSafeQueue<T>::empty()
    {
        std::unique_lock lock(_m);
        return _queue.empty();
    }

    template<typename T>
    void ThreadSafeQueue<T>::logInternal(const std::string& message)
    {
        if (_internalLogHandler)
        {
            _internalLogHandler(message);
        }
    }

    template<typename T>
    void ThreadSafeQueue<T>::registerInternalLogHandler(ThreadSafeQueue::internalLogHandlerProc internalLogHandler)
    {
        _internalLogHandler = std::move(internalLogHandler);
    }

} // namespace Lib::Utils
