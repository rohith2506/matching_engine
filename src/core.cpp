#include "include/core.hpp"

// Implementation of symbol order book
void SymbolOrderBook::create_order(uint64_t order_id, std::string symbol, utils::OrderSide order_side, uint32_t price, uint32_t quantity) {
    if (symbol_order_book.find(symbol) == symbol_order_book.end()) {
        symbol_order_book[symbol] = OrderBook{};
    }
    if (orders.find(order_id) != orders.end() && orders[order_id]->symbol == symbol) {
        throw std::runtime_error("Order ID already exists for specific symbol. Order ID: " + 
            std::to_string(order_id)  + ", Symbol: " + symbol);
    }

    auto& order_book = symbol_order_book[symbol];

    if (order_side == utils::OrderSide::BUY) {
        auto& bids = order_book.bids;
        bids[price].push_back(Order{order_id, symbol, order_side, price, quantity});
        auto iter = bids[price].end();
        std::advance(iter, -1);
        orders[order_id] = iter;
    } else {
        auto& asks = order_book.asks;
        asks[price].push_back(Order{order_id, symbol, order_side, price, quantity});
        auto iter = asks[price].end();
        std::advance(iter, -1);
        orders[order_id] = iter;
    }
}

void SymbolOrderBook::update_order(uint64_t order_id, uint32_t price, uint32_t quantity) {
    if (filled_orders.contains(order_id)) {
        throw std::runtime_error("order_id: " +  std::to_string(order_id) + " is already filled before update");
    }
    if (orders.find(order_id) == orders.end()) {
        throw std::runtime_error("Unknown order id to update: " + std::to_string(order_id));
    }
    auto order_price = orders[order_id]->price;
    auto order_quantity = orders[order_id]->quantity;

    if (order_price == price && quantity < order_quantity) {
        auto order_iter = orders[order_id];
        order_iter->quantity = quantity;
    } else {
        auto symbol = orders[order_id]->symbol;
        auto order_side = orders[order_id]->order_side;
        delete_order(order_id);
        create_order(order_id, symbol, order_side, price, quantity);
    }
}

void SymbolOrderBook::delete_order(uint64_t order_id) {
    if (filled_orders.contains(order_id)) {
        throw std::runtime_error("order_id: " +  std::to_string(order_id) + " is already filled before delete");
    }
    if (orders.find(order_id) == orders.end()) {
        throw std::runtime_error("Unknown order id to delete. Order ID: " + std::to_string(order_id));
    }
    auto existing_order = orders[order_id];
    auto& order_book = symbol_order_book[existing_order->symbol];

    if (existing_order->order_side == utils::OrderSide::BUY) {
        order_book.bids[existing_order->price].erase(existing_order);
    }
    else {
        order_book.asks[existing_order->price].erase(existing_order);
    }    
    orders.erase(order_id);
}

void SymbolOrderBook::process_trade(uint64_t order_id) {
    auto& taker_order = orders[order_id];

    auto symbol = taker_order->symbol;
    auto taker_order_side = taker_order->order_side;
    auto taker_order_price = taker_order->price;
    auto& taker_order_quantity = taker_order->quantity;

    auto process_level_orders = [&](uint32_t maker_price, auto& maker_order_levels) {
        std::vector<uint64_t> sweeped_orders;
        for (auto& maker_order_level : maker_order_levels) {
            if (taker_order_quantity > maker_order_level.quantity) {
                trades.emplace_back(
                    Trade{symbol, maker_order_level.price, maker_order_level.quantity, order_id, maker_order_level.order_id}
                );
                taker_order_quantity -= maker_order_level.quantity;
                sweeped_orders.emplace_back(maker_order_level.order_id);
            }
            else {
                trades.emplace_back(
                    Trade{symbol, maker_order_level.price, taker_order_quantity, order_id, maker_order_level.order_id});
                maker_order_level.quantity -= taker_order_quantity;
                sweeped_orders.emplace_back(order_id);
                break;
            }
        }

        // Delete those sweeped orders
        for (auto order_id : sweeped_orders) {
            filled_orders.insert(order_id);
            auto order_id_iter = orders[order_id];
            maker_order_levels.erase(order_id_iter);
            orders.erase(order_id);
        }
    };

    if (taker_order_side == utils::OrderSide::BUY) {
        for (auto& [maker_price, maker_order_levels]: symbol_order_book[symbol].asks) {
            if (!taker_order_quantity || taker_order_price < maker_price)
                return ;

            process_level_orders(maker_price, maker_order_levels);
        }
    } else {
        for (auto& [maker_price, maker_order_levels]: symbol_order_book[symbol].bids) {
            if (!taker_order_quantity || maker_price < taker_order_price)
                return ;

            process_level_orders(maker_price, maker_order_levels);
        }
    }
}

std::vector<Trade> SymbolOrderBook::get_trades() {
    return trades;
}

std::vector<std::string> SymbolOrderBook::get_symbols() {
    std::vector<std::string> symbols;
    for (const auto& [symbol, _] : symbol_order_book) {
        symbols.push_back(symbol);
    }
    return symbols;
}

std::vector<std::string> SymbolOrderBook::get_levels(const std::string& symbol, utils::OrderSide order_side) {
    std::vector<std::string> result;

    auto process_orders = [&](auto& order_book) {
        for (auto& [price, order_levels] : order_book) {
            std::stringstream ss;
            auto total_order_quantity = std::accumulate(
                order_levels.begin(), order_levels.end(), 0, [](auto sum, const Order& order) {
                    return sum + order.quantity;
                });
            if (!total_order_quantity)
                continue;
            ss << utils::dequantize(price) << "," << total_order_quantity;
            result.emplace_back(std::move(ss.str()));
        }
    };

    if (order_side == utils::OrderSide::BUY)  {
        process_orders(symbol_order_book[symbol].bids);
    }
    else {
        process_orders(symbol_order_book[symbol].asks);
    }
    return result;
}

// Implementation of matching engine
MatchingEngine::MatchingEngine() {
    symbol_order_book = std::make_unique<SymbolOrderBook>();
}

void MatchingEngine::process_order(const std::string& order) {
    std::vector<std::string> order_parts = utils::split_string(order, ',');    
    auto order_type = utils::get_order_type(order_parts[0]);

    switch(order_type) {
        // INSERT,4,AAPL,BUY,23.45,12
        case utils::OrderType::INSERT: 
        {
            auto order_id = std::stoull(order_parts[1]);
            auto& symbol = order_parts[2];
            auto order_side = utils::get_order_side(order_parts[3]);
            auto price = utils::quantize(order_parts[4]);
            auto quantity = std::stoi(order_parts[5]);
            symbol_order_book->create_order(order_id, std::move(symbol), order_side, price, quantity);
            symbol_order_book->process_trade(order_id);
            break;
        }
        // AMEND,4,23.12,11
        case utils::OrderType::UPDATE:
        {
            auto order_id = std::stoull(order_parts[1]);
            auto price = utils::quantize(order_parts[2]);
            auto quantity = std::stoi(order_parts[3]);            
            symbol_order_book->update_order(order_id, price, quantity);
            symbol_order_book->process_trade(order_id);
            break;
        }
        // PULL,4
        case utils::OrderType::DELETE:
        {
            auto order_id = std::stoull(order_parts[1]);
            symbol_order_book->delete_order(order_id);
            break;
        }
    }
}

std::vector<std::string> MatchingEngine::get_trades() {
    std::vector<std::string> trades;
    for (auto& trade: symbol_order_book->get_trades()) {
        trades.emplace_back(trade.to_string());
    }
    return trades;
}

std::map<std::string, std::vector<std::string>> MatchingEngine::get_outstanding_orders() {
    std::map<std::string, std::vector<std::string>> outstanding_orders;

    auto symbols = symbol_order_book->get_symbols();
    for (auto& symbol : symbols) {
        auto bids = symbol_order_book->get_levels(symbol, utils::OrderSide::BUY);
        auto asks = symbol_order_book->get_levels(symbol, utils::OrderSide::SELL);

        int index = 0;
        while (index < std::min(bids.size(), asks.size())) {
            outstanding_orders[symbol].emplace_back(bids[index] + "," + asks[index]);
            index++;
        }
        while (index < bids.size()) {
            outstanding_orders[symbol].emplace_back(bids[index] + ",,");
            index++;
        }
        while (index < asks.size()) {
            outstanding_orders[symbol].emplace_back(",," + asks[index]);
            index++;
        }
    }

    return outstanding_orders;
}
