#include "RateLimit.hpp"
#include <iostream>
#include <thread>

RateLimiter::RateLimiter()
{
  remaining = 9999;
  resetEpoch = 0;
  minDelayMs = 0;
  lastRequest = std::chrono::steady_clock::now();
}

void RateLimiter::setMinDelayMs(int ms)
{
  std::lock_guard<std::mutex> lock(mutex);
  minDelayMs = ms;
}

void RateLimiter::onResponse(const httplib::Headers& headers)
{
  std::lock_guard<std::mutex> lock(mutex);
  updateRateLimits(headers);

  lastRequest = std::chrono::steady_clock::now();
}

void RateLimiter::updateRateLimits(const httplib::Headers& headers)
{
  remaining = getHeaderLong(headers, "x-ratelimit-remaining", remaining);
  resetEpoch = getHeaderLong(headers, "x-ratelimit-reset", resetEpoch);
}

long RateLimiter::getHeaderLong(const httplib::Headers& headers, const std::string& key, long fallback)
{
  auto it = headers.find(key);
  if (it == headers.end())
    return fallback;

  try
  {
    return std::stol(it->second);
  }
  catch (...)
  {
    return fallback;
  }
}

void RateLimiter::waitIfNeeded()
{
  std::unique_lock<std::mutex> lock(mutex);

  if (minDelayMs > 0)
  {
    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRequest).count();

    if (diff < minDelayMs)
    {
      auto waitMs = minDelayMs - diff;
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
      lock.lock();
    }
  }

  if (remaining < 50)
  {
    lock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    lock.lock();
  }
  if (remaining < 10)
  {
    lock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    lock.lock();
  }

  if (remaining <= 0)
  {
    long nowUnix = (long)time(nullptr);

    if (resetEpoch > nowUnix)
    {
      long sleepSec = resetEpoch - nowUnix;
      std::cout << "[RateLimiter] Ratelimit hit. Sleeping " << sleepSec << " seconds...\n";

      lock.unlock();
      std::this_thread::sleep_for(std::chrono::seconds(sleepSec + 1));
      lock.lock();

      remaining = 9999;
    }
  }
}