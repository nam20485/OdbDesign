//
// Created by nam20485 on 5/30/22.
//

#pragma once

#include "ThreadSafeQueue.h"
#include <exception>
#include <mutex>
#include <string>
#include <thread>
#include <memory>
#include <chrono>


namespace Utils
{
    template<typename TWorkItem>
    class WorkQueueLoopThread
    {
    public:
        typedef std::function<bool(TWorkItem&)> processWorkItemFunc;

        WorkQueueLoopThread();
        explicit WorkQueueLoopThread(processWorkItemFunc processWorkItemProc/*, bool enableInternalLogging*/);

        void addWorkItem(TWorkItem&& workItem);
        //void addWorkItem(TWorkItem& workItem);
        void startProcessing();
        void stopProcessing();

        /** Text of the last processor exception that stopped the loop
         * (empty when no exception occurred). A processor exception is
         * contained here -- it stops the loop like a failed work item
         * instead of propagating out of the loop thread, which would
         * std::terminate the whole process.
         */
        std::string lastProcessingError() const;

    protected:
        void processWorkItemsLoop();
        virtual bool processWorkItem(TWorkItem& workItem);

    private:
        ThreadSafeQueue<TWorkItem> _workItemQueue;
        std::unique_ptr<std::thread> _processWorkItemsLoopThread;
        std::atomic_bool _stopProcessingWorkItemsFlag;
        processWorkItemFunc _processWorkItemProc;

        const bool _StopProcessingOnWorkItemFail{ true };
        bool _enableInternalLogging{ false };

        // set when a work item FAILS (or throws): the loop abandons the rest
        // of the queue instead of draining it; a NORMAL stopProcessing()
        // still drains everything queued before the stop call
        std::atomic_bool _abandonRemainingItems{ false };

        mutable std::mutex _lastProcessingErrorMutex;
        std::string _lastProcessingError;

    };
    
    template<typename TWorkItem>
    /*virtual*/ bool WorkQueueLoopThread<TWorkItem>::processWorkItem(TWorkItem& workItem)
    {
        if (_processWorkItemProc == nullptr) return false;       
        return _processWorkItemProc(workItem);       
    }


    template<typename TWorkItem>
    inline WorkQueueLoopThread<TWorkItem>::WorkQueueLoopThread()
        : WorkQueueLoopThread(nullptr)
    {
    }

    template<typename TWorkItem>
    WorkQueueLoopThread<TWorkItem>::WorkQueueLoopThread(WorkQueueLoopThread::processWorkItemFunc processWorkItemProc/*, bool enableInternalLogging*/)
        : _stopProcessingWorkItemsFlag(false)
        , _processWorkItemProc(processWorkItemProc)
        //, _enableInternalLogging(enableInternalLogging)
    {
        if (_enableInternalLogging)
        {
            _workItemQueue.registerInternalLogHandler([&](const std::string& m)
                {
                    std::cout << m << std::endl;
                });
        }
    }

    template<typename TWorkItem>
    void WorkQueueLoopThread<TWorkItem>::addWorkItem(TWorkItem&& workItem)
    {
        _workItemQueue.push(std::move(workItem));
    }

  //  template<typename TWorkItem>
  //  inline void WorkQueueLoopThread<TWorkItem>::addWorkItem(TWorkItem& workItem)
  //  {
		//_workItemQueue.push(workItem);
  //  }

    template<typename TWorkItem>
    void WorkQueueLoopThread<TWorkItem>::processWorkItemsLoop()
    {
        for (;;)
        {
            // exit only when stopped AND there is nothing (allowed) left to
            // drain. Order matters: stop flag first, emptiness second. A
            // normal stopProcessing() therefore FLUSHES everything queued
            // before the stop call (push happens-before the stop store, and
            // the emptiness re-check below observes it via the queue mutex +
            // seq_cst flag). A FAILED work item sets _abandonRemainingItems,
            // which drops the rest of the queue instead of draining it.
            if (_stopProcessingWorkItemsFlag.load() &&
                (_abandonRemainingItems.load() || _workItemQueue.empty()))
            {
                break;
            }

            auto wait_result = _workItemQueue.wait(/*std::chrono::milliseconds(1)*/);
            switch (wait_result)
            {
                case ThreadSafeQueue<TWorkItem>::wait_result::timed_out:
                    break;
                case ThreadSafeQueue<TWorkItem>::wait_result::interrupted:
					break;
                case ThreadSafeQueue<TWorkItem>::wait_result::notified:
					break;
				default:
					break;
            }

            while (!_workItemQueue.empty())
            {
                auto workItem = _workItemQueue.front(false);

                // contain processor exceptions: letting one propagate out of
                // this (thread entry) function would std::terminate the whole
                // process; treat it like a failed work item instead
                bool processed = false;
                try
                {
                    processed = processWorkItem(workItem);
                }
                catch (const std::exception& e)
                {
                    std::lock_guard lock(_lastProcessingErrorMutex);
                    _lastProcessingError = e.what();
                    processed = false;
                }
                catch (...)
                {
                    std::lock_guard lock(_lastProcessingErrorMutex);
                    _lastProcessingError = "unknown non-standard exception";
                    processed = false;
                }

                if (!processed)
                {
                    if (_StopProcessingOnWorkItemFail)
                    {
                        _abandonRemainingItems.store(true);
                        _stopProcessingWorkItemsFlag.store(true);
                        break;
                    }
                }
                _workItemQueue.pop();
            }
        }
    }

    template<typename TWorkItem>
    std::string WorkQueueLoopThread<TWorkItem>::lastProcessingError() const
    {
        std::lock_guard lock(_lastProcessingErrorMutex);
        return _lastProcessingError;
    }

    template<typename TWorkItem>
    void WorkQueueLoopThread<TWorkItem>::startProcessing()
    {
        _stopProcessingWorkItemsFlag.store(false);
        _abandonRemainingItems.store(false);
        // a previous stopProcessing() leaves the queue's interrupt latch set;
        // clear it so this loop's wait() blocks when idle instead of
        // busy-spinning on the stale latch
        _workItemQueue.clearInterrupt();
        //    _processWorkItemsLoopThread = std::make_unique<std::thread>(std::thread([&]
        //                                                                            {
        //                                                                                processWorkItemsLoop();
        //                                                                            }));

        _processWorkItemsLoopThread = std::make_unique<std::thread>(std::thread(&WorkQueueLoopThread::processWorkItemsLoop, this));
    }

    template<typename TWorkItem>
    void WorkQueueLoopThread<TWorkItem>::stopProcessing()
    {
        _stopProcessingWorkItemsFlag.store(true);
        _workItemQueue.interrupt();
        // guarded: stopping before startProcessing() (or stopping twice)
        // must be a clean no-op, not a null/thread-not-joinable crash
        if (_processWorkItemsLoopThread &&
            _processWorkItemsLoopThread->joinable())
        {
            _processWorkItemsLoopThread->join();
        }
    }
}
