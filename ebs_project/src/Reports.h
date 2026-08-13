#ifndef REPORTS_H
#define REPORTS_H

#include <vector>
#include <memory>
#include "Consumer.h"

// -----------------------------------------------------------------------
// Reports: read-only statistics over the in-memory consumer list.
//
// Kept as a separate module (rather than stuffed into main.cpp) because
// reporting is a distinct responsibility from data entry / editing, and
// because it makes the report logic easy to reuse (e.g. if a future
// feature wants to export the same numbers to a file).
//
// Every function here only *reads* the consumers vector - it never
// modifies a Consumer, which is why they all take a const reference.
// -----------------------------------------------------------------------
namespace Reports {

    // Prints a small sub-menu and dispatches to the report the admin picks.
    void showReportMenu(const std::vector<std::unique_ptr<Consumer>> &consumers);

    // Overall summary: total consumers, split by category, total revenue
    // collected, total pending dues, number of unpaid bills.
    void printSummaryReport(const std::vector<std::unique_ptr<Consumer>> &consumers);

    // Lists every consumer with a non-zero outstanding balance, sorted by
    // balance (highest first) - a "who owes the most" report.
    void printPendingDuesReport(const std::vector<std::unique_ptr<Consumer>> &consumers);

    // Highest single bill ever generated, and the consumer with the
    // highest total lifetime billed amount.
    void printTopConsumersReport(const std::vector<std::unique_ptr<Consumer>> &consumers);
}

#endif
