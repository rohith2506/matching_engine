#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <sstream>
#include <iostream>

namespace utils
{

constexpr double tick_size = 4;

enum OrderType {
    INSERT = 0,
    UPDATE,
    DELETE
};

enum OrderSide {
    BUY = 0,
    SELL = 1
};

std::vector<std::string> split_string(const std::string& input, char delimiter);
OrderSide get_order_side(const std::string& order_side);
OrderType get_order_type(const std::string& order_type);

uint32_t quantize(const std::string& input);
std::string dequantize(uint32_t input);

} // namespace utils

