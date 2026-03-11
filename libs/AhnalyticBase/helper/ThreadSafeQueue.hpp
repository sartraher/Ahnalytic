#ifndef TreadSafeQueue_hpp__
#define TreadSafeQueue_hpp__

#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>

template <typename T>
class ThreadSafeQueue
{
public:
  void push(const T& value)
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      queue_.push(std::move(value));
    }
    cond_.notify_one();
  }

  T wait_and_pop()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] { return !queue_.empty(); });

    T value = std::move(queue_.front());
    queue_.pop();
    return value;
  }

  std::optional<T> try_pop()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty())
      return std::nullopt;

    T value = std::move(queue_.front());
    queue_.pop();
    return value;
  }

  bool empty() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  size_t size() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

private:
  mutable std::mutex mutex_;
  std::queue<T> queue_;
  std::condition_variable cond_;
};

#endif