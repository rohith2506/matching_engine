#include "include/utils.hpp"

namespace utils
{

std::vector<std::string> split_string(const std::string& input, char delimiter) {
    std::vector<std::string> string_parts;
    std::string sub_string = "";
    for (size_t index = 0; index < input.length(); index++) {
        if (input[index] == delimiter) {
            string_parts.emplace_back(sub_string);
            sub_string.clear();
        }
        else {
            sub_string.push_back(input[index]);
        }
    }
    if (sub_string.length())
        string_parts.emplace_back(sub_string);
    return string_parts;
}

OrderSide get_order_side(const std::string& order_side) {
    if (order_side == "BUY") {
        return OrderSide::BUY;
    } else if (order_side == "SELL") {
        return OrderSide::SELL;
    } else {
        throw std::runtime_error("Unknown order side " + order_side);
    }
}

OrderType get_order_type(const std::string& order_type) {
    if (order_type == "INSERT") {
        return OrderType::INSERT;
    } else if (order_type == "AMEND") {
        return OrderType::UPDATE;
    } else if (order_type == "PULL") {
        return OrderType::DELETE;
    } else {
        throw std::runtime_error("Unknown order type " + order_type);
    }
}

// Instead of using double and floats which results in precision issues,
// I prefer to use integers in the multiple of tick_size and then convert
// them back to strings while printing out
uint32_t quantize(const std::string& input) {
    size_t point_index = input.find('.');
    std::string p1, p2;
    if (point_index == std::string::npos) {
        p1 = input.substr(0, input.length());
        p2 = "";        
    }
    else {
        p1 = input.substr(0, point_index);
        p2 = input.substr(point_index + 1, tick_size);
    }
    while (p2.length() < tick_size) p2.push_back('0');
    return std::stoi(p1 + p2);
}

std::string dequantize(uint32_t input) {
    std::string input_str = std::to_string(input);
    std::string p1 = input_str.substr(0, input_str.length() - tick_size);    
    std::string p2 = input_str.substr(p1.length(), input_str.length());
    
    if (!p1.length()) p1.push_back('0');
    while (p2[p2.length() - 1] == '0') p2.pop_back();

    if (!p2.length()) return p1;
    else return p1 + "." + p2;
}

} // namespace utils
