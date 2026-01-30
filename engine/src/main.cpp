#include <iostream>
#include <thread>
#include <csignal>
#include "../include/order_book.hpp"
#include "../include/http_server.hpp"

using namespace trading;

// Global server pointer for signal handling
HttpServer* g_server = nullptr;

// Signal handler
void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   🚀 TRADING ENGINE MATCHING ENGINE" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Version: 1.0.0" << std::endl;
    std::cout << "Port: 8080" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Create order book
    OrderBook order_book;
    
    // Create HTTP server
    HttpServer server(8080, order_book);
    g_server = &server;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // kill command
    
    // Start server (blocking)
    server.start();
    
    std::cout << "\nShutdown complete." << std::endl;
    
    return 0;
}