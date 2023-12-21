// Database.cpp

#include "Database.hpp"
#include <cstring>

namespace Kapua {

Database::Database(const char* host, const char* user, const char* passwd, const char* db, unsigned int port, Logger* logger) {
    
    _logger = new ScopedLogger("Database", logger);

    mysql_init(&mysql);
    if (!mysql_real_connect(&mysql, host, user, passwd, db, port, NULL, 0)) {
        // Handle connection error
        _logger->error("MySQL connection error");
    }
}

Database::~Database() {
    mysql_close(&mysql);
}

int Database::addNode(uint64_t id, sockaddr_in ipv4, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex);

    char query[256];
    char ipv4_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ipv4.sin_addr), ipv4_str, INET_ADDRSTRLEN);

    snprintf(query, sizeof(query), 
             "INSERT INTO node (id, ip4, port, last_registration) VALUES (%llu, '%s', %u, NOW()) ON DUPLICATE KEY UPDATE ip4='%s', port=%u, last_registration=NOW()",
             id, ipv4_str, port, ipv4_str, port);

    if (mysql_query(&mysql, query)) {
        _logger->error("MySQL query error: " + std::string(mysql_error(&mysql)));
        return mysql_errno(&mysql);
    }

    _logger->info("Node added or updated successfully");
    return 0; // Success
}

int Database::getNode(uint64_t id, Node* node) {
    std::lock_guard<std::mutex> lock(mutex);

    char query[128];
    snprintf(query, sizeof(query), "SELECT id, ip4, port FROM node WHERE id = %llu", id);

    if (mysql_query(&mysql, query)) {
        _logger->error("MySQL query error: " + std::string(mysql_error(&mysql)));
        return mysql_errno(&mysql);
    }

    MYSQL_RES *result = mysql_store_result(&mysql);
    if (!result) {
        _logger->error("MySQL result error");
        return mysql_errno(&mysql);
    }

    if (mysql_num_rows(result) == 0) {
        mysql_free_result(result);
        _logger->warn("Node not found");
        return -1; // Node not found
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        _logger->error("Error in fetching row");
        return -1; // Error in fetching row
    }

    node->id = id;
    inet_pton(AF_INET, row[1], &(node->ipv4.sin_addr));
    node->port = static_cast<uint16_t>(atoi(row[2]));

    mysql_free_result(result);
    _logger->debug("Node retrieved successfully");
    return 0; // Success
}

} // namespace Kapua
