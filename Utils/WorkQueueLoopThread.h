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
        explicit WorkQueueLoopThread(processWorkItemFunc processWorkItemProc/*, bool enableInternalLogging*/);

        void addWorkItem(TWorkItem&& workItem);
        //void addWorkItem(TWorkItem& workItem);
        void startProcessing();
        void stopProcessing();

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
        while (!_stopProcessingWorkItemsFlag.load())
        {
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
                if (!processWorkItem(workItem))
                {
                    if (_StopProcessingOnWorkItemFail)
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
