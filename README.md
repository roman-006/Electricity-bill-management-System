# Electricity Billing and Consumer Management System

A console-based C++ implementation matching the minor project proposal
(TU / IOE Purwanchal, Dept. of ECIE — "Electricity Billing and Consumer
Management System").

## Build & run

```
make          # builds ./billing_system
./billing_system
make clean    # removes the binary and all generated data (saved data,
              # admin credentials, receipts, backups)
```

Requires a C++17 compiler (g++ recommended). No external libraries, no
database, no networking.

On the very first run you'll be prompted to log in - a default admin
account (`admin` / `admin123`) is created automatically the first time.
**Change this password immediately** using menu option 16.

## Project layout

```
src/
  Exceptions.h       - BillingException hierarchy (std::exception derived)
  Meter.h             - Meter class (composed inside Consumer)
  Tariff.h            - TariffCalculator abstract class + 3 tariff classes
  Bill.h / .cpp        - Bill class (operator<<, friend class FileManager)
  Consumer.h / .cpp    - Consumer abstract class + 3 derived consumer classes
  FileManager.h/.cpp   - flat-file persistence + receipt generation (fstream)
  Utils.h / .cpp        - shared input validation & console UI helpers
  Auth.h / .cpp         - admin login (hashed credentials in a text file)
  Reports.h / .cpp      - statistics & reporting (revenue, dues, top consumers)
  BackupManager.h/.cpp  - timestamped backup / restore (std::filesystem)
  main.cpp              - console menu, ties everything together
data/
  consumers.dat      - created on first save; holds all consumer + bill data
  admin.txt          - created on first run; admin username + hashed password
  receipts/          - one .txt receipt per bill you choose to save/print
  backups/           - timestamped copies of consumers.dat
```

## Features

Beyond the core billing workflow (register, meter reading, generate bill,
record payment), this version adds:

- **Admin login** - a username/password gate (menu is unreachable without
  logging in); password is hashed before being written to disk, and there's
  a "change password" option. 3 failed attempts locks the session.
- **Search** - by exact consumer ID, or by partial/case-insensitive name.
- **Sort** - by name, by ID, or by outstanding balance (highest first).
- **Strong input validation** - names must look like names, addresses can't
  be blank, meter readings can't be negative, payment amounts must be
  positive, dates must be valid `YYYY-MM-DD`. Every prompt re-asks on bad
  input instead of accepting garbage.
- **Bill receipts** - after generating a bill you can save a formatted
  `.txt` receipt; past receipts can be reprinted any time from the bill
  history (menu option 12).
- **Reports & statistics** - total consumers (by category), total revenue
  collected, total pending dues, unpaid bill count, who owes the most, and
  the biggest bill/consumer on record.
- **Backup & restore** - one-click timestamped backup of the data file, and
  a restore menu that lists all backups and lets you pick one (with a
  confirmation prompt, since it overwrites current data).
- **Robust exception handling** - two new exception types
  (`ValidationException`, `AuthenticationException`) alongside the existing
  ones; the input loops also detect a closed/exhausted input stream (EOF)
  and exit gracefully instead of hanging.

## How each OOP concept maps to the code (for the viva)

| Concept | Where | What to say |
|---|---|---|
| **Classes & Objects** | every header | Consumer, Meter, Bill, TariffCalculator each model a real entity; each registered consumer / generated bill is an object. |
| **Constructors/Destructors** | `Consumer.cpp`, `Bill.cpp` | Consumer's constructor assigns a unique ID via a static counter; `Consumer` has a virtual destructor (needed because it's deleted through a base pointer/`unique_ptr<Consumer>`). |
| **Encapsulation** | `Meter.h` | `currentReading` is private; the only way to change it is `updateReading()`, which validates it can't go below the previous reading. |
| **Abstraction** | `Consumer.h`, `Tariff.h` | `Consumer::calculateBill()` and `TariffCalculator::calculate()` are pure virtual — callers work with "a consumer" / "a tariff" without knowing the concrete category. |
| **Inheritance** | `Consumer.h/.cpp`, `Tariff.h` | `ResidentialConsumer` / `CommercialConsumer` / `IndustrialConsumer` extend `Consumer`; matching `*Tariff` classes extend `TariffCalculator`. |
| **Polymorphism** | `Consumer::generateBill()` | Calls `calculateBill()` through a `Consumer&` — the correct derived-class version runs at runtime depending on the actual object type. |
| **Function Overloading** | `Consumer::displayInfo()` | Two overloads: no-arg gives a one-line summary, `displayInfo(true)` gives a full detailed record with bill history. |
| **Operator Overloading** | `Bill.cpp` | `operator<<` lets you `std::cout << bill` (and it's reused to write bills into the save file and into receipts). |
| **Friend Function/Class** | `Bill.h`, `FileManager.cpp` | `FileManager` is declared a `friend class` of `Bill`, so it can serialize `billId`, `paid`, etc. directly without a getter per field. |
| **Static Members** | `Consumer.h/.cpp`, `Bill.h/.cpp` | `Consumer::consumerCount` generates the next unique consumer ID; `Bill::billCounter` does the same for bill IDs. |
| **File Handling** | `FileManager.cpp`, `Auth.cpp`, `BackupManager.cpp` | `std::ifstream`/`std::ofstream` save/load the pipe-delimited data file and admin credentials; receipts and backups are additional file-handling examples; `std::filesystem` creates folders and copies backup files. |
| **Exception Handling** | `Exceptions.h`, `main.cpp` | Custom exceptions (`InvalidReadingException`, `FileException`, `ConsumerNotFoundException`, `ValidationException`, `AuthenticationException`) all derive from `BillingException` → `std::exception`. The menu loop wraps every operation in `try/catch` so bad input never crashes the program. |
| **STL** | `Consumer.cpp` (billHistory), `main.cpp` (consumers list), `Reports.cpp`/`BackupManager.cpp` (`std::sort`, `std::vector`) | `std::vector<Bill>` holds each consumer's bill history; `std::vector<std::unique_ptr<Consumer>>` holds the master list — dynamic sizing instead of a fixed array; `std::sort` with lambda comparators powers both the Sort menu and the reports. |
| **Namespaces** | `Utils.h/.cpp`, `Auth.h/.cpp`, `Reports.h/.cpp`, `BackupManager.h/.cpp` | These new modules are plain function collections (no state of their own), so each is wrapped in its own `namespace` instead of being written as a class — a good talking point on when a namespace is more appropriate than a class. |

## Design notes worth mentioning in defense

- **Composition vs inheritance**: `Consumer` "has-a" `Meter` (a member, not a
  base class) — that's composition. `ResidentialConsumer` "is-a" `Consumer`
  — that's inheritance. Being able to explain the difference on the spot is
  a common follow-up question.
- **Why `unique_ptr<Consumer>` instead of `Consumer*`**: ownership is clear
  (the vector owns the consumer), and it's automatically deleted — no manual
  `delete`, no memory leak, and it still supports polymorphism through the
  pointer.
- **Why the tariff logic isn't just an `if/else` inside `Consumer`**: pulling
  it out into a separate `TariffCalculator` hierarchy means adding a new
  tariff type later doesn't require touching `Consumer` at all — that's the
  "open/closed" motivation behind using inheritance + polymorphism here
  instead of a switch statement.
- **Tariff rates** (Residential slabs, Commercial flat+surcharge, Industrial
  bulk+demand charge) are illustrative numbers, not sourced from an actual
  utility — worth saying if asked, so it isn't mistaken for real NEA rates.
- **Password hashing**: `Auth.cpp` uses `std::hash<std::string>` to avoid
  storing the raw password on disk. This is explicitly *not* claimed to be
  cryptographically secure — it demonstrates the concept within the scope
  of a 2nd-semester course without pulling in an external crypto library.
- **Why `Utils`/`Auth`/`Reports`/`BackupManager` are namespaces, not
  classes**: none of them hold per-object state — there's only ever "the"
  admin credentials file, "the" backup folder, etc. A namespace of free
  functions is the more honest tool here than a class with only static
  members and no instance data.

## Known simplifications (matches "Limitations" in the proposal)

- No GUI — console menu only.
- Flat-file storage, not a database.
- Single admin account, no concurrency/multi-user handling.
- Password hashing is simple (`std::hash`), not cryptographically secure —
  acceptable for a course project, not for production use.
- No online payment/SMS/email — those are listed as future enhancements.
