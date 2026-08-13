#ifndef CONSUMER_H
#define CONSUMER_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "Meter.h"
#include "Tariff.h"
#include "Bill.h"

// Abstract base class. Cannot be instantiated directly (pure virtual
// calculateBill()). Residential/Commercial/Industrial consumers all
// inherit from this common interface, which is what lets the rest of
// the program store them polymorphically as Consumer* / unique_ptr<Consumer>.
class Consumer {
protected:
    static int consumerCount; // static member: total consumers ever registered

    int id;
    std::string name;
    std::string address;
    Meter meter;
    std::vector<Bill> billHistory;
    double outstandingBalance;
    std::unique_ptr<TariffCalculator> tariff; // each derived class installs its own tariff

public:
    Consumer(const std::string &name, const std::string &address, int previousReading = 0);
    virtual ~Consumer(); // virtual destructor - required whenever a class is used polymorphically

    int getId() const { return id; }
    const std::string &getName() const { return name; }
    const std::string &getAddress() const { return address; }
    double getOutstandingBalance() const { return outstandingBalance; }
    Meter &getMeter() { return meter; }
    const Meter &getMeter() const { return meter; }
    std::vector<Bill> &getBillHistory() { return billHistory; }
    const std::vector<Bill> &getBillHistory() const { return billHistory; }

    void setAddress(const std::string &newAddress) { address = newAddress; }
    void setName(const std::string &newName) { name = newName; }

    // Pure virtual functions -> what makes Consumer abstract, and what
    // enables runtime polymorphism: calling generateBill() through a
    // Consumer* / Consumer& invokes the correct category's version.
    virtual double calculateBill() const = 0;
    virtual std::string getCategory() const = 0;

    // Uses the (validated) meter reading to produce a Bill for this billing
    // cycle, appends it to history, updates the outstanding balance, and
    // rolls the meter forward.
    Bill generateBill(const std::string &date);

    void recordPayment(double amount);

    // Function overloading: same name, different behaviour depending on
    // the argument supplied.
    void displayInfo() const;                 // short summary
    void displayInfo(bool detailed) const;     // detailed record (includes bill history)

    static int getConsumerCount() { return consumerCount; }
    static void setConsumerCount(int value) { consumerCount = value; } // used when loading from file
};

class ResidentialConsumer : public Consumer {
public:
    ResidentialConsumer(const std::string &name, const std::string &address, int previousReading = 0);
    double calculateBill() const override;
    std::string getCategory() const override { return "Residential"; }
};

class CommercialConsumer : public Consumer {
public:
    CommercialConsumer(const std::string &name, const std::string &address, int previousReading = 0);
    double calculateBill() const override;
    std::string getCategory() const override { return "Commercial"; }
};

class IndustrialConsumer : public Consumer {
public:
    IndustrialConsumer(const std::string &name, const std::string &address, int previousReading = 0);
    double calculateBill() const override;
    std::string getCategory() const override { return "Industrial"; }
};

// Factory helper: builds the right derived class from a category string.
// Centralizes the "if category == ... new XConsumer(...)" logic in one place.
std::unique_ptr<Consumer> makeConsumer(const std::string &category,
                                        const std::string &name,
                                        const std::string &address,
                                        int previousReading = 0);

#endif
