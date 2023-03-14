#pragma once

#include <string>
#include <sstream>
#include <map>
#include <iostream>
#include <set>
#include <list>
#include <iomanip>
#include <utility>
#include <ranges>
#include <numeric>
#include <unordered_map>

#include "utils.hpp"

struct Order {
    uint64_t order_id;
    std::string symbol;
    utils::OrderSide order_side;
    uint32_t price;
    uint32_t quantity;
};

struct Trade {
    std::string symbol;
    uint32_t price;
    uint32_t quantity;
    uint64_t taker_order_id;
    uint64_t maker_order_id;

    std::string to_string() const {
        std::stringstream ss;
        ss << symbol << "," << utils::dequantize(price) <<  "," << quantity << "," << taker_order_id << "," << maker_order_id;        
        return ss.str();
    }
};

struct OrderBook {
    std::map<uint32_t, std::list<Order>, std::greater<>> bids;
    std::map<uint32_t, std::list<Order>, std::less<>> asks;
};

class SymbolOrderBook {
private:
    std::map<std::string, OrderBook> symbol_order_book;
    std::unordered_map<uint64_t, std::list<Order>::iterator> orders;
    std::vector<Trade> trades;
    std::set<uint64_t> filled_orders;
public:
    void create_order(uint64_t order_id, std::string symbol, utils::OrderSide order_side, uint32_t price, uint32_t quantity);
    void update_order(uint64_t order_id, uint32_t price, uint32_t quantity);
    void delete_order(uint64_t order_id);
    void process_trade(uint64_t order_id);
    std::vector<Trade> get_trades();
    std::vector<std::string> get_symbols();
    std::vector<std::string> get_levels(const std::string& symbol, utils::OrderSide order_side);
};

class MatchingEngine {
private:
    std::unique_ptr<SymbolOrderBook> symbol_order_book;
public:
    MatchingEngine();
    void process_order(const std::string& order);
    std::map<std::string, std::vector<std::string>> get_outstanding_orders();
    std::vector<std::string> get_trades();
};