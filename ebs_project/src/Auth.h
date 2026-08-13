#ifndef AUTH_H
#define AUTH_H

#include <string>

// -----------------------------------------------------------------------
// Auth: a very small, file-based admin login system.
//
// Design (kept deliberately simple for a 2nd-semester minor project):
//   - Credentials live in a plain text file: username|hashedPassword
//   - The password is never stored as plain text; it is run through a
//     simple hash function first (std::hash, see hashPassword()). This is
//     NOT cryptographically secure (that's out of scope for this course),
//     but it demonstrates the *idea* of not storing raw passwords, and
//     is a common, honest way to present this in an IOE viva.
//   - No external libraries, no database, no networking - just fstream,
//     matching the rest of the project's constraints.
// -----------------------------------------------------------------------
namespace Auth {

    // Ensures the credentials file exists; if this is the very first run,
    // creates it with a default admin/admin123 login and tells the user
    // to change the password.
    void ensureCredentialsFile(const std::string &filename);

    // Prompts for username/password (max attempts) and returns true if
    // the login succeeds. Throws AuthenticationException if the number
    // of allowed attempts is exceeded.
    bool login(const std::string &filename, int maxAttempts = 3);

    // Lets the logged-in admin change their password. Requires the
    // current correct password before accepting a new one.
    void changePassword(const std::string &filename);
}

#endif
