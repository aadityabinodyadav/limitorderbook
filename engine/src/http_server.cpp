#include "../include/http_server.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>

// --- Cross-Platform Networking Header Logic ---
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    using ssize_t = __int64;
    #define close closesocket
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    typedef int SOCKET;
    #define INVALID_SOCKET -1
#endif

namespace trading {

// Constructor
HttpServer::HttpServer(int port, OrderBook& book)
    : port_(port)
    , server_socket_(-1)
    , running_(false)
    , order_book_(book)
{}

// Destructor
HttpServer::~HttpServer() {
    stop();
}

// Start server
void HttpServer::start() {
#ifdef _WIN32
    // Initialize Winsock for Windows
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return;
    }
#endif

    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    // Set socket options (allow reuse) - Cast to const char* for Windows compatibility
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    // Bind to port
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port_ << std::endl;
        close(server_socket_);
        return;
    }
    
    // Listen for connections
    if (listen(server_socket_, 10) < 0) {
        std::cerr << "Failed to listen" << std::endl;
        close(server_socket_);
        return;
    }
    
    running_ = true;
    std::cout << "🚀 HTTP Server listening on port " << port_ << std::endl;
    
    // Accept loop
    while (running_) {
        struct sockaddr_in client_address;
#ifdef _WIN32
        int client_len = sizeof(client_address);
#else
        socklen_t client_len = sizeof(client_address);
#endif
        
        int client_socket = accept(server_socket_, 
                                   (struct sockaddr*)&client_address, 
                                   &client_len);
        
        if (client_socket < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }
        
        handle_connection(client_socket);
        close(client_socket);
    }
}

// Stop server
void HttpServer::stop() {
    if (running_) {
        running_ = false;
        if (server_socket_ >= 0) {
            close(server_socket_);
            server_socket_ = -1;
        }
#ifdef _WIN32
        WSACleanup();
#endif
        std::cout << "Server stopped" << std::endl;
    }
}

// Handle connection
void HttpServer::handle_connection(int client_socket) {
    char buffer[4096] = {0};
    // Use recv instead of read for cross-platform support
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        return;
    }
    
    std::string request_str(buffer, bytes_read);
    HttpRequest request = parse_request(request_str);
    HttpResponse response = route_request(request);
    std::string response_str = build_response(response);
    
    // Use send instead of write for cross-platform support
    send(client_socket, response_str.c_str(), (int)response_str.length(), 0);
}

// Parse HTTP request
HttpRequest HttpServer::parse_request(const std::string& request_str) {
    HttpRequest request;
    std::istringstream stream(request_str);
    std::string line;
    
    if (std::getline(stream, line)) {
        std::istringstream line_stream(line);
        line_stream >> request.method >> request.path;
    }
    
    bool found_empty_line = false;
    while (std::getline(stream, line)) {
        if (line == "\r" || line.empty()) {
            found_empty_line = true;
            break;
        }
    }
    
    if (found_empty_line) {
        std::string body_line;
        while (std::getline(stream, body_line)) {
            request.body += body_line;
        }
    }
    
    return request;
}

// Route request
HttpResponse HttpServer::route_request(const HttpRequest& request) {
    std::cout << "[" << request.method << "] " << request.path << std::endl;
    
    if (request.path == "/health") {
        return handle_health_check();
    }
    else if (request.path == "/order" && request.method == "POST") {
        return handle_place_order(request.body);
    }
    else if (request.path == "/order" && request.method == "DELETE") {
        return handle_cancel_order(request.body);
    }
    else if (request.path == "/orderbook" && request.method == "GET") {
        return handle_get_orderbook();
    }
    else if (request.path == "/stats" && request.method == "GET") {
        return handle_get_stats();
    }
    else {
        return HttpResponse(404, "{\"error\":\"Not found\"}");
    }
}

// Handle place order
HttpResponse HttpServer::handle_place_order(const std::string& body) {
    try {
        size_t price_pos = body.find("\"price\":");
        if (price_pos == std::string::npos) return HttpResponse(400, "{\"error\":\"Missing price\"}");
        price_pos += 8;
        Price price = std::stoul(body.substr(price_pos));
        
        size_t qty_pos = body.find("\"quantity\":");
        if (qty_pos == std::string::npos) return HttpResponse(400, "{\"error\":\"Missing quantity\"}");
        qty_pos += 11;
        Quantity quantity = std::stoul(body.substr(qty_pos));
        
        size_t side_pos = body.find("\"side\":\"");
        if (side_pos == std::string::npos) return HttpResponse(400, "{\"error\":\"Missing side\"}");
        side_pos += 8;
        std::string side_str = body.substr(side_pos, 3);
        Side side = (side_str == "BUY") ? Side::BUY : Side::SELL;
        
        auto trades = order_book_.add_order(price, quantity, side);
        
        std::ostringstream response;
        response << "{\"status\":\"success\",\"trades\":[";
        for (size_t i = 0; i < trades.size(); ++i) {
            const auto& trade = trades[i];
            response << "{\"buyer_id\":" << trade.buyer_id 
                     << ",\"seller_id\":" << trade.seller_id 
                     << ",\"price\":" << trade.price 
                     << ",\"quantity\":" << trade.quantity << "}";
            if (i < trades.size() - 1) response << ",";
        }
        response << "]}";
        
        return HttpResponse(200, response.str());
        
    } catch (...) {
        return HttpResponse(400, "{\"error\":\"Invalid request data\"}");
    }
}

// Handle cancel order
HttpResponse HttpServer::handle_cancel_order(const std::string& body) {
    try {
        size_t id_pos = body.find("\"order_id\":");
        if (id_pos == std::string::npos) return HttpResponse(400, "{\"error\":\"Missing order_id\"}");
        id_pos += 11;
        OrderId order_id = std::stoull(body.substr(id_pos));
        
        if (order_book_.cancel_order(order_id)) {
            return HttpResponse(200, "{\"status\":\"cancelled\"}");
        } else {
            return HttpResponse(404, "{\"error\":\"Order not found\"}");
        }
    } catch (...) {
        return HttpResponse(400, "{\"error\":\"Invalid request\"}");
    }
}

HttpResponse HttpServer::handle_get_orderbook() {
    std::ostringstream response;
    response << "{\"best_bid\":" << order_book_.get_best_bid() 
             << ",\"best_ask\":" << order_book_.get_best_ask() 
             << ",\"spread\":" << order_book_.get_spread() 
             << ",\"order_count\":" << order_book_.get_order_count() << "}";
    return HttpResponse(200, response.str());
}

HttpResponse HttpServer::handle_get_stats() {
    std::ostringstream response;
    response << "{\"total_orders\":" << order_book_.get_order_count() 
             << ",\"bid_levels\":" << order_book_.get_bid_level_count() 
             << ",\"ask_levels\":" << order_book_.get_ask_level_count() 
             << ",\"best_bid\":" << order_book_.get_best_bid() 
             << ",\"best_ask\":" << order_book_.get_best_ask() 
             << ",\"spread\":" << order_book_.get_spread() << "}";
    return HttpResponse(200, response.str());
}

HttpResponse HttpServer::handle_health_check() {
    return HttpResponse(200, "{\"status\":\"ok\"}");
}

// Build HTTP response
std::string HttpServer::build_response(const HttpResponse& response) {
    std::ostringstream stream;
    stream << "HTTP/1.1 " << response.status_code << " ";
    if (response.status_code == 200) stream << "OK";
    else if (response.status_code == 400) stream << "Bad Request";
    else if (response.status_code == 404) stream << "Not Found";
    else stream << "Unknown";
    
    stream << "\r\nContent-Type: application/json\r\n"
           << "Content-Length: " << response.body.length() << "\r\n"
           << "Access-Control-Allow-Origin: *\r\n"
           << "Connection: close\r\n\r\n"
           << response.body;
    
    return stream.str();
}

} // namespace trading