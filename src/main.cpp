#include <iostream>
#include <memory>

#include "include/main.hpp"
#include "include/core.hpp"


std::vector<std::string> run(std::vector<std::string> const& input) {
    std::unique_ptr<MatchingEngine> matching_engine = std::make_unique<MatchingEngine>();

    for (auto& order : input) {
        matching_engine->process_order(order);
    }
    
    std::vector<std::string> result;
    auto trades = matching_engine->get_trades();
    for (auto trade : matching_engine->get_trades()) {
        result.emplace_back(trade);
    }

    auto outstanding_orders = matching_engine->get_outstanding_orders();
    for (auto& [symbol, outstanding_orders] : matching_engine->get_outstanding_orders()) {
        result.emplace_back("===" + symbol + "===");
        for (auto& outstanding_order : outstanding_orders) {
            result.emplace_back(outstanding_order);
        }        
    }

    return result;
}

int main() {
    std::vector<std::string> input;

    input.emplace_back("INSERT,20,TSLA,BUY,412,31");
    input.emplace_back("INSERT,30,TSLA,SELL,510.7,27");

    auto result = run(input);
    for (auto& res : result) {
        std::cout << res << std::endl;
    }

    return 0;
}
