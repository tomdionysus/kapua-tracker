//
// Kapua Tracker class
//
// Author: Tom Cully <mail@tomcully.com>
// Copyright (c) Tom Cully 2023
//
#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>

#include "Logger.hpp"

#define KAPUA_SERVER_STRING "kapua-tracker v0.0.1"

namespace Kapua {

using boost::asio::ip::tcp;

class Tracker {
 public:
  Tracker(Logger* logger, uint16_t port);
  ~Tracker();
  void start();
  void stop();

 private:
  void start_accept();
  void handle_request(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
  void handle_http_request(const boost::beast::http::request<boost::beast::http::string_body>& req, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
  boost::asio::io_context _io_context;
  boost::asio::ip::tcp::acceptor _acceptor;
  Logger* _logger;

  void write_async_response(const boost::beast::http::request<boost::beast::http::string_body>& req,
                                   std::shared_ptr<boost::asio::ip::tcp::socket> socket, boost::beast::http::status status, std::string contentType,
                                   std::string body, std::vector<std::tuple<boost::beast::http::field, std::string>>* headers);

  // void handle_http_json_request(const boost::beast::http::request<boost::beast::http::string_body>& req, std::shared_ptr<boost::asio::ip::tcp::socket> socket);

  // Rate Limiting
  struct RateLimitInfo {
    int count;
    std::chrono::steady_clock::time_point last_request;
  };

  std::map<std::string, RateLimitInfo> rate_limit_map;
  const int rate_limit_threshold = 100;                                       // Example limit
  const std::chrono::seconds rate_limit_interval = std::chrono::seconds(60);  // Reset interval

  bool check_rate_limit(const std::string& ip);
  void cleanup_rate_limits();

  // Threading
  boost::thread _thread;
  boost::mutex _mutex;
  boost::condition_variable _cond;
  bool _running;
};

};  // namespace Kapua