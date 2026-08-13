CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
SRC_DIR = src
SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/Consumer.cpp $(SRC_DIR)/Bill.cpp $(SRC_DIR)/FileManager.cpp \
          $(SRC_DIR)/Utils.cpp $(SRC_DIR)/Auth.cpp $(SRC_DIR)/Reports.cpp $(SRC_DIR)/BackupManager.cpp
TARGET = billing_system

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)
	rm -f data/consumers.dat
	rm -f data/admin.txt
	rm -rf data/receipts
	rm -rf data/backups

.PHONY: clean
