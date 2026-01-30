#pragma once
#include <string>
#include <cstdint>
#include <chrono>

namespace trading {

using OrderId = uint64_t;
using Price = uint64_t;      
using Quantity = uint32_t;
using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;

enum class Side : uint8_t {
    BUY = 0,
    SELL = 1
};

enum class OrderStatus : uint8_t {
    NEW = 0,
    PARTIALLY_FILLED = 1,
    FILLED = 2,
    CANCELLED = 3,
    REJECTED = 4
};

struct Order {
    OrderId id;
    Price price;
    Quantity quantity;
    Quantity filled_quantity;
    Side side;
    OrderStatus status;
    Timestamp timestamp;
    
    Order* next;
    Order* prev;
    
    Order(OrderId id_, Price price_, Quantity quantity_, Side side_)
        : id(id_)
        , price(price_)
        , quantity(quantity_)
        , filled_quantity(0)
        , side(side_)
        , status(OrderStatus::NEW)
        , timestamp(std::chrono::high_resolution_clock::now())
        , next(nullptr)
        , prev(nullptr)
    {}
    
    bool is_fully_filled() const {
        return filled_quantity >= quantity;
    }
    
    Quantity remaining_quantity() const {
        return quantity - filled_quantity;
    }
    
    void fill(Quantity qty) {
        filled_quantity += qty;
        if (is_fully_filled()) {
            status = OrderStatus::FILLED;
        } else {
            status = OrderStatus::PARTIALLY_FILLED;
        }
    }
};

struct Trade {
    OrderId buyer_id;
    OrderId seller_id;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
    
    Trade(OrderId buyer, OrderId seller, Price p, Quantity qty)
        : buyer_id(buyer)
        , seller_id(seller)
        , price(p)
        , quantity(qty)
        , timestamp(std::chrono::high_resolution_clock::now())
    {}
};

inline std::string side_to_string(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

inline std::string status_to_string(OrderStatus status) {
    switch (status) {
        case OrderStatus::NEW: return "NEW";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::REJECTED: return "REJECTED";
        default: return "UNKNOWN";
    }
}

inline double price_to_double(Price price) {
    return static_cast<double>(price) / 100.0;
}

inline Price double_to_price(double price) {
    return static_cast<Price>(price * 100.0);
}

} 