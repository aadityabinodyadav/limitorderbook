#pragma once 
#include <string>
#include <functional>
#include "order_book.hpp"

namespace trading {

struct HttpRequest {
    std::string method;      // GET, POST, etc.
    std::string path;        // /order, /orderbook, etc.
    std::string body;        // Request body (JSON)
};

// Simple HTTP response structure
struct HttpResponse {
    int status_code;         // 200, 404, etc.
    std::string body;        // Response body (JSON)
    
    HttpResponse(int code, const std::string& body_)
        : status_code(code), body(body_) {}
};

// HTTP Server class
class HttpServer {
private:
    int port_;
    int server_socket_;
    bool running_;
    OrderBook& order_book_;
    
public:
    // Constructor
    HttpServer(int port, OrderBook& book);
    
    // Destructor
    ~HttpServer();
    
    // Start the server
    void start();
    
    // Stop the server
    void stop();
    
private:
    // Handle incoming connection
    void handle_connection(int client_socket);
    
    // Parse HTTP request
    HttpRequest parse_request(const std::string& request_str);
    
    // Route request to appropriate handler
    HttpResponse route_request(const HttpRequest& request);
    
    // API Handlers
    HttpResponse handle_place_order(const std::string& body);
    HttpResponse handle_cancel_order(const std::string& body);
    HttpResponse handle_get_orderbook();
    HttpResponse handle_get_stats();
    HttpResponse handle_health_check();
    
    // Helper: Build HTTP response string
    std::string build_response(const HttpResponse& response);
};

} 