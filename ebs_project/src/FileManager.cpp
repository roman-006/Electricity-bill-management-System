#include "FileManager.h"
#include "Exceptions.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <filesystem> // standard C++17 header - used only to create the
                       // data/receipts and data/backups folders if missing

// Small helper: replace '|' inside free-text fields so it never collides
// with our pipe-delimited file format.
static std::string sanitize(std::string s) {
    std::replace(s.begin(), s.end(), '|', ' ');
    return s;
}

void FileManager::saveConsumers(const std::vector<std::unique_ptr<Consumer>> &consumers,
                                 const std::string &filename) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw FileException("could not open '" + filename + "' for writing");
    }

    for (const auto &c : consumers) {
        out << "CONSUMER|" << c->getId() << "|" << c->getCategory() << "|"
            << sanitize(c->getName()) << "|" << sanitize(c->getAddress()) << "|"
            << c->getMeter().getPreviousReading() << "|" << c->getMeter().getCurrentReading() << "|"
            << c->getOutstandingBalance() << "|" << c->getBillHistory().size() << "\n";

        // FileManager is a friend of Bill, so it can serialize each bill's
        // private fields directly instead of calling a getter per field.
        for (const auto &b : c->getBillHistory()) {
            out << "BILL|" << b.billId << "|" << b.consumerId << "|" << sanitize(b.consumerName) << "|"
                << b.category << "|" << b.unitsConsumed << "|" << b.amount << "|" << b.date << "|"
                << (b.paid ? 1 : 0) << "|" << b.amountPaid << "\n";
        }
    }
    out.close();
}

void FileManager::loadConsumers(std::vector<std::unique_ptr<Consumer>> &consumers,
                                 const std::string &filename) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        // No save file yet (first run) - not treated as a fatal error.
        return;
    }

    consumers.clear();
    int maxConsumerId = 0;
    int maxBillId = 0;

    std::string line;
    Consumer *current = nullptr;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string tag;
        std::getline(ss, tag, '|');

        if (tag == "CONSUMER") {
            std::string idStr, category, name, address, prevStr, currStr, balStr, billCountStr;
            std::getline(ss, idStr, '|');
            std::getline(ss, category, '|');
            std::getline(ss, name, '|');
            std::getline(ss, address, '|');
            std::getline(ss, prevStr, '|');
            std::getline(ss, currStr, '|');
            std::getline(ss, balStr, '|');
            std::getline(ss, billCountStr, '|');

            int prevReading = std::stoi(prevStr);
            int currReading = std::stoi(currStr);

            auto consumer = makeConsumer(category, name, address, prevReading);
            if (!consumer) {
                throw FileException("unknown consumer category '" + category + "' in file");
            }
            if (currReading > prevReading) {
                consumer->getMeter().updateReading(currReading);
            }
            consumer->recordPayment(-std::stod(balStr)); // restore outstanding balance
            // recordPayment expects a positive "payment"; a negative value
            // increases the balance back to what was saved, since a freshly
            // constructed consumer starts at 0.

            maxConsumerId = std::max(maxConsumerId, std::stoi(idStr));
            current = consumer.get();
            consumers.push_back(std::move(consumer));
        } else if (tag == "BILL") {
            if (!current) {
                throw FileException("malformed data file: BILL entry with no CONSUMER");
            }
            std::string billIdStr, consumerIdStr, consumerName, category, unitsStr, amountStr, date, paidStr, paidAmtStr;
            std::getline(ss, billIdStr, '|');
            std::getline(ss, consumerIdStr, '|');
            std::getline(ss, consumerName, '|');
            std::getline(ss, category, '|');
            std::getline(ss, unitsStr, '|');
            std::getline(ss, amountStr, '|');
            std::getline(ss, date, '|');
            std::getline(ss, paidStr, '|');
            std::getline(ss, paidAmtStr, '|');

            Bill b(std::stoi(consumerIdStr), consumerName, category,
                   std::stoi(unitsStr), std::stod(amountStr), date);
            // Direct private-field access via the FileManager<->Bill friendship,
            // needed to restore the exact saved billId/paid state rather than
            // whatever the constructor would generate fresh.
            b.billId = std::stoi(billIdStr);
            b.paid = (paidStr == "1");
            b.amountPaid = std::stod(paidAmtStr);

            maxBillId = std::max(maxBillId, b.billId);
            current->getBillHistory().push_back(b);
        }
    }

    Consumer::setConsumerCount(maxConsumerId);
    Bill::setBillCounter(maxBillId);
}

std::string FileManager::saveReceipt(const Bill &bill, const std::string &consumerAddress) {
    // std::filesystem::create_directories is safe to call even if the
    // folder already exists - it simply does nothing in that case.
    std::filesystem::create_directories("data/receipts");

    std::string path = "data/receipts/receipt_" + std::to_string(bill.getBillId()) + ".txt";
    std::ofstream out(path);
    if (!out.is_open()) {
        throw FileException("could not create receipt file '" + path + "'");
    }

    out << "=========================================\n";
    out << "     ELECTRICITY BILL PAYMENT RECEIPT\n";
    out << "=========================================\n";
    out << "Bill No.        : " << bill.getBillId() << "\n";
    out << "Date            : " << bill.getDate() << "\n";
    out << "-----------------------------------------\n";
    out << "Consumer ID     : " << bill.getConsumerId() << "\n";
    out << "Consumer Name   : " << bill.getConsumerName() << "\n";
    out << "Address         : " << consumerAddress << "\n";
    out << "Category        : " << bill.getCategory() << "\n";
    out << "-----------------------------------------\n";
    out << "Units Consumed  : " << bill.getUnits() << "\n";
    out << std::fixed << std::setprecision(2);
    out << "Bill Amount     : Rs. " << bill.getAmount() << "\n";
    out << "Amount Paid     : Rs. " << bill.getAmountPaid() << "\n";
    out << "Balance Due     : Rs. " << bill.getOutstanding() << "\n";
    out << "Status          : " << (bill.isPaid() ? "PAID" : "DUE") << "\n";
    out << "=========================================\n";
    out << "     Thank you for using our service.\n";
    out << "=========================================\n";
    out.close();

    return path;
}
