#include "Reports.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace Reports {

void showReportMenu(const std::vector<std::unique_ptr<Consumer>> &consumers) {
    std::cout << "Reports:\n"
              << "  1. Overall Summary (totals, revenue, pending dues)\n"
              << "  2. Pending Dues Report (who owes money)\n"
              << "  3. Top Consumers Report\n";
    int choice = Utils::readInt("Choice: ");
    switch (choice) {
        case 1: printSummaryReport(consumers); break;
        case 2: printPendingDuesReport(consumers); break;
        case 3: printTopConsumersReport(consumers); break;
        default: std::cout << "Unknown option.\n";
    }
}

void printSummaryReport(const std::vector<std::unique_ptr<Consumer>> &consumers) {
    Utils::printHeader("Overall Summary Report");

    int residential = 0, commercial = 0, industrial = 0;
    double totalRevenueCollected = 0.0;   // sum of amountPaid across all bills
    double totalPendingDues = 0.0;        // sum of outstandingBalance across all consumers
    double totalBilled = 0.0;             // sum of every bill's amount
    int totalBillsIssued = 0;
    int unpaidBillCount = 0;

    for (const auto &c : consumers) {
        if (c->getCategory() == "Residential") ++residential;
        else if (c->getCategory() == "Commercial") ++commercial;
        else if (c->getCategory() == "Industrial") ++industrial;

        totalPendingDues += c->getOutstandingBalance();

        for (const auto &b : c->getBillHistory()) {
            totalBilled += b.getAmount();
            totalRevenueCollected += b.getAmountPaid();
            ++totalBillsIssued;
            if (!b.isPaid()) ++unpaidBillCount;
        }
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total consumers registered : " << consumers.size() << "\n";
    std::cout << "  Residential               : " << residential << "\n";
    std::cout << "  Commercial                : " << commercial << "\n";
    std::cout << "  Industrial                : " << industrial << "\n";
    Utils::printDivider();
    std::cout << "Total bills issued          : " << totalBillsIssued << "\n";
    std::cout << "Unpaid bills                : " << unpaidBillCount << "\n";
    std::cout << "Total amount billed         : Rs. " << totalBilled << "\n";
    std::cout << "Total revenue collected     : Rs. " << totalRevenueCollected << "\n";
    std::cout << "Total pending dues          : Rs. " << totalPendingDues << "\n";
}

void printPendingDuesReport(const std::vector<std::unique_ptr<Consumer>> &consumers) {
    Utils::printHeader("Pending Dues Report");

    // Collect raw pointers to consumers who owe money, then sort a
    // *copy* of the pointer list (does not disturb the master vector's
    // order - this report is read-only, per the module's contract).
    std::vector<const Consumer*> owing;
    for (const auto &c : consumers) {
        if (c->getOutstandingBalance() > 0) owing.push_back(c.get());
    }

    if (owing.empty()) {
        std::cout << "No consumers currently have pending dues.\n";
        return;
    }

    std::sort(owing.begin(), owing.end(),
              [](const Consumer *a, const Consumer *b) {
                  return a->getOutstandingBalance() > b->getOutstandingBalance();
              });

    std::cout << std::fixed << std::setprecision(2);
    double total = 0.0;
    for (const auto *c : owing) {
        std::cout << "[" << c->getId() << "] " << c->getName()
                  << " (" << c->getCategory() << ") - Rs. "
                  << c->getOutstandingBalance() << "\n";
        total += c->getOutstandingBalance();
    }
    Utils::printDivider();
    std::cout << "Total pending across " << owing.size() << " consumer(s): Rs. " << total << "\n";
}

void printTopConsumersReport(const std::vector<std::unique_ptr<Consumer>> &consumers) {
    Utils::printHeader("Top Consumers Report");

    if (consumers.empty()) {
        std::cout << "No consumers registered yet.\n";
        return;
    }

    const Bill *highestBill = nullptr;
    const Consumer *highestBillConsumer = nullptr;

    const Consumer *topLifetimeConsumer = nullptr;
    double topLifetimeAmount = -1.0;

    for (const auto &c : consumers) {
        double lifetimeTotal = 0.0;
        for (const auto &b : c->getBillHistory()) {
            lifetimeTotal += b.getAmount();
            if (!highestBill || b.getAmount() > highestBill->getAmount()) {
                highestBill = &b;
                highestBillConsumer = c.get();
            }
        }
        if (lifetimeTotal > topLifetimeAmount) {
            topLifetimeAmount = lifetimeTotal;
            topLifetimeConsumer = c.get();
        }
    }

    std::cout << std::fixed << std::setprecision(2);
    if (highestBill && highestBillConsumer) {
        std::cout << "Highest single bill  : Rs. " << highestBill->getAmount()
                  << " (Consumer #" << highestBillConsumer->getId()
                  << " - " << highestBillConsumer->getName() << ")\n";
    } else {
        std::cout << "Highest single bill  : (no bills generated yet)\n";
    }

    if (topLifetimeConsumer && topLifetimeAmount > 0) {
        std::cout << "Top lifetime billing : Rs. " << topLifetimeAmount
                  << " (Consumer #" << topLifetimeConsumer->getId()
                  << " - " << topLifetimeConsumer->getName() << ")\n";
    } else {
        std::cout << "Top lifetime billing : (no bills generated yet)\n";
    }
}

} // namespace Reports
