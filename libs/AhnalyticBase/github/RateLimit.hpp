#pragma once

#include "AhnalyticBase/Export.hpp"

#include <chrono>
#include <mutex>
#include <string>

#define CPPHTTPLIB_NO_MULTIPART_FORM_DATA
#define CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_ZLIB_SUPPORT
#include <httplib.h>

class DLLEXPORT RateLimiter
{
public:
  RateLimiter();

  // Call AFTER every GitHub HTTP request (provide response headers)
  void onResponse(const httplib::Headers& headers);

  // Call BEFORE every new request
  void waitIfNeeded();

  // Optional: force minimum delay between requests (ms)
  void setMinDelayMs(int ms);

private:
  void updateRateLimits(const httplib::Headers& headers);
  long getHeaderLong(const httplib::Headers& headers, const std::string& key, long fallback);

  std::mutex mutex;

  long remaining;  // X-RateLimit-Remaining
  long resetEpoch; // X-RateLimit-Reset (UTC epoch sec)
  int minDelayMs;  // constant delay between requests

  std::chrono::steady_clock::time_point lastRequest;
};