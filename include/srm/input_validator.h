#pragma once

#include <cstddef>
#include <string>

namespace srm {

class InputValidator {
public:
    static bool validateText(const std::string& input,
                             std::size_t maxLength = 100);

    static bool parseInteger(const std::string& input, int& value);

    static bool validateRange(int value, int minValue, int maxValue);

    static bool validateConfirmation(const std::string& input);

    // Validates a date in YYYY-MM-DD format.
    static bool validateDateOfBirth(const std::string& date);

    // Validates a 10-digit contact number.
    static bool validateContactNumber(const std::string& number);

    // Validates password requirements.
    static bool validatePassword(const std::string& password);
};

}  // namespace srm