// =============================================================================
// main.cpp - console menu and program entry point.
//
// This file wires together every other module in the project:
//   Consumer / Bill / Meter / Tariff  - the core billing domain model
//   FileManager                        - saving/loading/receipts (fstream)
//   Auth                                - admin login (data/admin.txt)
//   Reports                            - statistics & summaries
//   BackupManager                      - timestamped backup/restore
//   Utils                              - shared input validation & UI helpers
//   Exceptions                         - the BillingException hierarchy
//
// The `consumers` vector below is the single in-memory master list for the
// whole run of the program; every menu action reads/modifies it, and it is
// loaded from / saved to DATA_FILE via FileManager.
// =============================================================================

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include "Consumer.h"
#include "FileManager.h"
#include "Exceptions.h"
#include "Utils.h"
#include "Auth.h"
#include "Reports.h"
#include "BackupManager.h"

const std::string DATA_FILE  = "data/consumers.dat";
const std::string ADMIN_FILE = "data/admin.txt";

// std::vector<std::unique_ptr<Consumer>> is the in-memory master list.
// Because Consumer is an abstract base class, storing pointers (not
// objects) is what makes runtime polymorphism possible here.
std::vector<std::unique_ptr<Consumer>> consumers;

// ---------------------------------------------------------------------------
// Lookup helper shared by almost every menu action below.
// ---------------------------------------------------------------------------
Consumer* findConsumer(int id) {
    for (auto &c : consumers) {
        if (c->getId() == id) return c.get();
    }
    throw ConsumerNotFoundException("no consumer with ID " + std::to_string(id));
}

// ---------------------------------------------------------------------------
// 1. Register Consumer
// ---------------------------------------------------------------------------
void registerConsumer() {
    Utils::printHeader("Register New Consumer");
    std::string name = Utils::readValidName("Name: ");
    std::string address = Utils::readValidAddress("Address: ");

    int choice;
    while (true) {
        choice = Utils::readInt("Category (1=Residential, 2=Commercial, 3=Industrial): ");
        if (choice >= 1 && choice <= 3) break;
        std::cout << "Please choose 1, 2, or 3.\n";
    }
    std::string category = (choice == 2) ? "Commercial" : (choice == 3) ? "Industrial" : "Residential";
    int initialReading = Utils::readNonNegativeInt("Initial meter reading: ");

    auto consumer = makeConsumer(category, name, address, initialReading);
    std::cout << "Registered consumer #" << consumer->getId() << " (" << category << ")\n";
    consumers.push_back(std::move(consumer));
}

// ---------------------------------------------------------------------------
// 2. View All Consumers
// ---------------------------------------------------------------------------
void viewAllConsumers() {
    Utils::printHeader("All Consumers");
    if (consumers.empty()) { std::cout << "No consumers registered yet.\n"; return; }
    for (const auto &c : consumers) {
        c->displayInfo(); // short overload
    }
}

// ---------------------------------------------------------------------------
// 3. View Consumer Detail / Bill History
// ---------------------------------------------------------------------------
void viewConsumerDetail() {
    int id = Utils::readInt("Consumer ID: ");
    Consumer *c = findConsumer(id);
    c->displayInfo(true); // detailed overload
}

// ---------------------------------------------------------------------------
// 4. Update Consumer Details (with validation)
// ---------------------------------------------------------------------------
void updateConsumer() {
    int id = Utils::readInt("Consumer ID: ");
    Consumer *c = findConsumer(id);
    std::cout << "Leave a field blank to keep it unchanged.\n";

    std::string name = Utils::readLine("New name [" + c->getName() + "]: ");
    if (!name.empty()) {
        if (!Utils::isValidName(name)) {
            std::cout << "That name looks invalid - keeping the old name.\n";
        } else {
            c->setName(Utils::trim(name));
        }
    }

    std::string address = Utils::readLine("New address [" + c->getAddress() + "]: ");
    if (!address.empty()) {
        if (!Utils::isValidAddress(address)) {
            std::cout << "That address looks invalid - keeping the old address.\n";
        } else {
            c->setAddress(Utils::trim(address));
        }
    }

    std::cout << "Consumer #" << c->getId() << " updated.\n";
    // Note: category is intentionally not editable here - changing category
    // would mean swapping the tariff/consumer type entirely (a different
    // derived class), which this menu keeps out of scope. Re-register the
    // consumer under the new category instead.
}

// ---------------------------------------------------------------------------
// 5. Remove Consumer
// ---------------------------------------------------------------------------
void removeConsumer() {
    int id = Utils::readInt("Consumer ID to remove: ");
    auto it = std::find_if(consumers.begin(), consumers.end(),
                            [id](const std::unique_ptr<Consumer> &c) { return c->getId() == id; });
    if (it == consumers.end()) {
        throw ConsumerNotFoundException("no consumer with ID " + std::to_string(id));
    }
    if ((*it)->getOutstandingBalance() > 0) {
        std::ostringstream balanceStr;
        balanceStr << std::fixed << std::setprecision(2) << (*it)->getOutstandingBalance();
        bool confirm = Utils::readYesNo("This consumer has an outstanding balance of Rs. " +
                                         balanceStr.str() + ". Remove anyway? (y/n): ");
        if (!confirm) {
            std::cout << "Cancelled.\n";
            return;
        }
    }
    std::cout << "Removed consumer #" << id << " (" << (*it)->getName() << ").\n";
    consumers.erase(it); // unique_ptr's destructor runs here, freeing the Consumer
}

// ---------------------------------------------------------------------------
// 6. Enter Meter Reading
// ---------------------------------------------------------------------------
void enterMeterReading() {
    int id = Utils::readInt("Consumer ID: ");
    Consumer *c = findConsumer(id);
    int reading = Utils::readNonNegativeInt(
        "New current reading (previous was " +
        std::to_string(c->getMeter().getPreviousReading()) + "): ");
    c->getMeter().updateReading(reading); // throws InvalidReadingException if reading < previous
    std::cout << "Reading updated. Units so far this cycle: " << c->getMeter().unitsConsumed() << "\n";
}

// ---------------------------------------------------------------------------
// 7. Generate Bill (+ optional receipt)
// ---------------------------------------------------------------------------
void generateBill() {
    int id = Utils::readInt("Consumer ID: ");
    Consumer *c = findConsumer(id);
    if (c->getMeter().unitsConsumed() <= 0) {
        std::cout << "No new consumption recorded since last bill - enter a meter reading first.\n";
        return;
    }
    std::string date = Utils::readValidDate("Bill date (YYYY-MM-DD, e.g. " + Utils::today() + "): ");
    Bill b = c->generateBill(date); // calculateBill() is called polymorphically inside here
    std::cout << "Generated: " << b << "\n"; // operator<< overload

    if (Utils::readYesNo("Save a printable receipt for this bill? (y/n): ")) {
        std::string path = FileManager::saveReceipt(b, c->getAddress());
        std::cout << "Receipt saved to " << path << "\n";
    }
}

// ---------------------------------------------------------------------------
// 8. Record Payment
// ---------------------------------------------------------------------------
void recordPayment() {
    int id = Utils::readInt("Consumer ID: ");
    Consumer *c = findConsumer(id);
    std::cout << "Outstanding balance: Rs. " << std::fixed << std::setprecision(2)
              << c->getOutstandingBalance() << "\n";
    if (c->getOutstandingBalance() <= 0) {
        std::cout << "This consumer has no outstanding balance.\n";
        return;
    }
    double amount = Utils::readPositiveDouble("Payment amount: ");
    c->recordPayment(amount);
    std::cout << "Payment recorded. New balance: Rs. " << std::fixed << std::setprecision(2)
              << c->getOutstandingBalance() << "\n";
}

// ---------------------------------------------------------------------------
// 9 / 10. Search by ID / Name
// ---------------------------------------------------------------------------
void searchById() {
    int id = Utils::readInt("Consumer ID to search: ");
    Consumer *c = findConsumer(id); // throws ConsumerNotFoundException if absent
    c->displayInfo(true);
}

// Searches by (partial, case-insensitive) name - scans every consumer
// and prints every match, since several consumers can share a name.
void searchByName() {
    std::string query = Utils::toLower(Utils::trim(Utils::readLine("Name (or part of it) to search: ")));
    if (query.empty()) { std::cout << "Search text cannot be blank.\n"; return; }

    bool any = false;
    for (const auto &c : consumers) {
        if (Utils::toLower(c->getName()).find(query) != std::string::npos) {
            c->displayInfo();
            any = true;
        }
    }
    if (!any) std::cout << "No consumer name matches \"" << query << "\".\n";
}

// ---------------------------------------------------------------------------
// 11. Sort Consumers
// ---------------------------------------------------------------------------
// Sorts the in-memory `consumers` vector in place. Because `consumers` is a
// std::vector<std::unique_ptr<Consumer>>, std::sort moves the unique_ptrs
// around (rather than copying, which unique_ptr forbids) - the comparators
// below only *read* through the pointer, never take ownership.
void sortConsumers() {
    if (consumers.empty()) { std::cout << "No consumers to sort.\n"; return; }

    std::cout << "Sort by:\n"
              << "  1. Name (A-Z)\n"
              << "  2. Consumer ID (ascending)\n"
              << "  3. Outstanding bill amount (highest first)\n";
    int choice = Utils::readInt("Choice: ");

    switch (choice) {
        case 1:
            std::sort(consumers.begin(), consumers.end(),
                      [](const std::unique_ptr<Consumer> &a, const std::unique_ptr<Consumer> &b) {
                          return Utils::toLower(a->getName()) < Utils::toLower(b->getName());
                      });
            std::cout << "Sorted by name.\n";
            break;
        case 2:
            std::sort(consumers.begin(), consumers.end(),
                      [](const std::unique_ptr<Consumer> &a, const std::unique_ptr<Consumer> &b) {
                          return a->getId() < b->getId();
                      });
            std::cout << "Sorted by consumer ID.\n";
            break;
        case 3:
            std::sort(consumers.begin(), consumers.end(),
                      [](const std::unique_ptr<Consumer> &a, const std::unique_ptr<Consumer> &b) {
                          return a->getOutstandingBalance() > b->getOutstandingBalance();
                      });
            std::cout << "Sorted by outstanding balance (highest first).\n";
            break;
        default:
            std::cout << "Unknown option - sort cancelled.\n";
            return;
    }
    viewAllConsumers();
}

// ---------------------------------------------------------------------------
// 12. Print / Reprint Bill Receipt
// ---------------------------------------------------------------------------
// Lets the user (re)print a receipt for any past bill, by consumer ID then
// bill ID - useful if the original receipt file was lost/deleted or the
// consumer wants another copy.
void printReceipt() {
    int id = Utils::readInt("Consumer ID: ");
    Consumer *c = findConsumer(id);
    const auto &bills = c->getBillHistory();
    if (bills.empty()) { std::cout << "This consumer has no bills yet.\n"; return; }

    std::cout << "Bills for " << c->getName() << ":\n";
    for (const auto &b : bills) std::cout << "   " << b << "\n";

    int billId = Utils::readInt("Enter the Bill No. to print a receipt for: ");
    auto it = std::find_if(bills.begin(), bills.end(),
                            [billId](const Bill &b) { return b.getBillId() == billId; });
    if (it == bills.end()) {
        std::cout << "No such bill number for this consumer.\n";
        return;
    }
    std::string path = FileManager::saveReceipt(*it, c->getAddress());
    std::cout << "Receipt saved to " << path << "\n";
}

// ---------------------------------------------------------------------------
// 17 / 0. Save Data
// ---------------------------------------------------------------------------
void saveData() {
    FileManager::saveConsumers(consumers, DATA_FILE);
    std::cout << "Data saved to " << DATA_FILE << "\n";
}

// ---------------------------------------------------------------------------
// Menu display
// ---------------------------------------------------------------------------
void printMenu() {
    Utils::printHeader("Electricity Billing and Consumer Management System");
    std::cout << " 1.  Register Consumer\n"
              << " 2.  View All Consumers\n"
              << " 3.  View Consumer Detail / Bill History\n"
              << " 4.  Update Consumer Details\n"
              << " 5.  Remove Consumer\n"
              << " 6.  Enter Meter Reading\n"
              << " 7.  Generate Bill\n"
              << " 8.  Record Payment\n"
              << " 9.  Search Consumer by ID\n"
              << "10.  Search Consumer by Name\n"
              << "11.  Sort Consumers\n"
              << "12.  Print / Reprint Bill Receipt\n"
              << "13.  Reports & Statistics\n"
              << "14.  Backup Data\n"
              << "15.  Restore Data from Backup\n"
              << "16.  Change Admin Password\n"
              << "17.  Save Data\n"
              << " 0.  Save & Exit\n";
    Utils::printDivider();
    std::cout << "Choice: ";
}

// ---------------------------------------------------------------------------
// Program entry point
// ---------------------------------------------------------------------------
int main() {
    Utils::printHeader("Electricity Billing and Consumer Management System");

    // Admin login gate: nobody reaches the main menu without a valid
    // username/password checked against ADMIN_FILE (see Auth.cpp).
    try {
        if (!Auth::login(ADMIN_FILE)) {
            std::cout << "Login failed. Exiting.\n";
            return 1;
        }
        std::cout << "Login successful. Welcome, admin!\n";
    } catch (const BillingException &e) {
        std::cout << "Error: " << e.what() << "\nExiting.\n";
        return 1;
    }

    std::cout << "Loading saved data...\n";
    try {
        FileManager::loadConsumers(consumers, DATA_FILE);
        std::cout << "Loaded " << consumers.size() << " consumer(s).\n";
    } catch (const FileException &e) {
        std::cout << "Warning: " << e.what() << " - starting with an empty dataset.\n";
    }

    bool running = true;
    while (running) {
        printMenu();
        int choice;
        if (!(std::cin >> choice)) {
            if (std::cin.eof()) {
                // Input stream closed (e.g. Ctrl+D, or piped input ran
                // out) - exit gracefully instead of looping forever.
                std::cout << "\nInput ended. Saving and exiting...\n";
                saveData();
                break;
            }
            std::cin.clear();
            Utils::discardRestOfLine();
            std::cout << "Invalid choice - please enter a number from the menu.\n";
            continue;
        }
        Utils::discardRestOfLine();

        // Every operation is wrapped in try/catch so a bad input or file
        // error is reported and the program returns to the menu instead
        // of crashing (this is the "Reliability" non-functional requirement).
        try {
            switch (choice) {
                case 1:  registerConsumer(); break;
                case 2:  viewAllConsumers(); break;
                case 3:  viewConsumerDetail(); break;
                case 4:  updateConsumer(); break;
                case 5:  removeConsumer(); break;
                case 6:  enterMeterReading(); break;
                case 7:  generateBill(); break;
                case 8:  recordPayment(); break;
                case 9:  searchById(); break;
                case 10: searchByName(); break;
                case 11: sortConsumers(); break;
                case 12: printReceipt(); break;
                case 13: Reports::showReportMenu(consumers); break;
                case 14: BackupManager::backupData(DATA_FILE); break;
                case 15:
                    if (BackupManager::restoreData(DATA_FILE)) {
                        FileManager::loadConsumers(consumers, DATA_FILE);
                        std::cout << "Data reloaded from restored backup. "
                                  << consumers.size() << " consumer(s) in memory.\n";
                    }
                    break;
                case 16: Auth::changePassword(ADMIN_FILE); break;
                case 17: saveData(); break;
                case 0:
                    saveData();
                    running = false;
                    break;
                default:
                    std::cout << "Unknown option. Please choose a number from the menu.\n";
            }
        } catch (const BillingException &e) {
            // Catches InvalidReadingException, FileException,
            // ConsumerNotFoundException, ValidationException,
            // AuthenticationException - all derive from BillingException.
            std::cout << "Error: " << e.what() << "\n";
        } catch (const std::exception &e) {
            // Safety net for anything the standard library itself throws
            // (e.g. std::bad_alloc) that isn't one of our own exceptions.
            std::cout << "Unexpected error: " << e.what() << "\n";
        }

        if (running) Utils::pause();
    }

    std::cout << "Goodbye.\n";
    return 0;
}
