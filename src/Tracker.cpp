//
// Kapua Tracker class
//
// Author: Tom Cully <mail@tomcully.com>
// Copyright (c) Tom Cully 2023
//
#include "Tracker.hpp"

using boost::asio::ip::tcp;

using namespace std;

namespace Kapua {
#include <iostream>
#include <memory>

#include "Tracker.hpp"

Tracker::Tracker(Logger* logger, uint16_t port) : _io_context(), _acceptor(_io_context) {
  _logger = new ScopedLogger("HTTP", logger);
  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
  _acceptor.open(endpoint.protocol());
  _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  _acceptor.bind(endpoint);
  _acceptor.listen();
}

Tracker::~Tracker() { delete _logger; }

void Tracker::start() {
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        _running = true;
    }

    _thread = boost::thread([this]() {
        start_accept();
        _io_context.run();
    });
}

void Tracker::stop() {
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        if (!_running) return;
        _running = false;
    }

    _io_context.stop();
    _thread.join();  // Wait for the thread to finish
}
void Tracker::start_accept() {
  auto socket = std::make_shared<boost::asio::ip::tcp::socket>(_io_context);
  _acceptor.async_accept(*socket, [this, socket](const boost::system::error_code& error) {
    if (!error) {
      handle_request(socket);
    } else {
      _logger->error("Accept failed: " + error.message());
    }
    start_accept();
  });
}

void Tracker::handle_request(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
  // Beast's buffer for holding incoming data
  auto buffer = std::make_shared<boost::beast::flat_buffer>();

  // Beast's HTTP request object
  auto req = std::make_shared<boost::beast::http::request<boost::beast::http::string_body>>();

  // Read the request
  boost::beast::http::async_read(*socket, *buffer, *req, [this, socket, buffer, req](boost::beast::error_code ec, std::size_t) {
    if (!ec) {
      handle_http_request(*req, socket);
    } else {
      _logger->error("Read failed: " + ec.message());
    }
  });
}

void Tracker::handle_http_request(const boost::beast::http::request<boost::beast::http::string_body>& req,
                                  std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
  boost::asio::ip::tcp::endpoint remote_ep = socket->remote_endpoint();
  boost::asio::ip::address remote_ad = remote_ep.address();
  std::string ip = remote_ad.to_string();
  unsigned short port = remote_ep.port();

  if (!check_rate_limit(ip)) {
    _logger->debug(ip + " Exceeded rate limit");

    auto res = std::make_shared<boost::beast::http::response<boost::beast::http::string_body>>(boost::beast::http::status::too_many_requests, req.version());

    res->set(boost::beast::http::field::server, KAPUA_SERVER_STRING);
    res->set(boost::beast::http::field::content_type, "text/plain");
    res->set(boost::beast::http::field::retry_after, "60");  // Retry after 60 seconds
    res->keep_alive(req.keep_alive());
    res->body() = "";
    res->content_length(0);

    boost::beast::http::async_write(*socket, *res, [this, socket, res](boost::beast::error_code ec, std::size_t) {
      if (ec) {
        _logger->error("Write failed: " + ec.message());
      }
    });
    return;
  }

  // Access the method, headers, and body from the request
  std::string method = req.method_string();
  const auto& headers = req.base();
  std::string body = req.body();

  _logger->info(ip + ":" + std::to_string(port) + " " + method + " " + std::string(req.target()));

  // Create the response object on the heap to manage its lifetime
  auto res = std::make_shared<boost::beast::http::response<boost::beast::http::string_body>>(boost::beast::http::status::ok, req.version());

  res->set(boost::beast::http::field::server, KAPUA_SERVER_STRING);
  res->set(boost::beast::http::field::content_type, "text/plain");
  res->keep_alive(req.keep_alive());
  res->body() = "";
  res->content_length(0);

  boost::beast::http::async_write(*socket, *res, [this, socket, res](boost::beast::error_code ec, std::size_t) {
    if (ec) {
      _logger->error("Write failed: " + ec.message());
    }
  });
}

bool Tracker::check_rate_limit(const std::string& ip) {
  auto now = std::chrono::steady_clock::now();
  auto& info = rate_limit_map[ip];

  if (now - info.last_request > rate_limit_interval) {
    info.count = 0;
    info.last_request = now;
  }

  if (++info.count > rate_limit_threshold) {
    return false;  // Rate limit exceeded
  }

  return true;  // Within rate limit
}

void Tracker::cleanup_rate_limits() {
  auto now = std::chrono::steady_clock::now();
  for (auto it = rate_limit_map.begin(); it != rate_limit_map.end();) {
    if (now - it->second.last_request > rate_limit_interval) {
      it = rate_limit_map.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace Kapua