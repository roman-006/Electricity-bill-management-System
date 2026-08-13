#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <string>

// Base exception for the whole system. Everything else derives from this,
// which is itself derived from std::exception (required by the proposal).
class BillingException : public std::exception {
protected:
    std::string message;
public:
    explicit BillingException(const std::string &msg) : message(msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

// Thrown when a current meter reading is invalid (e.g. lower than previous).
class InvalidReadingException : public BillingException {
public:
    explicit InvalidReadingException(const std::string &msg)
        : BillingException("Invalid meter reading: " + msg) {}
};

// Thrown when a data file cannot be opened / read / written.
class FileException : public BillingException {
public:
    explicit FileException(const std::string &msg)
        : BillingException("File error: " + msg) {}
};

// Thrown when a lookup (e.g. consumer ID) does not exist.
class ConsumerNotFoundException : public BillingException {
public:
    explicit ConsumerNotFoundException(const std::string &msg)
        : BillingException("Consumer not found: " + msg) {}
};

// Thrown when user-supplied data fails a validation rule (bad name,
// bad date, negative amount, etc.) after the interactive re-prompt
// loops are bypassed - e.g. when validating data loaded from a file.
class ValidationException : public BillingException {
public:
    explicit ValidationException(const std::string &msg)
        : BillingException("Validation error: " + msg) {}
};

// Thrown by the Auth module when a login attempt fails (wrong username/
// password) or when the admin credentials file is missing/corrupted.
class AuthenticationException : public BillingException {
public:
    explicit AuthenticationException(const std::string &msg)
        : BillingException("Authentication error: " + msg) {}
};

#endif
