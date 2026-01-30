#include "../include/order_book.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace trading {

std::vector<Trade> OrderBook::add_order(Price price, Quantity quantity, Side side) {
    Order* order = new Order(next_order_id_++, price, quantity, side);
        orders_[order->id] = order;
        std::vector<Trade> trades = match_order(order);
        if (!order->is_fully_filled()) {
        add_to_book(order);
    } else {
        orders_.erase(order->id);
        delete order;
    }
    
    return trades;
}

bool OrderBook::cancel_order(OrderId order_id) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    Order* order = it->second;
    remove_from_book(order);
    orders_.erase(it);
    delete order;
    
    return true;
}

// --- Private Logic ---

std::vector<Trade> OrderBook::match_order(Order* order) {
    std::vector<Trade> local_trades;
    
    if (order->side == Side::BUY) {
        match_against(order, asks_, local_trades);
    } else {
        match_against(order, bids_, local_trades);
    }
    
    return local_trades;
}


template<typename T>
void OrderBook::match_against(Order* taker_order, T& opposite_side, std::vector<Trade>& trades) {
    while (!taker_order->is_fully_filled() && !opposite_side.empty()) {
        auto it = opposite_side.begin();
        Price best_resting_price = it->first;
        PriceLevel& price_level = it->second;

        // Verify Price Cross (Marketability)
        bool can_match = (taker_order->side == Side::BUY) ? (taker_order->price >= best_resting_price) 
                                                          : (taker_order->price <= best_resting_price);
        if (!can_match) break;

        while (!taker_order->is_fully_filled() && !price_level.is_empty()) {
            Order* maker_order = price_level.get_head();
            Quantity fill_qty = std::min(taker_order->remaining_quantity(), maker_order->remaining_quantity());

            // Generate Trade record
            Trade trade(
                taker_order->side == Side::BUY ? taker_order->id : maker_order->id,
                taker_order->side == Side::SELL ? taker_order->id : maker_order->id,
                best_resting_price,
                fill_qty
            );
            
            trades.push_back(trade);
            trades_.push_back(trade);

            // Update quantities
            taker_order->fill(fill_qty);
            maker_order->fill(fill_qty);
            price_level.update_quantity(fill_qty);

            if (maker_order->is_fully_filled()) {
                price_level.remove_order(maker_order);
                orders_.erase(maker_order->id);
                delete maker_order;
            }
        }

        if (price_level.is_empty()) {
            opposite_side.erase(it);
        }
    }
}

void OrderBook::add_to_book(Order* order) {
    if (order->side == Side::BUY) {
        auto it = bids_.find(order->price);
        if (it == bids_.end()) {
            bids_.emplace(order->price, PriceLevel(order->price));
            it = bids_.find(order->price);
        }
        it->second.add_order(order);
    } else {
        auto it = asks_.find(order->price);
        if (it == asks_.end()) {
            asks_.emplace(order->price, PriceLevel(order->price));
            it = asks_.find(order->price);
        }
        it->second.add_order(order);
    }
}

void OrderBook::remove_from_book(Order* order) {
    if (order->side == Side::BUY) {
        auto it = bids_.find(order->price);
        if (it != bids_.end()) {
            it->second.remove_order(order);
            if (it->second.is_empty()) bids_.erase(it);
        }
    } else {
        auto it = asks_.find(order->price);
        if (it != asks_.end()) {
            it->second.remove_order(order);
            if (it->second.is_empty()) asks_.erase(it);
        }
    }
}

// --- Debug/Display ---

void OrderBook::print() const {
    std::cout << "\n" << std::string(40, '=') << "\n";
    std::cout << "          LIMIT ORDER BOOK\n";
    std::cout << std::string(40, '=') << "\n";
    
    std::cout << "\n--- ASKS (SELLS) ---\n";
    if (asks_.empty()) std::cout << "      (Empty)\n";
    for (auto it = asks_.rbegin(); it != asks_.rend(); ++it) {
        std::cout << "Price: " << std::setw(8) << it->first 
                  << " | Qty: " << std::setw(6) << it->second.get_total_quantity() << "\n";
    }

    // Print Spread
    if (!bids_.empty() && !asks_.empty()) {
        std::cout << "\n[ SPREAD: " << (get_best_ask() - get_best_bid()) << " ]\n";
    }

    std::cout << "\n--- BIDS (BUYS) ---\n";
    if (bids_.empty()) std::cout << "      (Empty)\n";
    for (const auto& [price, level] : bids_) {
        std::cout << "Price: " << std::setw(8) << price 
                  << " | Qty: " << std::setw(6) << level.get_total_quantity() << "\n";
    }
    
    std::cout << "\n" << std::string(40, '=') << "\n";
}

} 