#include "Consumer.h"
#include <iomanip>

int Consumer::consumerCount = 0; // static member definition

Consumer::Consumer(const std::string &name_, const std::string &address_, int previousReading)
    : id(++consumerCount), name(name_), address(address_),
      meter(previousReading), outstandingBalance(0.0) {}

Consumer::~Consumer() {
    // billHistory (a std::vector<Bill>) and tariff (a unique_ptr) both clean
    // up their own memory automatically here - this destructor exists mainly
    // to document that cleanup and to give each derived class object a
    // well-defined teardown point.
}

Bill Consumer::generateBill(const std::string &date) {
    int units = meter.unitsConsumed();
    double amount = calculateBill(); // polymorphic call -> correct category logic runs
    Bill bill(id, name, getCategory(), units, amount, date);
    billHistory.push_back(bill);
    outstandingBalance += amount;
    meter.rollOver();
    return bill;
}

void Consumer::recordPayment(double amount) {
    outstandingBalance -= amount;
    if (outstandingBalance < 0) outstandingBalance = 0;
    // Apply the payment to the oldest unpaid bill(s) first.
    double remaining = amount;
    for (auto &b : billHistory) {
        if (remaining <= 0) break;
        if (!b.isPaid()) {
            double due = b.getOutstanding();
            double pay = std::min(due, remaining);
            b.recordPayment(pay);
            remaining -= pay;
        }
    }
}

// Overload 1: short summary line.
void Consumer::displayInfo() const {
    std::cout << "[" << id << "] " << name << " (" << getCategory() << ") - "
              << "Balance due: Rs. " << std::fixed << std::setprecision(2) << outstandingBalance
              << std::endl;
}

// Overload 2: full detail including address, meter, and bill history.
void Consumer::displayInfo(bool detailed) const {
    if (!detailed) { displayInfo(); return; }
    std::cout << "----------------------------------------\n";
    std::cout << "Consumer ID   : " << id << "\n";
    std::cout << "Name          : " << name << "\n";
    std::cout << "Address       : " << address << "\n";
    std::cout << "Category      : " << getCategory() << "\n";
    std::cout << "Previous Read : " << meter.getPreviousReading() << "\n";
    std::cout << "Current Read  : " << meter.getCurrentReading() << "\n";
    std::cout << "Outstanding   : Rs. " << std::fixed << std::setprecision(2) << outstandingBalance << "\n";
    std::cout << "Bill history  :\n";
    if (billHistory.empty()) {
        std::cout << "   (no bills generated yet)\n";
    } else {
        for (const auto &b : billHistory) {
            std::cout << "   " << b << "\n"; // uses overloaded operator<<
        }
    }
    std::cout << "----------------------------------------\n";
}

// ---------------- Derived classes ----------------

ResidentialConsumer::ResidentialConsumer(const std::string &name, const std::string &address, int previousReading)
    : Consumer(name, address, previousReading) {
    tariff = std::make_unique<ResidentialTariff>();
}
double ResidentialConsumer::calculateBill() const {
    return tariff->calculate(meter.unitsConsumed());
}

CommercialConsumer::CommercialConsumer(const std::string &name, const std::string &address, int previousReading)
    : Consumer(name, address, previousReading) {
    tariff = std::make_unique<CommercialTariff>();
}
double CommercialConsumer::calculateBill() const {
    return tariff->calculate(meter.unitsConsumed());
}

IndustrialConsumer::IndustrialConsumer(const std::string &name, const std::string &address, int previousReading)
    : Consumer(name, address, previousReading) {
    tariff = std::make_unique<IndustrialTariff>();
}
double IndustrialConsumer::calculateBill() const {
    return tariff->calculate(meter.unitsConsumed());
}

std::unique_ptr<Consumer> makeConsumer(const std::string &category,
                                        const std::string &name,
                                        const std::string &address,
                                        int previousReading) {
    if (category == "Residential")
        return std::make_unique<ResidentialConsumer>(name, address, previousReading);
    if (category == "Commercial")
        return std::make_unique<CommercialConsumer>(name, address, previousReading);
    if (category == "Industrial")
        return std::make_unique<IndustrialConsumer>(name, address, previousReading);
    return nullptr;
}
