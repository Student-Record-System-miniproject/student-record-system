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
void testPasswordValidation() {
    assert(InputValidator::validatePassword("abc12345"));
    assert(InputValidator::validatePassword("A1bcdefg"));

    assert(!InputValidator::validatePassword("abcdefgh"));
    assert(!InputValidator::validatePassword("12345678"));
    assert(!InputValidator::validatePassword("abc123"));
    assert(!InputValidator::validatePassword(""));
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
    testPasswordValidation();
    return 0;
}