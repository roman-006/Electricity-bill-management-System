#include "BackupManager.h"
#include "Exceptions.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <ctime>

namespace BackupManager {

const std::string BACKUP_DIR = "data/backups";

// Builds a filesystem-safe timestamp like 20260806_143015 for use in a
// backup filename (no spaces or colons, which some filesystems dislike).
static std::string timestamp() {
    std::time_t t = std::time(nullptr);
    std::tm *lt = std::localtime(&t);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d%02d%02d_%02d%02d%02d",
                  lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                  lt->tm_hour, lt->tm_min, lt->tm_sec);
    return std::string(buf);
}

void backupData(const std::string &dataFile) {
    if (!std::filesystem::exists(dataFile)) {
        std::cout << "No saved data file yet - save data at least once before backing up.\n";
        return;
    }

    std::filesystem::create_directories(BACKUP_DIR);
    std::string backupPath = BACKUP_DIR + "/consumers_" + timestamp() + ".dat";

    try {
        // copy_file with overwrite_existing is a plain, safe file copy -
        // no manual byte-by-byte streaming needed for this simple case.
        std::filesystem::copy_file(dataFile, backupPath,
                                    std::filesystem::copy_options::overwrite_existing);
    } catch (const std::filesystem::filesystem_error &e) {
        throw FileException(std::string("backup failed - ") + e.what());
    }

    std::cout << "Backup created: " << backupPath << "\n";
}

bool restoreData(const std::string &dataFile) {
    if (!std::filesystem::exists(BACKUP_DIR) ||
        std::filesystem::is_empty(BACKUP_DIR)) {
        std::cout << "No backups found in " << BACKUP_DIR << ".\n";
        return false;
    }

    std::vector<std::filesystem::path> backups;
    for (const auto &entry : std::filesystem::directory_iterator(BACKUP_DIR)) {
        if (entry.is_regular_file()) backups.push_back(entry.path());
    }
    std::sort(backups.begin(), backups.end()); // filenames sort chronologically

    if (backups.empty()) {
        std::cout << "No backups found in " << BACKUP_DIR << ".\n";
        return false;
    }

    std::cout << "Available backups:\n";
    for (size_t i = 0; i < backups.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << backups[i].filename().string() << "\n";
    }

    int choice = Utils::readInt("Enter the number of the backup to restore (0 to cancel): ");
    if (choice <= 0 || static_cast<size_t>(choice) > backups.size()) {
        std::cout << "Restore cancelled.\n";
        return false;
    }

    bool confirm = Utils::readYesNo(
        "This will OVERWRITE your current saved data with the chosen backup. Continue? (y/n): ");
    if (!confirm) {
        std::cout << "Restore cancelled.\n";
        return false;
    }

    try {
        std::filesystem::copy_file(backups[choice - 1], dataFile,
                                    std::filesystem::copy_options::overwrite_existing);
    } catch (const std::filesystem::filesystem_error &e) {
        throw FileException(std::string("restore failed - ") + e.what());
    }

    std::cout << "Restored from " << backups[choice - 1].filename().string() << "\n";
    return true;
}

} // namespace BackupManager
