#pragma once
#include "order.hpp"

namespace trading {

class PriceLevel {
private:
    Price price_;              // The price for this level
    Quantity total_quantity_;  // Sum of all order quantities at this price
    Order* head_;              // First order in queue (FIFO)
    Order* tail_;              // Last order in queue (FIFO)
    
public:
    PriceLevel(Price price)
        : price_(price)
        , total_quantity_(0)
        , head_(nullptr)
        , tail_(nullptr)
    {}
    
    Price get_price() const {
        return price_;
    }
    
    Quantity get_total_quantity() const {
        return total_quantity_;
    }
    
    bool is_empty() const {
        return head_ == nullptr;
    }
    
    void add_order(Order* order) {
        if (tail_ == nullptr) {
            head_ = tail_ = order;
            order->next = order->prev = nullptr;
        } else {
            tail_->next = order;
            order->prev = tail_;
            order->next = nullptr;
            tail_ = order;
        }
        
        total_quantity_ += order->remaining_quantity();
    }
    
    void remove_order(Order* order) {
        if (order->prev) {
            order->prev->next = order->next;
        } else {
            head_ = order->next;
        }
        
        if (order->next) {
            order->next->prev = order->prev;
        } else {
            tail_ = order->prev;
        }
        
        total_quantity_ -= order->remaining_quantity();
        
        order->next = order->prev = nullptr;
    }
    
    Order* get_head() {
        return head_;
    }
    
    void update_quantity(Quantity filled) {
        total_quantity_ -= filled;
    }
};

} 