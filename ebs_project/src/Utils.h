#ifndef UTILS_H
#define UTILS_H

#include <string>

// -----------------------------------------------------------------------
// Utils: a small collection of free functions used across the whole
// program (main.cpp, Auth.cpp, Reports.cpp, BackupManager.cpp, ...).
//
// Why a separate module?
//   Several features (login, search, reports) all need the same basic
//   things - "ask the user for a whole number until they give one",
//   "check a name isn't empty/garbage", "print a section header" - so
//   this file collects them in ONE place instead of copy-pasting the
//   same loop into every .cpp file. This is plain functional decomposition,
//   not a class, because these helpers don't hold any state of their own.
// -----------------------------------------------------------------------
namespace Utils {

    // ---------- Console formatting ----------
    void printHeader(const std::string &title);   // e.g. ===== TITLE =====
    void printDivider();                            // a line of dashes
    void pause();                                    // "Press Enter to continue..."
    void clearScreen();                              // best-effort screen clear

    // ---------- Raw input helpers (with retry-until-valid loops) ----------
    void discardRestOfLine();
    int readInt(const std::string &prompt);
    double readDouble(const std::string &prompt);
    std::string readLine(const std::string &prompt);
    bool readYesNo(const std::string &prompt);       // loops until y/n

    // Reads an integer but re-prompts until it is >= 0 (used for meter
    // readings, ages, counts, etc. where negative values make no sense).
    int readNonNegativeInt(const std::string &prompt);

    // Reads a double but re-prompts until it is > 0 (used for payments).
    double readPositiveDouble(const std::string &prompt);

    // ---------- Validation (used both for interactive input and for
    //             sanity-checking data coming back out of a file) ----------
    std::string trim(const std::string &s);
    std::string toLower(const std::string &s);

    // A "valid name" is non-empty after trimming and contains only
    // letters, spaces, apostrophes, hyphens and periods (covers names
    // like "Anne-Marie", "O'Brien", or "Hari B. Thapa") - blocks
    // accidental numeric/garbage input.
    bool isValidName(const std::string &s);

    // An address just needs to be non-empty after trimming; addresses can
    // legitimately contain numbers, commas, etc.
    bool isValidAddress(const std::string &s);

    // Accepts dates in strict YYYY-MM-DD form with sane ranges
    // (this is a lightweight structural check, not a full calendar
    // validator - enough to keep obviously wrong input out of bills).
    bool isValidDate(const std::string &s);

    // Repeatedly asks for a name until isValidName() accepts it.
    std::string readValidName(const std::string &prompt);

    // Repeatedly asks for an address until isValidAddress() accepts it.
    std::string readValidAddress(const std::string &prompt);

    // Repeatedly asks for a date until isValidDate() accepts it.
    std::string readValidDate(const std::string &prompt);

    // Returns the current system date formatted as YYYY-MM-DD, used as a
    // sensible default so the user isn't forced to type today's date by hand.
    std::string today();
}

#endif
