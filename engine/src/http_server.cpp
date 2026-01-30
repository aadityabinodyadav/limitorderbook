#include "../include/http_server.hpp"
#include "../include/nlohmann/json.hpp"
#include "../include/order.hpp" 
#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cmath> 


#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    #define close closesocket
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

using json = nlohmann::json;

namespace trading {

HttpServer::HttpServer(int port, OrderBook& book)
    : port_(port), server_socket_(-1), running_(false), order_book_(book) {}

HttpServer::~HttpServer() { stop(); }

void HttpServer::start() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return;
    }
    
    listen(server_socket_, 10);
    running_ = true;
    std::cout << "🚀 Server listening on port " << port_ << std::endl;
    
    while (running_) {
        struct sockaddr_in client_addr;
#ifdef _WIN32
        int addr_len = sizeof(client_addr);
#else
        socklen_t addr_len = sizeof(client_addr);
#endif
        int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket >= 0) {
            handle_connection(client_socket);
            close(client_socket);
        }
    }
}

void HttpServer::stop() {
    if (running_) {
        running_ = false;
        close(server_socket_);
#ifdef _WIN32
        WSACleanup();
#endif
    }
}

void HttpServer::handle_connection(int client_socket) {
    char buffer[1024 * 4] = {0};
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) return;

    std::string request_raw(buffer, bytes_read);
    HttpRequest request = parse_request(request_raw);
    HttpResponse response = route_request(request);
    std::string response_str = build_response(response);
    
    send(client_socket, response_str.c_str(), (int)response_str.length(), 0);
}

HttpRequest HttpServer::parse_request(const std::string& request_str) {
    HttpRequest request;
    
    // Split Header and Body
    size_t body_pos = request_str.find("\r\n\r\n");
    if (body_pos == std::string::npos) body_pos = request_str.find("\n\n");

    if (body_pos != std::string::npos) {
        // Headers
        std::string header_section = request_str.substr(0, body_pos);
        std::istringstream iss(header_section);
        iss >> request.method >> request.path;

        // Body: Find the first '{' and last '}' to strip socket junk
        std::string raw_body = request_str.substr(body_pos);
        size_t first_brace = raw_body.find('{');
        size_t last_brace = raw_body.rfind('}');
        
        if (first_brace != std::string::npos && last_brace != std::string::npos) {
            request.body = raw_body.substr(first_brace, last_brace - first_brace + 1);
        }
    }
    
    // Debug output
    std::cout << "Method: " << request.method << ", Path: " << request.path << std::endl;
    if (!request.body.empty()) {
        std::cout << "Body: " << request.body << std::endl;
    }
    
    return request;
}

HttpResponse HttpServer::route_request(const HttpRequest& request) {
    std::cout << "[" << request.method << "] " << request.path << std::endl;
    
    if (request.path == "/health" && request.method == "GET") {
        return handle_health_check();
    }
    else if (request.path == "/order" && request.method == "POST") {
        return handle_place_order(request.body);
    }
    else if (request.path == "/orderbook" && request.method == "GET") {
        return handle_get_orderbook();
    }
    else if (request.path == "/stats" && request.method == "GET") {
        return handle_get_stats();
    }
    else if (request.path == "/order" && request.method == "DELETE") {
        return handle_cancel_order(request.body);
    }
    
    return HttpResponse(404, "{\"error\":\"Not Found\"}");
}

HttpResponse HttpServer::handle_place_order(const std::string& body) {
    try {
        if (body.empty()) {
            return HttpResponse(400, "{\"error\":\"No JSON body found\"}");
        }
        
        auto j = json::parse(body);

        // Strict validation
        if (!j.contains("price") || !j.contains("quantity") || !j.contains("side")) {
            json err;
            err["error"] = "Missing required fields";
            err["required"] = {"price", "quantity", "side"};
            err["received"] = j;
            return HttpResponse(400, err.dump());
        }

        // ============================================================
        // CLEAR PRICE LOGIC: Always expect dollars (e.g., 100.50)
        // Internally convert to cents (10050)
        // ============================================================
        Price price;
        
        if (j["price"].is_number()) {
            double price_dollars = j["price"].get<double>();
            
            // Validate range
            if (price_dollars <= 0.0) {
                return HttpResponse(400, "{\"error\":\"Price must be positive\"}");
            }
            if (price_dollars > 1000000.0) {
                return HttpResponse(400, "{\"error\":\"Price too large (max: $1,000,000)\"}");
            }
            
            // Convert to cents (round to nearest cent)
            price = static_cast<Price>(std::round(price_dollars * 100.0));
            
        } else {
            return HttpResponse(400, "{\"error\":\"Price must be a number\"}");
        }
        
        // ============================================================
        // QUANTITY VALIDATION
        // ============================================================
        if (!j["quantity"].is_number_unsigned()) {
            return HttpResponse(400, "{\"error\":\"Quantity must be a positive integer\"}");
        }
        
        Quantity quantity = j["quantity"].get<Quantity>();
        
        if (quantity == 0) {
            return HttpResponse(400, "{\"error\":\"Quantity must be greater than 0\"}");
        }
        if (quantity > 1000000) {
            return HttpResponse(400, "{\"error\":\"Quantity too large (max: 1,000,000)\"}");
        }
        
        // ============================================================
        // SIDE VALIDATION
        // ============================================================
        if (!j["side"].is_string()) {
            return HttpResponse(400, "{\"error\":\"Side must be a string\"}");
        }
        
        std::string side_str = j["side"].get<std::string>();
        
        // Convert to uppercase for case-insensitive matching
        std::transform(side_str.begin(), side_str.end(), side_str.begin(), ::toupper);
        
        Side side;
        if (side_str == "BUY") {
            side = Side::BUY;
        } else if (side_str == "SELL") {
            side = Side::SELL;
        } else {
            return HttpResponse(400, "{\"error\":\"Side must be 'BUY' or 'SELL'\"}");
        }

        // ============================================================
        // ADD ORDER TO BOOK
        // ============================================================
        auto trades = order_book_.add_order(price, quantity, side);

        // ============================================================
        // BUILD RESPONSE WITH HUMAN-READABLE PRICES
        // ============================================================
        json res;
        res["status"] = "success";
        res["order_count"] = order_book_.get_order_count();
        res["price_received"] = j["price"].get<double>();  // Echo back what user sent
        res["price_internal"] = price;  // Show internal representation
        
        // Add trades
        json trades_array = json::array();
        for (const auto& trade : trades) {
            json trade_obj;
            trade_obj["buyer_id"] = trade.buyer_id;
            trade_obj["seller_id"] = trade.seller_id;
            trade_obj["price"] = trade.price / 100.0;  // Convert back to dollars for display
            trade_obj["price_cents"] = trade.price;     // Also show cents for debugging
            trade_obj["quantity"] = trade.quantity;
            trades_array.push_back(trade_obj);
        }
        res["trades"] = trades_array;
        
        return HttpResponse(200, res.dump());
    } 
    catch (const json::parse_error& e) {
        json err;
        err["error"] = "JSON parse error";
        err["details"] = e.what();
        err["body"] = body;
        return HttpResponse(400, err.dump());
    }
    catch (const std::exception& e) {
        json err;
        err["error"] = "Exception";
        err["msg"] = e.what();
        return HttpResponse(400, err.dump());
    }
}

HttpResponse HttpServer::handle_cancel_order(const std::string& body) {
    try {
        auto j = json::parse(body);
        
        if (!j.contains("order_id")) {
            return HttpResponse(400, "{\"error\":\"Missing order_id\"}");
        }
        
        OrderId order_id = j["order_id"].get<OrderId>();
        bool success = order_book_.cancel_order(order_id);
        
        json res;
        if (success) {
            res["status"] = "cancelled";
            res["order_id"] = order_id;
            return HttpResponse(200, res.dump());
        } else {
            res["error"] = "Order not found";
            res["order_id"] = order_id;
            return HttpResponse(404, res.dump());
        }
    }
    catch (const std::exception& e) {
        return HttpResponse(400, "{\"error\":\"Invalid request\"}");
    }
}

HttpResponse HttpServer::handle_get_orderbook() {
    json res;
    
    // Return prices in dollars for user-friendliness
    Price best_bid = order_book_.get_best_bid();
    Price best_ask = order_book_.get_best_ask();
    Price spread = order_book_.get_spread();
    
    res["best_bid"] = best_bid > 0 ? best_bid / 100.0 : 0.0;
    res["best_ask"] = best_ask > 0 ? best_ask / 100.0 : 0.0;
    res["spread"] = spread > 0 ? spread / 100.0 : 0.0;
    
    // Also include internal representation for debugging
    res["best_bid_cents"] = best_bid;
    res["best_ask_cents"] = best_ask;
    
    res["order_count"] = order_book_.get_order_count();
    res["bid_levels"] = order_book_.get_bid_level_count();
    res["ask_levels"] = order_book_.get_ask_level_count();
    
    return HttpResponse(200, res.dump());
}

HttpResponse HttpServer::handle_get_stats() {
    json res;
    res["total_orders"] = order_book_.get_order_count();
    res["bid_levels"] = order_book_.get_bid_level_count();
    res["ask_levels"] = order_book_.get_ask_level_count();
    
    Price best_bid = order_book_.get_best_bid();
    Price best_ask = order_book_.get_best_ask();
    Price spread = order_book_.get_spread();
    
    res["best_bid"] = best_bid > 0 ? best_bid / 100.0 : 0.0;
    res["best_ask"] = best_ask > 0 ? best_ask / 100.0 : 0.0;
    res["spread"] = spread > 0 ? spread / 100.0 : 0.0;
    
    if (best_bid > 0 && best_ask > 0) {
        res["mid_price"] = (best_bid + best_ask) / 200.0;  // Divide by 200 because already in cents
    } else {
        res["mid_price"] = nullptr;
    }
    
    return HttpResponse(200, res.dump());
}

HttpResponse HttpServer::handle_health_check() {
    return HttpResponse(200, "{\"status\":\"ok\"}");
}

std::string HttpServer::build_response(const HttpResponse& response) {
    std::ostringstream oss;
    
    std::string status_text;
    switch(response.status_code) {
        case 200: status_text = "OK"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        case 501: status_text = "Not Implemented"; break;
        default: status_text = "Unknown"; break;
    }
    
    oss << "HTTP/1.1 " << response.status_code << " " << status_text << "\r\n"
        << "Content-Type: application/json\r\n"
        << "Content-Length: " << response.body.length() << "\r\n"
        << "Access-Control-Allow-Origin: *\r\n"
        << "Connection: close\r\n\r\n"
        << response.body;
    return oss.str();
}

} // namespace trading