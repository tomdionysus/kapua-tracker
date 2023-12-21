//
// Kapua Database class
//
// Author: Tom Cully <mail@tomcully.com>
// Copyright (c) Tom Cully 2023
//
#pragma once

#include <mysql/mysql.h>
#include <mutex>
#include <cstdint>
#include <arpa/inet.h>

#include "Logger.hpp"

namespace Kapua {

struct Node {
    uint64_t id;
    sockaddr_in ipv4;
    uint16_t port;
};

class Database {
public:
    Database(const char* host, const char* user, const char* passwd, const char* db, unsigned int port, Logger* logger);
    ~Database();

    int addNode(uint64_t id, sockaddr_in ipv4, uint16_t port);
    int getNode(uint64_t id, Node* node);

private:
    MYSQL mysql;
    std::mutex mutex;
    Logger* _logger;
};

} // namespace Kapua
