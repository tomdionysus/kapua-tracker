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

    const char *query = "INSERT INTO node (id, ip4, port, last_registration) VALUES (?, ?, ?, NOW()) ON DUPLICATE KEY UPDATE ip4=?, port=?, last_registration=NOW()";

    MYSQL_STMT *stmt = mysql_stmt_init(&mysql);
    if (!stmt) {
        _logger->error("MySQL stmt init error");
        return -1; // Or appropriate error code
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        _logger->error("MySQL stmt prepare error: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        return -1; // Or appropriate error code
    }

    MYSQL_BIND bind[5];
    memset(bind, 0, sizeof(bind));

    char ipv4_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ipv4.sin_addr), ipv4_str, INET_ADDRSTRLEN);

    // Bind parameters
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = (char *)&id;
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = ipv4_str;
    bind[1].buffer_length = strlen(ipv4_str);
    bind[2].buffer_type = MYSQL_TYPE_SHORT;
    bind[2].buffer = (char *)&port;
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = ipv4_str;
    bind[3].buffer_length = strlen(ipv4_str);
    bind[4].buffer_type = MYSQL_TYPE_SHORT;
    bind[4].buffer = (char *)&port;

    if (mysql_stmt_bind_param(stmt, bind)) {
        _logger->error("MySQL stmt bind param error: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        return -1; // Or appropriate error code
    }

    if (mysql_stmt_execute(stmt)) {
        _logger->error("MySQL stmt execute error: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        return -1; // Or appropriate error code
    }

    _logger->info("Node added or updated successfully");
    mysql_stmt_close(stmt);
    return 0; // Success
}

int Database::getNode(uint64_t id, Node* node) {
    std::lock_guard<std::mutex> lock(mutex);

    const char *query = "SELECT id, ip4, port FROM node WHERE id = ?";

    MYSQL_STMT *stmt = mysql_stmt_init(&mysql);
    if (!stmt) {
        _logger->error("MySQL stmt init error");
        return -1;
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        _logger->error("MySQL stmt prepare error: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        return -1;
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));

    // Bind parameter
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = (char *)&id;

    if (mysql_stmt_bind_param(stmt, bind)) {
        _logger->error("MySQL stmt bind param error: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        return -1;
    }

    if (mysql_stmt_execute(stmt)) {
        _logger->error("MySQL stmt execute error: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        return -1;
    }

    MYSQL_BIND bind_out[3];
    memset(bind_out, 0, sizeof(bind_out));
    uint64_t out_id;
    char out_ip4[INET_ADDRSTRLEN];
    uint16_t out_port;

    bind_out[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind_out[0].buffer = (char *)&out_id;
    bind_out[1].buffer_type = MYSQL_TYPE_STRING;
    bind_out[1].buffer = out_ip4;
    bind_out[1].buffer_length = INET_ADDRSTRLEN;
    bind_out[2].buffer_type = MYSQL_TYPE_SHORT;
    bind_out[2].buffer = (char *)&out_port;

    if (mysql_stmt_bind_result(stmt, bind_out)) {
        _logger->error("MySQL stmt bind result error: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        return -1;
    }

    if (mysql_stmt_store_result(stmt)) {
        _logger->error("MySQL stmt store result error: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        return -1;
    }

    if (mysql_stmt_num_rows(stmt) == 0) {
        _logger->warn("Node not found");
        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);
        return -1; // Node not found
    }

    if (mysql_stmt_fetch(stmt)) {
        _logger->error("MySQL stmt fetch error: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);
        return -1; // Error in fetching row
    }

    node->id = out_id;
    inet_pton(AF_INET, out_ip4, &(node->ipv4.sin_addr));
    node->port = out_port;

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    _logger->debug("Node retrieved successfully");
    return 0; // Success
}


} // namespace Kapua
