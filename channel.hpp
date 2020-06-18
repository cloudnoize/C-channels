#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#pragma once

// Buffered channel, can push until capacity is full, then block
template <typename T, unsigned int CAP> class Channel {
private:
  std::queue<T> data_queue_;
  mutable std::mutex mut;
  std::condition_variable data_cond;

public:
  void push(T new_value) {
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk, [this] { return data_queue_.size() < CAP; });
    data_queue_.push(std::move(new_value));
    data_cond.notify_all();
  }

  T pop() {
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk, [this] { return !data_queue_.empty(); });
    T value = std::move(data_queue_.front());
    data_queue_.pop();
    data_cond.notify_all();
    return value;
  }
};

// Zero capacity, in order for the pushing thread to continue its value must be consumed.
template <typename T> class Channel<T, 0> {
private:
  T item_;
  bool have_value{false};
  mutable std::mutex mut;
  std::condition_variable data_cond;

  mutable std::mutex release_mut_;
  std::condition_variable release_cond_;
  std::atomic_bool threadWaiting_{false};

public:
  // Don't allow pusher to get in while another thread is waiting for its value to be consumed.
  void push(T new_value) {
    {
      std::unique_lock<std::mutex> lk(mut);
      data_cond.wait(lk, [this] { return !have_value && !threadWaiting_; });
      threadWaiting_ = true;
      item_ = std::move(new_value);
      have_value = true;
    }
    // Motify all, since many waits and they can be of type push or pop, and we need to notify poppers.
    data_cond.notify_all();
    {
      std::unique_lock<std::mutex> rk(release_mut_);
      release_cond_.wait(rk, [this] { return !have_value; });
    }
    threadWaiting_ = false;
    // Same reason as the above but now we need to notify pushers
    data_cond.notify_all();
  }

  T pop() {
    T value;
    {
      std::unique_lock<std::mutex> lk(mut);
      data_cond.wait(lk, [this] { return have_value; });
      value = std::move(item_);
      have_value = false;
    }
    // only one push thread waits
    release_cond_.notify_one();
    return value;
  }
};