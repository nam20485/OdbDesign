#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/resource.h>
#endif

#include "WorkQueueLoopThread.h"

using ::Utils::WorkQueueLoopThread;

namespace Odb::Test
{
    namespace
    {
        using steady_clock = std::chrono::steady_clock;

        struct WorkItem
        {
            int value;
        };

        // thread-safe recorder the processor lambda appends into
        class Recorder
        {
        public:
            void record(int value)
            {
                std::lock_guard lock(m_mutex);
                m_values.push_back(value);
            }

            std::vector<int> snapshot() const
            {
                std::lock_guard lock(m_mutex);
                return m_values;
            }

        private:
            mutable std::mutex m_mutex;
            std::vector<int> m_values;
        };

        bool waitFor(const std::function<bool()>& condition, std::chrono::milliseconds timeout)
        {
            const auto deadline = steady_clock::now() + timeout;
            while (steady_clock::now() < deadline)
            {
                if (condition()) return true;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            return condition();
        }

        // total CPU consumed by this process (user + kernel), in ms --
        // getrusage on POSIX, GetProcessTimes on Windows
        long long processCpuMs()
        {
#ifdef _WIN32
            FILETIME createTime{}, exitTime{}, kernelTime{}, userTime{};
            if (!GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime,
                                 &kernelTime, &userTime))
            {
                return 0LL;
            }
            const auto fileTimeToMs = [](const FILETIME& ft)
            {
                ULARGE_INTEGER value;
                value.LowPart = ft.dwLowDateTime;
                value.HighPart = ft.dwHighDateTime;
                return static_cast<long long>(value.QuadPart / 10000LL); // 100ns units
            };
            return fileTimeToMs(kernelTime) + fileTimeToMs(userTime);
#else
            rusage usage{};
            getrusage(RUSAGE_SELF, &usage);
            return static_cast<long long>(usage.ru_utime.tv_sec) * 1000LL +
                   usage.ru_utime.tv_usec / 1000LL +
                   static_cast<long long>(usage.ru_stime.tv_sec) * 1000LL +
                   usage.ru_stime.tv_usec / 1000LL;
#endif
        }
    }

    TEST(WorkQueueLoopThreadTests, Processing_ItemsAddedBeforeStart_ProcessedInOrder)
    {
        Recorder recorder;
        WorkQueueLoopThread<WorkItem> loop([&recorder](WorkItem& item)
        {
            recorder.record(item.value);
            return true;
        });

        loop.addWorkItem(WorkItem{ 1 });
        loop.addWorkItem(WorkItem{ 2 });
        loop.addWorkItem(WorkItem{ 3 });

        loop.startProcessing();
        const bool drained = waitFor([&] { return recorder.snapshot().size() == 3; },
                                     std::chrono::milliseconds(2000));
        loop.stopProcessing();

        ASSERT_TRUE(drained) << "only " << recorder.snapshot().size() << " of 3 items processed";
        EXPECT_EQ(recorder.snapshot(), (std::vector<int>{ 1, 2, 3 }));
    }

    TEST(WorkQueueLoopThreadTests, StopProcessing_PendingItemsAreFlushed)
    {
        Recorder recorder;
        WorkQueueLoopThread<WorkItem> loop([&recorder](WorkItem& item)
        {
            recorder.record(item.value);
            return true;
        });

        // the loop warm-up is a convenience, not a requirement: since the
        // stop-flush root fix, the exit check re-reads emptiness AFTER the
        // stop flag, so a normal stopProcessing() always drains items queued
        // before the stop call (see the zero-warmup stress test below)
        loop.addWorkItem(WorkItem{ 0 });
        loop.startProcessing();
        ASSERT_TRUE(waitFor([&] { return recorder.snapshot().size() == 1; },
                            std::chrono::milliseconds(2000)));

        // queue work, then stop immediately: stopProcessing() interrupts the
        // idle wait and the loop drains what is queued before exiting
        loop.addWorkItem(WorkItem{ 7 });
        loop.addWorkItem(WorkItem{ 8 });
        loop.stopProcessing();

        const auto values = recorder.snapshot();
        ASSERT_EQ(values.size(), 3u);
        EXPECT_EQ(values[1], 7);
        EXPECT_EQ(values[2], 8);
    }

    TEST(WorkQueueLoopThreadTests, RepeatedStartStop_EachCycleProcessesItsWork)
    {
        Recorder recorder;
        WorkQueueLoopThread<WorkItem> loop([&recorder](WorkItem& item)
        {
            recorder.record(item.value);
            return true;
        });

        loop.addWorkItem(WorkItem{ 10 });
        loop.startProcessing();
        ASSERT_TRUE(waitFor([&] { return recorder.snapshot().size() == 1; },
                            std::chrono::milliseconds(2000)));
        loop.stopProcessing();

        loop.addWorkItem(WorkItem{ 20 });
        loop.startProcessing();
        ASSERT_TRUE(waitFor([&] { return recorder.snapshot().size() == 2; },
                            std::chrono::milliseconds(2000)));
        loop.stopProcessing();

        const auto values = recorder.snapshot();
        ASSERT_EQ(values.size(), 2u);
        EXPECT_EQ(values[0], 10);
        EXPECT_EQ(values[1], 20);
    }

    TEST(WorkQueueLoopThreadTests, ProcessorFailure_StopsLoop_DropsRemainingItems)
    {
        // _StopProcessingOnWorkItemFail is const true: a false return stops
        // the loop and abandons the rest of the drained batch
        Recorder recorder;
        WorkQueueLoopThread<WorkItem> loop([&recorder](WorkItem& item)
        {
            recorder.record(item.value);
            return item.value != 0;
        });

        loop.addWorkItem(WorkItem{ 0 });
        loop.addWorkItem(WorkItem{ 1 });
        loop.addWorkItem(WorkItem{ 2 });

        loop.startProcessing();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        loop.stopProcessing(); // thread already exited after the failure; joins

        EXPECT_EQ(recorder.snapshot(), (std::vector<int>{ 0 }));
    }

    // was a hard crash (null _processWorkItemsLoopThread dereference in the
    // unguarded join); stopProcessing() now guards the join, so stopping
    // before startProcessing() (or stopping twice) is a clean no-op
    TEST(WorkQueueLoopThreadTests, StopBeforeStart_IsACleanNoOp)
    {
        Recorder recorder;
        WorkQueueLoopThread<WorkItem> loop([&recorder](WorkItem& item)
        {
            recorder.record(item.value);
            return true;
        });

        loop.stopProcessing(); // never started
        loop.stopProcessing(); // idempotent

        EXPECT_TRUE(recorder.snapshot().empty());
        EXPECT_EQ(loop.lastProcessingError(), "");
    }

    // was std::terminate (exception escaped the thread entry function);
    // processWorkItemsLoop() now contains processor exceptions, stops the
    // loop like a failed work item, and surfaces the error text via
    // lastProcessingError()
    TEST(WorkQueueLoopThreadTests, ProcessorException_Contained_StopsLoopAndSurfacesError)
    {
        Recorder recorder;
        WorkQueueLoopThread<WorkItem> loop([&recorder](WorkItem& item) -> bool
        {
            if (item.value == 0)
            {
                throw std::runtime_error("processor boom");
            }
            recorder.record(item.value);
            return true;
        });

        loop.addWorkItem(WorkItem{ 0 });
        loop.addWorkItem(WorkItem{ 1 });

        loop.startProcessing();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        loop.stopProcessing(); // loop already exited after the contained failure; joins cleanly

        EXPECT_TRUE(recorder.snapshot().empty());
        EXPECT_NE(loop.lastProcessingError().find("processor boom"), std::string::npos);
    }

    TEST(WorkQueueLoopThreadTests, ProcessorException_LastProcessingErrorEmptyWithoutException)
    {
        Recorder recorder;
        WorkQueueLoopThread<WorkItem> loop([&recorder](WorkItem& item)
        {
            recorder.record(item.value);
            return true;
        });

        loop.addWorkItem(WorkItem{ 5 });
        loop.startProcessing();
        ASSERT_TRUE(waitFor([&] { return recorder.snapshot().size() == 1; },
                            std::chrono::milliseconds(2000)));
        loop.stopProcessing();

        EXPECT_EQ(loop.lastProcessingError(), "");
    }

    // STILL a hard-crash path, kept as a death test: destroying a
    // WorkQueueLoopThread whose loop is still running destroys a joinable
    // std::thread (unique_ptr<thread> member), which calls std::terminate.
    // Fixing this would require an implicit-stop destructor -- a lifecycle
    // design change (RAII-style auto-join) that alters destruction semantics
    // for every consumer, beyond a contained fix; left as a documented
    // tripwire so any future RAII change must consciously update it.
    TEST(WorkQueueLoopThreadDeathTests, DestructionWhileRunning_CurrentlyTerminates)
    {
        ASSERT_DEATH(
            {
                WorkQueueLoopThread<WorkItem> loop([](WorkItem&) -> bool
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(300));
                    return true;
                });
                loop.addWorkItem(WorkItem{ 1 });
                loop.startProcessing();
                // scope ends while the loop thread is running
            },
            ".*");
    }

    // stress: the stop can land anywhere in the loop's cycle -- before the
    // loop thread even runs its first exit check, or between an emptiness
    // check and the stop re-read. The flush guarantee must hold with zero
    // synchronization from the test. REGRESSION GUARD for the old racy
    // `while (!stop)` outer-loop exit that could abandon queued items.
    TEST(WorkQueueLoopThreadTests, StopProcessing_FlushesWithoutAnyWarmup_Stress)
    {
        constexpr int kCycles = 60;
        for (int cycle = 0; cycle < kCycles; ++cycle)
        {
            Recorder recorder;
            WorkQueueLoopThread<WorkItem> loop([&recorder](WorkItem& item)
            {
                recorder.record(item.value);
                return true;
            });

            loop.addWorkItem(WorkItem{ 1 });
            loop.addWorkItem(WorkItem{ 2 });
            loop.startProcessing();
            loop.stopProcessing(); // immediately: races the loop startup on purpose

            const auto values = recorder.snapshot();
            ASSERT_EQ(values.size(), 2u) << "cycle " << cycle << " dropped flushed items";
            EXPECT_EQ(loop.lastProcessingError(), "") << "cycle " << cycle;
        }
    }

    // REGRESSION GUARD (was a known defect, fixed in this PR): the queue's
    // interrupt latch used to stay set after stopProcessing(), so a
    // restarted idle loop busy-spinned (~296ms CPU per 300ms window).
    // startProcessing() now clears the latch via ThreadSafeQueue::
    // clearInterrupt(), so an idle restarted loop blocks in wait().
    TEST(WorkQueueLoopThreadTests, RestartedIdleLoop_BurnsNoCpu)
    {
        Recorder recorder;
        WorkQueueLoopThread<WorkItem> loop([&recorder](WorkItem& item)
        {
            recorder.record(item.value);
            return true;
        });

        // full lifecycle, then restart and go idle
        loop.addWorkItem(WorkItem{ 1 });
        loop.startProcessing();
        ASSERT_TRUE(waitFor([&] { return recorder.snapshot().size() == 1; },
                            std::chrono::milliseconds(2000)));
        loop.stopProcessing();

        loop.startProcessing();

        const auto cpuBeforeMs = processCpuMs();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        const auto cpuAfterMs = processCpuMs();

        loop.stopProcessing();

        const auto idleCpuMs = cpuAfterMs - cpuBeforeMs;
        EXPECT_LT(idleCpuMs, 150)
            << "idle restarted loop consumed " << idleCpuMs
            << "ms CPU in a 300ms window (busy-spin)";
    }
} // namespace Odb::Test
