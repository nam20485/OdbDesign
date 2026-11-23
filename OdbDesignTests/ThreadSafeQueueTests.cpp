#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "ThreadSafeQueue.h"

using ::Utils::ThreadSafeQueue;

namespace Odb::Test
{
    namespace
    {
        using steady_clock = std::chrono::steady_clock;
    }

    TEST(ThreadSafeQueueTests, PushFrontPop_FifoOrderMultipleItems)
    {
        ThreadSafeQueue<int> queue;
        queue.push(1);
        queue.push(2);
        queue.push(3);

        EXPECT_FALSE(queue.empty());
        EXPECT_EQ(queue.front(false), 1);
        queue.pop();
        EXPECT_EQ(queue.front(false), 2);
        queue.pop();
        EXPECT_EQ(queue.front(false), 3);
        queue.pop();
        EXPECT_TRUE(queue.empty());
    }

    TEST(ThreadSafeQueueTests, Empty_TrueInitially)
    {
        ThreadSafeQueue<int> queue;
        EXPECT_TRUE(queue.empty());
    }

    TEST(ThreadSafeQueueTests, Front_NonBlocking_DoesNotRemoveItem)
    {
        ThreadSafeQueue<std::string> queue;
        queue.push("head");
        EXPECT_EQ(queue.front(false), "head");
        EXPECT_EQ(queue.front(false), "head"); // still there
        EXPECT_FALSE(queue.empty());
        queue.pop();
        EXPECT_TRUE(queue.empty());
    }

    TEST(ThreadSafeQueueTests, WaitTimed_EmptyQueue_TimesOut)
    {
        ThreadSafeQueue<int> queue;
        const auto start = steady_clock::now();
        auto result = queue.wait(std::chrono::milliseconds(80));
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            steady_clock::now() - start);

        EXPECT_EQ(result, ThreadSafeQueue<int>::wait_result::timed_out);
        EXPECT_GE(elapsed.count(), 70);
    }

    TEST(ThreadSafeQueueTests, WaitTimed_ItemAlreadyPresent_ReturnsNotifiedImmediately)
    {
        ThreadSafeQueue<int> queue;
        queue.push(7);
        const auto start = steady_clock::now();
        auto result = queue.wait(std::chrono::milliseconds(5000));
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            steady_clock::now() - start);

        EXPECT_EQ(result, ThreadSafeQueue<int>::wait_result::notified);
        EXPECT_LT(elapsed.count(), 1000);
        EXPECT_FALSE(queue.empty());
    }

    TEST(ThreadSafeQueueTests, Wait_BlockedThenPush_ReturnsNotified)
    {
        ThreadSafeQueue<int> queue;
        std::atomic<int> result{ -1 };
        std::thread waiter([&]
        {
            result.store(static_cast<int>(queue.wait()));
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let the waiter block
        queue.push(42);
        waiter.join();

        EXPECT_EQ(result.load(), static_cast<int>(ThreadSafeQueue<int>::wait_result::notified));
        EXPECT_EQ(queue.front(false), 42);
    }

    TEST(ThreadSafeQueueTests, Wait_BlockedThenInterrupt_ReturnsInterrupted)
    {
        ThreadSafeQueue<int> queue;
        std::atomic<int> result{ -1 };
        std::thread waiter([&]
        {
            result.store(static_cast<int>(queue.wait()));
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let the waiter block
        queue.interrupt();
        waiter.join();

        EXPECT_EQ(result.load(), static_cast<int>(ThreadSafeQueue<int>::wait_result::interrupted));
        EXPECT_TRUE(queue.empty());
    }

    TEST(ThreadSafeQueueTests, FrontWait_BlockedThread_ReceivesPushedItem)
    {
        ThreadSafeQueue<int> queue;
        std::atomic<int> received{ -1 };
        std::thread waiter([&]
        {
            auto& item = queue.front(true);
            received.store(item);
            queue.pop();
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let the waiter block
        queue.push(99);
        waiter.join();

        EXPECT_EQ(received.load(), 99);
        EXPECT_TRUE(queue.empty());
    }

    TEST(ThreadSafeQueueTests, Interrupt_Sticky_EveryLaterWaitReturnsInterruptedImmediately)
    {
        ThreadSafeQueue<int> queue;
        queue.interrupt();

        // documented actual behavior: interrupt() is never cleared, so every
        // subsequent wait returns interrupted instantly even on an empty queue
        EXPECT_EQ(queue.wait(), ThreadSafeQueue<int>::wait_result::interrupted);
        EXPECT_EQ(queue.wait(std::chrono::milliseconds(50)),
                  ThreadSafeQueue<int>::wait_result::interrupted);
    }

    TEST(ThreadSafeQueueTests, Interrupt_TakesPrecedenceOverPushedItem)
    {
        ThreadSafeQueue<int> queue;
        queue.push(1);
        queue.interrupt();

        // documented actual behavior: with both a queued item and a tripped
        // interrupt, the interrupt result wins
        EXPECT_EQ(queue.wait(), ThreadSafeQueue<int>::wait_result::interrupted);
        EXPECT_FALSE(queue.empty()); // item is still queued
    }

    TEST(ThreadSafeQueueTests, ClearInterrupt_LetsLaterWaitsBlockAgain)
    {
        ThreadSafeQueue<int> queue;
        queue.interrupt();
        ASSERT_EQ(queue.wait(), ThreadSafeQueue<int>::wait_result::interrupted);

        // clearInterrupt() resets the latch: an empty-queue wait must BLOCK
        // (time out) again instead of returning interrupted instantly
        queue.clearInterrupt();
        const auto start = steady_clock::now();
        EXPECT_EQ(queue.wait(std::chrono::milliseconds(80)),
                  ThreadSafeQueue<int>::wait_result::timed_out);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            steady_clock::now() - start);
        EXPECT_GE(elapsed.count(), 70);

        // and a later interrupt still works after a clear
        std::atomic<int> result{ -1 };
        std::thread waiter([&]
        {
            result.store(static_cast<int>(queue.wait()));
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        queue.interrupt();
        waiter.join();
        EXPECT_EQ(result.load(), static_cast<int>(ThreadSafeQueue<int>::wait_result::interrupted));
    }

    TEST(ThreadSafeQueueTests, MultipleWaiters_EachInterruptWakesOneWaiter)
    {
        ThreadSafeQueue<int> queue;
        std::atomic<int> interruptedCount{ 0 };
        std::atomic<int> timedOutCount{ 0 };

        std::vector<std::thread> waiters;
        for (int i = 0; i < 3; ++i)
        {
            waiters.emplace_back([&]
            {
                auto result = queue.wait(std::chrono::milliseconds(5000));
                if (result == ThreadSafeQueue<int>::wait_result::interrupted)
                {
                    interruptedCount.fetch_add(1);
                }
                else if (result == ThreadSafeQueue<int>::wait_result::timed_out)
                {
                    timedOutCount.fetch_add(1);
                }
            });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(150)); // let all three block
        // notify_one semantics: exactly one waiter is released per interrupt
        queue.interrupt();
        queue.interrupt();
        queue.interrupt();

        for (auto& waiter : waiters)
        {
            waiter.join();
        }

        EXPECT_EQ(interruptedCount.load(), 3);
        EXPECT_EQ(timedOutCount.load(), 0);
    }

    TEST(ThreadSafeQueueTests, PushWhileOtherThreadWaits_WaiterConsumesItem)
    {
        // producer/consumer smoke: a blocked timed waiter is woken by a push
        // and consumes exactly the pushed item
        ThreadSafeQueue<std::string> queue;
        std::atomic<bool> consumed{ false };
        std::string consumedValue;

        std::thread consumer([&]
        {
            if (queue.wait(std::chrono::milliseconds(5000)) ==
                ThreadSafeQueue<std::string>::wait_result::notified)
            {
                consumedValue = queue.front(false);
                queue.pop();
                consumed.store(true);
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        queue.push("payload");
        consumer.join();

        EXPECT_TRUE(consumed.load());
        EXPECT_EQ(consumedValue, "payload");
        EXPECT_TRUE(queue.empty());
    }
} // namespace Odb::Test
