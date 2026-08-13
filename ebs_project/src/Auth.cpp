#include "Auth.h"
#include "Utils.h"
#include "Exceptions.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>
#include <vector>

namespace Auth {

// A lightweight, dependency-free "hash": we deliberately use std::hash
// (from the standard library, so it needs no external crypto library)
// so the raw password is never written to disk. This is NOT meant to be
// cryptographically secure - it exists to demonstrate the *concept* of
// not storing plain-text passwords, which is appropriate for this
// course's scope.
static std::string hashPassword(const std::string &password) {
    std::hash<std::string> hasher;
    size_t h = hasher(password);
    return std::to_string(h);
}

void ensureCredentialsFile(const std::string &filename) {
    std::ifstream check(filename);
    if (check.good()) return; // already exists

    std::ofstream out(filename);
    if (!out.is_open()) {
        throw FileException("could not create admin credentials file '" + filename + "'");
    }
    out << "admin|" << hashPassword("admin123") << "\n";
    out.close();

    std::cout << "\nNo admin account found - a default account has been created.\n"
              << "  Username: admin\n"
              << "  Password: admin123\n"
              << "Please log in and change this password immediately (Admin menu).\n";
}

bool login(const std::string &filename, int maxAttempts) {
    ensureCredentialsFile(filename);

    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        std::string user = Utils::readLine("Username: ");
        if (std::cin.eof() && user.empty()) {
            throw AuthenticationException("input ended before login could complete");
        }
        std::string pass = Utils::readLine("Password: ");
        std::string hashed = hashPassword(pass);

        std::ifstream in(filename);
        if (!in.is_open()) {
            throw FileException("could not open admin credentials file '" + filename + "'");
        }

        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string storedUser, storedHash;
            std::getline(ss, storedUser, '|');
            std::getline(ss, storedHash, '|');
            if (storedUser == user && storedHash == hashed) {
                return true; // success
            }
        }

        std::cout << "Incorrect username or password. Attempts left: "
                  << (maxAttempts - attempt) << "\n";
    }
    throw AuthenticationException("too many failed login attempts");
}

void changePassword(const std::string &filename) {
    std::string user = Utils::readLine("Confirm your username: ");
    std::string oldPass = Utils::readLine("Current password: ");
    std::string oldHash = hashPassword(oldPass);

    std::ifstream in(filename);
    if (!in.is_open()) {
        throw FileException("could not open admin credentials file '" + filename + "'");
    }
    std::vector<std::string> lines;
    std::string line;
    bool found = false;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string storedUser, storedHash;
        std::getline(ss, storedUser, '|');
        std::getline(ss, storedHash, '|');
        if (storedUser == user && storedHash == oldHash) {
            found = true;
        }
        lines.push_back(line);
    }
    in.close();

    if (!found) {
        throw AuthenticationException("username/password did not match - password not changed");
    }

    std::string newPass = Utils::readLine("New password (min 4 characters): ");
    while (newPass.size() < 4) {
        newPass = Utils::readLine("Too short. New password (min 4 characters): ");
    }
    std::string newHash = hashPassword(newPass);

    std::ofstream out(filename, std::ios::trunc);
    if (!out.is_open()) {
        throw FileException("could not open admin credentials file '" + filename + "' for writing");
    }
    out << user << "|" << newHash << "\n";
    out.close();
    std::cout << "Password changed successfully.\n";
}

} // namespace Auth
