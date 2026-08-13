#ifndef BILL_H
#define BILL_H

#include <string>
#include <iostream>

class FileManager; // forward declaration, needed for the friend class

class Bill {
private:
    static int billCounter;      // static member: shared by every Bill object

    int billId;
    int consumerId;
    std::string consumerName;
    std::string category;
    int unitsConsumed;
    double amount;
    std::string date;
    bool paid;
    double amountPaid;

public:
    Bill(); // default constructor, needed so Bill can live in a std::vector
    Bill(int consumerId, const std::string &consumerName,
         const std::string &category, int unitsConsumed,
         double amount, const std::string &date);

    int getBillId() const { return billId; }
    int getConsumerId() const { return consumerId; }
    double getAmount() const { return amount; }
    double getAmountPaid() const { return amountPaid; }
    double getOutstanding() const { return amount - amountPaid; }
    bool isPaid() const { return paid; }
    const std::string &getDate() const { return date; }
    int getUnits() const { return unitsConsumed; }
    const std::string &getCategory() const { return category; }
    const std::string &getConsumerName() const { return consumerName; }

    // Records a payment against this bill; marks it paid once fully covered.
    void recordPayment(double value);

    static int getBillCounter() { return billCounter; }
    static void setBillCounter(int value) { billCounter = value; } // used when loading from file

    // Operator overloading: lets us do  std::cout << bill;
    friend std::ostream& operator<<(std::ostream &out, const Bill &b);

    // FileManager needs raw access to every field to serialize the bill
    // efficiently without going through a getter for each one.
    friend class FileManager;
};

#endif
