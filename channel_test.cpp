#include <channel.hpp>

#include "gtest/gtest.h"
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

TEST(channle_0, basic) {
  Channel<int, 0> c;
  int res = 400;
  auto pusher = [&]() { c.push(1); };
  auto reciever = [&]() {
    res = c.pop();
    EXPECT_EQ(1, res);
  };
  std::thread t1(pusher);
  std::thread t2(reciever);
  t1.join();
  t2.join();
}

TEST(channle_0, blocking) {
  Channel<int, 0> c;
  int res = 400;
  int sleepForMs = 1345;
  auto pusher = [&]() {
    auto start = std::chrono::steady_clock::now();
    c.push(1);
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    EXPECT_GE(duration, sleepForMs);
  };
  auto reciever = [&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepForMs));
    res = c.pop();
  };

  std::thread t1(pusher);
  std::thread t2(reciever);
  t1.join();
  t2.join();
}

// Test that multi push is synched
TEST(channle_0, multithreads) {
  Channel<int, 0> c;
  int res = 400;
  int sleepForMs = 1345;
  int sleep2 = 500;
  auto pusher = [&]() {
    auto start = std::chrono::steady_clock::now();
    c.push(1);
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    EXPECT_GE(duration, sleepForMs);
  };

  auto pusher2 = [&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto start = std::chrono::steady_clock::now();
    c.push(100);
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    EXPECT_GE(duration, sleepForMs + sleep2 - 100);
  };
  auto reciever = [&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepForMs));
    res = c.pop();
    EXPECT_EQ(res, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep2));
    res = c.pop();
    EXPECT_EQ(res, 100);
  };

  std::thread t1(pusher);
  std::thread t2(reciever);
  std::thread t3(pusher2);
  t1.join();
  t2.join();
  t3.join();
}

// Check that can push without blocking.
TEST(channl_bufferd, no_block) {
  Channel<int, 5> c;
  std::vector<std::thread> threads;
  c.push(1);
  for (int i : {2, 3, 4, 5}) {
    auto pusher = [&]() { c.push(i); };
    std::thread t(pusher);
    threads.push_back(std::move(t));
  }
  for (auto &t : threads) {
    t.join();
  }
}

// Check that can push without blocking.
TEST(channl_bufferd, order) {
  Channel<int, 5> c;
  std::vector<std::thread> threads;
  c.push(1);
  c.push(2);
  c.push(3);
  c.push(4);
  EXPECT_EQ(1, c.pop());
  EXPECT_EQ(2, c.pop());
  EXPECT_EQ(3, c.pop());
  EXPECT_EQ(4, c.pop());
}

// Check that can push without blocking.
TEST(channl_bufferd, block) {
  Channel<int, 5> c;
  std::vector<std::thread> threads;
  c.push(1);
  for (int i : {2, 3, 4, 5, 6}) {
    auto pusher = [&]() { c.push(i); };
    std::thread t(pusher);
    threads.push_back(std::move(t));
  }
  bool before{true};
  auto pusher = [&]() {
    c.push(6);
    before = false;
  };
  std::thread t(pusher);
  threads.push_back(std::move(t));

  EXPECT_EQ(before, true);
  for (int i : {1}) {
    c.pop();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(before, true);
  c.pop();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(before, false);

  for (auto &t : threads) {
    t.join();
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}