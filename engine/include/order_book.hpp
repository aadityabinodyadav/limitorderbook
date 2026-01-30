#pragma once
#include <map>
#include <vector>
#include <memory>
#include "order.hpp"
#include "price_level.hpp"

namespace trading {

class OrderBook {
private:
    std::map<Price, PriceLevel, std::greater<Price>> bids_; // Highest price first
    std::map<Price, PriceLevel, std::less<Price>> asks_;    // Lowest price first
    std::map<OrderId, Order*> orders_;
    std::vector<Trade> trades_;
    OrderId next_order_id_;
    
public:
    OrderBook() : next_order_id_(1) {}
    // Clean up raw pointers in destructor
    ~OrderBook() { for (auto& [id, order] : orders_) delete order; }

    std::vector<Trade> add_order(Price price, Quantity quantity, Side side);
    bool cancel_order(OrderId order_id);
    
    Price get_best_bid() const { return bids_.empty() ? 0 : bids_.begin()->first; }
    Price get_best_ask() const { return asks_.empty() ? 0 : asks_.begin()->first; }
    Price get_spread() const { return (bids_.empty() || asks_.empty()) ? 0 : get_best_ask() - get_best_bid(); }
    size_t get_order_count() const { return orders_.size(); }
    size_t get_bid_level_count() const { return bids_.size(); }
    size_t get_ask_level_count() const { return asks_.size(); }
    
    void print() const;
    
private:
    std::vector<Trade> match_order(Order* order);
    
    // The Template Helper that fixes the ternary error
    template<typename T>
    void match_against(Order* taker_order, T& opposite_side, std::vector<Trade>& trades);

    void add_to_book(Order* order);
    void remove_from_book(Order* order);
};

}