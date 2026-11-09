#pragma once

#include <string>

namespace srm {

class InputValidator {
public:
    // Validates general text input.
    // Rejects empty input, overly long input, and control characters.
    static bool validateText(const std::string& input,
                             std::size_t maxLength = 100);

    // Safely parses an integer without throwing exceptions.
    static bool parseInteger(const std::string& input, int& value);

    // Validates that an integer is within the specified inclusive range.
    static bool validateRange(int value, int minValue, int maxValue);

    // Validates a confirmation input such as Y/y or N/n.
    static bool validateConfirmation(const std::string& input);

    // Validates a date in YYYY-MM-DD format.
    static bool validateDateOfBirth(const std::string& date);

    // Validates a 10-digit contact number.
    static bool validateContactNumber(const std::string& number);
};

}  // namespace srm