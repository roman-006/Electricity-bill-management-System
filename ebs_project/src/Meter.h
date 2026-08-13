#ifndef METER_H
#define METER_H

#include "Exceptions.h"

// Meter is "has-a" composed inside Consumer (composition, not inheritance).
class Meter {
private:
    int previousReading;
    int currentReading;

public:
    Meter(int prevReading = 0) : previousReading(prevReading), currentReading(prevReading) {}

    int getPreviousReading() const { return previousReading; }
    int getCurrentReading() const { return currentReading; }

    // Encapsulation: the only way to change currentReading is through this
    // function, which validates the input before accepting it.
    void updateReading(int newReading) {
        if (newReading < previousReading) {
            throw InvalidReadingException(
                "current reading (" + std::to_string(newReading) +
                ") cannot be less than previous reading (" +
                std::to_string(previousReading) + ")");
        }
        currentReading = newReading;
    }

    int unitsConsumed() const {
        return currentReading - previousReading;
    }

    // Called after a bill is generated, so next cycle's "previous" becomes
    // today's "current".
    void rollOver() {
        previousReading = currentReading;
    }
};

#endif
