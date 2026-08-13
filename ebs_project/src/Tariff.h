#ifndef TARIFF_H
#define TARIFF_H

#include <string>

// Abstraction: client code (Consumer) only ever talks to a TariffCalculator*
// and never needs to know which concrete tariff it is holding.
class TariffCalculator {
public:
    // Pure virtual function -> makes this class abstract.
    virtual double calculate(int units) const = 0;
    virtual std::string name() const = 0;
    virtual ~TariffCalculator() {} // virtual destructor: safe deletion via base pointer
};

// ---- Residential: cheap slab rates, small fixed charge ----
class ResidentialTariff : public TariffCalculator {
public:
    double calculate(int units) const override {
        double amount = 50.0; // fixed service charge
        if (units <= 50) {
            amount += units * 5.0;
        } else if (units <= 100) {
            amount += 50 * 5.0 + (units - 50) * 7.0;
        } else {
            amount += 50 * 5.0 + 50 * 7.0 + (units - 100) * 10.0;
        }
        return amount;
    }
    std::string name() const override { return "Residential Tariff"; }
};

// ---- Commercial: higher flat-ish rate, higher fixed charge ----
class CommercialTariff : public TariffCalculator {
public:
    double calculate(int units) const override {
        double amount = 150.0; // fixed service charge
        amount += units * 10.0;
        if (units > 200) {
            amount += (units - 200) * 2.0; // small surcharge for heavy usage
        }
        return amount;
    }
    std::string name() const override { return "Commercial Tariff"; }
};

// ---- Industrial: bulk rate + demand-linked fixed charge ----
class IndustrialTariff : public TariffCalculator {
public:
    double calculate(int units) const override {
        double amount = 300.0; // fixed demand charge
        amount += units * 12.0;
        if (units > 500) {
            amount += (units - 500) * 3.0;
        }
        return amount;
    }
    std::string name() const override { return "Industrial Tariff"; }
};

#endif
