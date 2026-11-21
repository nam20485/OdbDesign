#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <sys/resource.h>

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

        long long processCpuMs()
        {
            rusage usage{};
            getrusage(RUSAGE_SELF, &usage);
            return static_cast<long long>(usage.ru_utime.tv_sec) * 1000LL +
                   usage.ru_utime.tv_usec / 1000LL +
                   static_cast<long long>(usage.ru_stime.tv_sec) * 1000LL +
                   usage.ru_stime.tv_usec / 1000LL;
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

        // warm-up proves the loop thread is up and cycling before the race
        // window that this test cares about
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

    // KNOWN DEFECT (asserted via death test because the failure mode is a
    // hard crash, not an exception): stopProcessing() before startProcessing()
    // dereferences the null _processWorkItemsLoopThread unique_ptr
    // (Utils/WorkQueueLoopThread.h:136 ->join()). Once fixed, this test must
    // be inverted to assert a clean no-op stop.
    TEST(WorkQueueLoopThreadDeathTests, StopBeforeStart_CurrentlyCrashes)
    {
        ASSERT_DEATH(
            {
                WorkQueueLoopThread<WorkItem> loop([](WorkItem&) -> bool { return true; });
                loop.stopProcessing(); // never started
            },
            ".*");
    }

    // KNOWN DEFECT (asserted via death test): processWorkItemsLoop() has no
    // try/catch, so a throwing processor propagates out of the thread entry
    // function and calls std::terminate (Utils/WorkQueueLoopThread.h:106).
    TEST(WorkQueueLoopThreadDeathTests, ProcessorException_CurrentlyTerminates)
    {
        ASSERT_DEATH(
            {
                WorkQueueLoopThread<WorkItem> loop([](WorkItem&) -> bool
                {
                    throw std::runtime_error("processor boom");
                });
                loop.addWorkItem(WorkItem{ 1 });
                loop.startProcessing();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            },
            ".*");
    }

    // KNOWN DEFECT (asserted via death test): destroying a WorkQueueLoopThread
    // whose loop is still running destroys a joinable std::thread
    // (unique_ptr<thread> member), which calls std::terminate.
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

    // KNOWN DEFECT TRIPWIRE (currently FAILS): ThreadSafeQueue::interrupt() is
    // sticky (the flag is never cleared), so after stopProcessing() the
    // restarted loop's wait() predicate is instantly true and the idle loop
    // busy-spins (Utils/ThreadSafeQueue.h:177 + Utils/WorkQueueLoopThread.h:88).
    // This test asserts the CORRECT behavior: an idle restarted loop blocks in
    // wait() and burns no CPU.
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
