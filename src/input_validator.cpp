#include "srm/input_validator.h"

#include <charconv>
#include <cctype>

namespace srm {

bool InputValidator::validateText(const std::string& input,
                                  std::size_t maxLength) {
    if (input.empty() || input.size() > maxLength) {
        return false;
    }

    for (unsigned char ch : input) {
        if (std::iscntrl(ch)) {
            return false;
        }
    }

    return true;
}

bool InputValidator::parseInteger(const std::string& input, int& value) {
    if (input.empty()) {
        return false;
    }

    const char* first = input.data();
    const char* last = first + input.size();

    const auto parsed = std::from_chars(first, last, value);

    return parsed.ec == std::errc{} && parsed.ptr == last;
}

bool InputValidator::validateRange(int value,
                                  int minValue,
                                  int maxValue) {
    return value >= minValue && value <= maxValue;
}

bool InputValidator::validateConfirmation(const std::string& input) {
    return input == "Y" || input == "y" ||
           input == "N" || input == "n";
}

}  // namespace srm