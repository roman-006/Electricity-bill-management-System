#include "Utils.h"
#include <iostream>
#include <limits>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace Utils {

void printHeader(const std::string &title) {
    std::cout << "\n=====================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "=====================================================\n";
}

void printDivider() {
    std::cout << "-----------------------------------------------------\n";
}

void pause() {
    std::cout << "Press Enter to continue...";
    // By this point every reader (readInt/readDouble via discardRestOfLine,
    // or readLine via getline) has already consumed its own trailing
    // newline, so the stream is positioned at the start of a fresh line.
    // A single get() is enough to wait for the user's Enter key without
    // swallowing an extra character from whatever comes next.
    std::cin.get();
}

void clearScreen() {
    // Best-effort only: on most terminals this clears the screen; if the
    // terminal doesn't support it, it just prints a few blank lines instead,
    // which is harmless. We avoid system("cls"/"clear") portability issues
    // by simply printing newlines - this keeps the project free of any
    // platform-specific / external calls, as required.
    for (int i = 0; i < 3; ++i) std::cout << "\n";
}

void discardRestOfLine() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int readInt(const std::string &prompt) {
    int value;
    std::cout << prompt;
    while (!(std::cin >> value)) {
        if (std::cin.eof()) {
            // Input stream has genuinely ended (e.g. piped input ran out,
            // or the user closed the terminal) - looping forever here
            // would hang the program, so we fail loudly instead.
            throw std::runtime_error("input stream ended unexpectedly while reading a number");
        }
        std::cin.clear();
        discardRestOfLine();
        std::cout << "Invalid input. Please enter a whole number: ";
    }
    discardRestOfLine();
    return value;
}

double readDouble(const std::string &prompt) {
    double value;
    std::cout << prompt;
    while (!(std::cin >> value)) {
        if (std::cin.eof()) {
            throw std::runtime_error("input stream ended unexpectedly while reading a number");
        }
        std::cin.clear();
        discardRestOfLine();
        std::cout << "Invalid input. Please enter a number: ";
    }
    discardRestOfLine();
    return value;
}

std::string readLine(const std::string &prompt) {
    std::string value;
    std::cout << prompt;
    std::getline(std::cin, value);
    return value;
}

bool readYesNo(const std::string &prompt) {
    while (true) {
        std::string ans = readLine(prompt);
        if (std::cin.eof() && ans.empty()) {
            throw std::runtime_error("input stream ended unexpectedly while expecting y/n");
        }
        ans = trim(toLower(ans));
        if (ans == "y" || ans == "yes") return true;
        if (ans == "n" || ans == "no") return false;
        std::cout << "Please answer y or n.\n";
    }
}

int readNonNegativeInt(const std::string &prompt) {
    while (true) {
        int v = readInt(prompt);
        if (v >= 0) return v;
        std::cout << "Value cannot be negative. Try again.\n";
    }
}

double readPositiveDouble(const std::string &prompt) {
    while (true) {
        double v = readDouble(prompt);
        if (v > 0) return v;
        std::cout << "Value must be greater than zero. Try again.\n";
    }
}

std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string toLower(const std::string &s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

bool isValidName(const std::string &raw) {
    std::string s = trim(raw);
    if (s.empty()) return false;
    for (char c : s) {
        if (!(std::isalpha(static_cast<unsigned char>(c)) || c == ' ' ||
              c == '\'' || c == '-' || c == '.')) {
            return false;
        }
    }
    return true;
}

bool isValidAddress(const std::string &raw) {
    return !trim(raw).empty();
}

bool isValidDate(const std::string &s) {
    // Expected strict format: YYYY-MM-DD  (10 characters)
    if (s.size() != 10) return false;
    if (s[4] != '-' || s[7] != '-') return false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    int year  = std::stoi(s.substr(0, 4));
    int month = std::stoi(s.substr(5, 2));
    int day   = std::stoi(s.substr(8, 2));
    if (year < 2000 || year > 2100) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    return true;
}

std::string readValidName(const std::string &prompt) {
    while (true) {
        std::string s = readLine(prompt);
        if (isValidName(s)) return trim(s);
        std::cout << "Invalid name - use letters, spaces, - . or ' only, and don't leave it blank.\n";
    }
}

std::string readValidAddress(const std::string &prompt) {
    while (true) {
        std::string s = readLine(prompt);
        if (isValidAddress(s)) return trim(s);
        std::cout << "Address cannot be blank.\n";
    }
}

std::string readValidDate(const std::string &prompt) {
    while (true) {
        std::string s = readLine(prompt);
        if (isValidDate(s)) return s;
        std::cout << "Invalid date - please use the format YYYY-MM-DD (e.g. 2026-08-06).\n";
    }
}

std::string today() {
    std::time_t t = std::time(nullptr);
    std::tm *lt = std::localtime(&t);
    std::ostringstream oss;
    oss << (lt->tm_year + 1900) << "-"
        << std::setw(2) << std::setfill('0') << (lt->tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << lt->tm_mday;
    return oss.str();
}

} // namespace Utils
