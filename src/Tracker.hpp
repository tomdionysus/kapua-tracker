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
#include <cstdint>
#include <iostream>
#include <memory>

#include "Logger.hpp"

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
};

};  // namespace Kapua