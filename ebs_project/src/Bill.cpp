#include "Bill.h"

int Bill::billCounter = 0; // static member definition

Bill::Bill()
    : billId(0), consumerId(0), unitsConsumed(0), amount(0.0),
      paid(false), amountPaid(0.0) {}

Bill::Bill(int consumerId_, const std::string &consumerName_,
           const std::string &category_, int unitsConsumed_,
           double amount_, const std::string &date_)
    : billId(++billCounter), consumerId(consumerId_), consumerName(consumerName_),
      category(category_), unitsConsumed(unitsConsumed_), amount(amount_),
      date(date_), paid(false), amountPaid(0.0) {}

void Bill::recordPayment(double value) {
    amountPaid += value;
    if (amountPaid >= amount) {
        paid = true;
    }
}

// Operator overloading (<<): prints a bill in a consistent, readable format
// both to the console and, via FileManager, to a file.
std::ostream& operator<<(std::ostream &out, const Bill &b) {
    out << "Bill #" << b.billId
        << " | Consumer #" << b.consumerId << " (" << b.consumerName << ", " << b.category << ")"
        << " | Units: " << b.unitsConsumed
        << " | Amount: Rs. " << b.amount
        << " | Paid: Rs. " << b.amountPaid
        << " | Status: " << (b.paid ? "PAID" : "DUE")
        << " | Date: " << b.date;
    return out;
}
