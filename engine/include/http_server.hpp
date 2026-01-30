#pragma once 
#include <string>
#include <functional>
#include "order_book.hpp"

namespace trading {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
};

struct HttpResponse {
    int status_code;
    std::string body;
    
    HttpResponse(int code, const std::string& body_)
        : status_code(code), body(body_) {}
};

class HttpServer {
private:
    int port_;
    int server_socket_;
    bool running_;
    OrderBook& order_book_;
    
public:
    HttpServer(int port, OrderBook& book);
    ~HttpServer();
    
    void start();
    void stop();
    
private:
    void handle_connection(int client_socket);
    HttpRequest parse_request(const std::string& request_str);
    HttpResponse route_request(const HttpRequest& request);
    
    HttpResponse handle_place_order(const std::string& body);
    HttpResponse handle_cancel_order(const std::string& body);
    HttpResponse handle_get_orderbook();
    HttpResponse handle_get_stats();
    HttpResponse handle_health_check();
    
    std::string build_response(const HttpResponse& response);
};

}