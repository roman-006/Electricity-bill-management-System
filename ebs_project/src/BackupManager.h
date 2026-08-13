#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <string>

// -----------------------------------------------------------------------
// BackupManager: makes timestamped copies of the main data file and lets
// the admin restore one later. Uses only <fstream> and <filesystem>, both
// standard C++17 headers - no external libraries.
//
// Backups are stored in data/backups/consumers_<timestamp>.dat so multiple
// backups can coexist and the admin can pick which one to restore.
// -----------------------------------------------------------------------
namespace BackupManager {

    // Copies `dataFile` into data/backups/ with a timestamped filename.
    void backupData(const std::string &dataFile);

    // Lists available backups, lets the admin pick one, and overwrites
    // `dataFile` with it. Returns true if a restore actually happened
    // (so the caller knows to reload consumers into memory).
    bool restoreData(const std::string &dataFile);
}

#endif
