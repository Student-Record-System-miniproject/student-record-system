#include "srm/input_validator.h"

#include <charconv>
#include <cctype>
#include <array>

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

bool InputValidator::validateDateOfBirth(const std::string& date) {
    if (date.size() != 10 ||
        date[4] != '-' ||
        date[7] != '-') {
        return false;
    }

    int year = 0;
    int month = 0;
    int day = 0;

    if (!parseInteger(date.substr(0, 4), year) ||
        !parseInteger(date.substr(5, 2), month) ||
        !parseInteger(date.substr(8, 2), day)) {
        return false;
    }

    if (year < 1900 || month < 1 || month > 12) {
        return false;
    }

    const std::array<int, 12> daysInMonth = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    int maxDays = daysInMonth[month - 1];

    const bool isLeapYear =
        (year % 400 == 0) ||
        (year % 4 == 0 && year % 100 != 0);

    if (month == 2 && isLeapYear) {
        maxDays = 29;
    }

    return day >= 1 && day <= maxDays;
}

bool InputValidator::validateContactNumber(const std::string& number) {
    if (number.size() != 10) {
        return false;
    }

    for (unsigned char ch : number) {
        if (!std::isdigit(ch)) {
            return false;
        }
    }

    return true;
}

}  // namespace srm