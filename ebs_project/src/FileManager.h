#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <vector>
#include <memory>
#include <string>
#include "Consumer.h"

// Responsible for all reading/writing of persistent data. Nothing else in
// the program touches fstream directly - that responsibility is isolated
// here, so swapping flat files for a database later only means rewriting
// this one class (as noted in the proposal's "Future Enhancements").
class FileManager {
public:
    static void saveConsumers(const std::vector<std::unique_ptr<Consumer>> &consumers,
                               const std::string &filename);

    static void loadConsumers(std::vector<std::unique_ptr<Consumer>> &consumers,
                               const std::string &filename);

    // Writes a nicely formatted plain-text receipt for one bill into
    // data/receipts/receipt_<billId>.txt and returns the path written to.
    // consumerAddress is passed separately because Bill itself doesn't
    // store it (keeps Bill focused on billing data, not consumer data).
    static std::string saveReceipt(const Bill &bill, const std::string &consumerAddress);
};

#endif
