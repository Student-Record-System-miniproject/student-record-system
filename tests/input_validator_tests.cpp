#include "srm/input_validator.h"

#include <cassert>
#include <string>

namespace {

using srm::InputValidator;

void testValidText() {
    assert(InputValidator::validateText("Alice"));
    assert(InputValidator::validateText("Computer Science"));
    assert(InputValidator::validateText("alice@example.com"));
}

void testEmptyTextRejected() {
    assert(!InputValidator::validateText(""));
}

void testOverlyLongTextRejected() {
    const std::string longInput(101, 'A');
    assert(!InputValidator::validateText(longInput));

    const std::string acceptedInput(100, 'A');
    assert(InputValidator::validateText(acceptedInput));
}

void testControlCharactersRejected() {
    assert(!InputValidator::validateText("Alice\nSmith"));
    assert(!InputValidator::validateText("Alice\tSmith"));
    assert(!InputValidator::validateText("Alice\rSmith"));
}

void testIntegerParsing() {
    int value = 0;

    assert(InputValidator::parseInteger("42", value));
    assert(value == 42);

    assert(InputValidator::parseInteger("0", value));
    assert(value == 0);

    assert(InputValidator::parseInteger("-10", value));
    assert(value == -10);
}

void testMalformedIntegerRejected() {
    int value = 0;

    assert(!InputValidator::parseInteger("", value));
    assert(!InputValidator::parseInteger("abc", value));
    assert(!InputValidator::parseInteger("12abc", value));
    assert(!InputValidator::parseInteger("12.5", value));
}

void testIntegerRange() {
    assert(InputValidator::validateRange(0, 0, 100));
    assert(InputValidator::validateRange(50, 0, 100));
    assert(InputValidator::validateRange(100, 0, 100));

    assert(!InputValidator::validateRange(-1, 0, 100));
    assert(!InputValidator::validateRange(101, 0, 100));
}

void testConfirmation() {
    assert(InputValidator::validateConfirmation("Y"));
    assert(InputValidator::validateConfirmation("y"));
    assert(InputValidator::validateConfirmation("N"));
    assert(InputValidator::validateConfirmation("n"));

    assert(!InputValidator::validateConfirmation(""));
    assert(!InputValidator::validateConfirmation("Yes"));
    assert(!InputValidator::validateConfirmation("No"));
    assert(!InputValidator::validateConfirmation("X"));
}

void testDateOfBirthValidation() {
    // Valid dates
    assert(InputValidator::validateDateOfBirth("2005-04-12"));
    assert(InputValidator::validateDateOfBirth("2004-02-29"));
    assert(InputValidator::validateDateOfBirth("2000-02-29"));

    // Invalid formats
    assert(!InputValidator::validateDateOfBirth("12-04-2005"));
    assert(!InputValidator::validateDateOfBirth("2005/04/12"));
    assert(!InputValidator::validateDateOfBirth("2005-4-12"));
    assert(!InputValidator::validateDateOfBirth(""));

    // Invalid calendar dates
    assert(!InputValidator::validateDateOfBirth("2005-02-29"));
    assert(!InputValidator::validateDateOfBirth("2005-13-01"));
    assert(!InputValidator::validateDateOfBirth("2005-04-31"));
    assert(!InputValidator::validateDateOfBirth("2005-00-12"));
}

void testContactNumberValidation() {
    // Valid contact number
    assert(InputValidator::validateContactNumber("9876543210"));

    // Invalid contact numbers
    assert(!InputValidator::validateContactNumber("12345"));
    assert(!InputValidator::validateContactNumber("987654321"));
    assert(!InputValidator::validateContactNumber("98765432101"));
    assert(!InputValidator::validateContactNumber("98765AB210"));
    assert(!InputValidator::validateContactNumber(""));
}

}  // namespace

int main() {
    testValidText();
    testEmptyTextRejected();
    testOverlyLongTextRejected();
    testControlCharactersRejected();
    testIntegerParsing();
    testMalformedIntegerRejected();
    testIntegerRange();
    testConfirmation();
    testDateOfBirthValidation();
    testContactNumberValidation();

    return 0;
}