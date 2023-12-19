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
  start_accept();
  _io_context.run();
}

void Tracker::stop() {
  _acceptor.close();
  _io_context.stop();
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

  // Access the method, headers, and body from the request
  std::string method = req.method_string();
  const auto& headers = req.base();
  std::string body = req.body();

  _logger->info(ip + ":" + std::to_string(port) + " " + method + " " + std::string(req.target()));

  // Create the response object on the heap to manage its lifetime
  auto res = std::make_shared<boost::beast::http::response<boost::beast::http::string_body>>(boost::beast::http::status::ok, req.version());

  res->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  res->set(boost::beast::http::field::content_type, "text/plain");
  res->content_length(body.size());
  res->keep_alive(req.keep_alive());
  res->body() = "";

  boost::beast::http::async_write(*socket, *res, [this, socket, res](boost::beast::error_code ec, std::size_t) {
    if (ec) {
      _logger->error("Write failed: " + ec.message());
    }
  });
}

}  // namespace Kapua