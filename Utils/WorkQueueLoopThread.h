//
// Created by nam20485 on 5/30/22.
//

#pragma once

#include "ThreadSafeQueue.h"
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
        explicit WorkQueueLoopThread(processWorkItemFunc processWorkItemProc);

        void addWorkItem(TWorkItem&& workItem);
        void startProcessing();
        void stopProcessing();

    protected:
        ThreadSafeQueue<TWorkItem> _workItemQueue;
        std::unique_ptr<std::thread> _processWorkItemsLoopThread;
        std::atomic_bool _stopProcessingWorkItemsFlag;
        processWorkItemFunc _processWorkItemProc;

        void processWorkItemsLoop();

        virtual bool processWorkItem(TWorkItem& workItem);

    private:
        const bool _stopProcessingOnWorkItemFail{ true };

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
    WorkQueueLoopThread<TWorkItem>::WorkQueueLoopThread(WorkQueueLoopThread::processWorkItemFunc processWorkItemProc)
        : _stopProcessingWorkItemsFlag(false),
        _processWorkItemProc(processWorkItemProc)
    {
    }

    template<typename TWorkItem>
    void WorkQueueLoopThread<TWorkItem>::addWorkItem(TWorkItem&& workItem)
    {
        _workItemQueue.push(std::move(workItem));
    }

    template<typename TWorkItem>
    void WorkQueueLoopThread<TWorkItem>::processWorkItemsLoop()
    {
        while (!_stopProcessingWorkItemsFlag.load())
        {
            auto wait_result = _workItemQueue.wait(/*std::chrono::milliseconds(1)*/);

            while (!_workItemQueue.empty())
            {
                auto workItem = _workItemQueue.front(false);
                if (!processWorkItem(workItem))
                {
                    if (_stopProcessingOnWorkItemFail)
                    {
                        _stopProcessingWorkItemsFlag.store(true);
                        break;
                    }
                }
                _workItemQueue.pop();
            }
        }
    }

    template<typename TWorkItem>
    void WorkQueueLoopThread<TWorkItem>::startProcessing()
    {
        _stopProcessingWorkItemsFlag.store(false);
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
        _processWorkItemsLoopThread->join();
    }
}
