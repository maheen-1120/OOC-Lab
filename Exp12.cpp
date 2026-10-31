#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <stdexcept>

using namespace std;

class DonorException : public exception {
private:
    string message;
public:
    explicit DonorException(const string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class InvalidInputException : public DonorException {
public:
    explicit InvalidInputException(const string& msg) : DonorException(msg) {}
};

class DuplicateDonorException : public DonorException {
public:
    explicit DuplicateDonorException(int id)
        : DonorException("A donor with ID " + to_string(id) + " already exists.") {}
};

class FileException : public DonorException {
public:
    explicit FileException(const string& msg) : DonorException(msg) {}
};

class DonorNotFoundException : public DonorException {
public:
    explicit DonorNotFoundException(const string& msg) : DonorException(msg) {}
};

class EndOfInputException : public DonorException {
public:
    EndOfInputException() : DonorException("Input stream closed.") {}
};

class Person {
private:
    string name;
    int age;

public:
    Person() : name("Unknown"), age(0) {}
    Person(const string& n, int a) : name(n), age(a) {}
    virtual ~Person() = default;

    virtual void display() const = 0;
    virtual string getRole() const { return "Person"; }

    void setName(const string& n) {
        if (n.empty()) throw InvalidInputException("Name cannot be empty.");
        name = n;
    }
    void setAge(int a) {
        if (a < 0) throw InvalidInputException("Age cannot be negative.");
        age = a;
    }

    string getName() const { return name; }
    int getAge() const { return age; }
};

class Donor : public Person {
private:
    int donorId;
    string bloodGroup;
    string phone;

    static int totalDonors;

public:
    Donor() : Person(), donorId(0), bloodGroup("Unknown"), phone("Not Available") {}
    Donor(const string& n, int a)
        : Person(n, a), donorId(0), bloodGroup("Unknown"), phone("Not Available") {}
    Donor(const string& n, int a, const string& bg)
        : Donor(n, a) {
        bloodGroup = bg;
    }
    Donor(const string& n, int a, const string& bg, const string& ph, int id)
        : Donor(n, a, bg) {
        phone = ph;
        donorId = id;
    }

    int getDonorId() const { return donorId; }
    string getBloodGroup() const { return bloodGroup; }
    string getPhone() const { return phone; }

    static int getTotalDonors() { return totalDonors; }
    static void setTotalDonors(int n) { totalDonors = n; }

    void display() const override {
        cout << "\n  +------------------------------------------------+\n";
        cout << "  | Donor ID    : " << donorId << '\n';
        cout << "  | Name        : " << getName() << '\n';
        cout << "  | Age         : " << getAge() << '\n';
        cout << "  | Blood Group : " << bloodGroup << '\n';
        cout << "  | Phone       : " << phone << '\n';
        cout << "  +------------------------------------------------+\n";
    }

    string getRole() const override { return "Donor"; }

    bool operator==(const Donor& other) const {
        return donorId == other.donorId;
    }

    friend ostream& operator<<(ostream& os, const Donor& d);

    void saveToFile() const {
        ofstream file("donors.txt", ios::app);
        if (!file.is_open())
            throw FileException("Could not open donors.txt. Check that the folder is writable.");

        // CSV format: ID,Name,Age,BloodGroup,Phone
        file << donorId << ',' << getName() << ',' << getAge() << ','
             << bloodGroup << ',' << phone << '\n';

        if (!file.good())
            throw FileException("Failed while writing to donors.txt.");
    }
};

int Donor::totalDonors = 0;

ostream& operator<<(ostream& os, const Donor& d) {
    os << "  " << d.donorId << "      " << d.getName()
       << string(d.getName().size() < 20 ? 20 - d.getName().size() : 1, ' ')
       << d.getAge() << "     " << d.bloodGroup << "     " << d.phone;
    return os;
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

string trim(const string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

string toUpper(string s) {
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return static_cast<char>(toupper(c)); });
    return s;
}

void pauseScreen() {
    cout << "\n  Press Enter to continue...";
    string dummy;
    getline(cin, dummy);
}

string readLine(const string& prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) throw EndOfInputException();
    return trim(s);
}

int readInt(const string& prompt) {
    string s = readLine(prompt);
    try {
        size_t pos = 0;
        int value = stoi(s, &pos);
        if (pos != s.size())
            throw InvalidInputException("'" + s + "' is not a valid whole number.");
        return value;
    } catch (const invalid_argument&) {
        throw InvalidInputException("Please enter a numeric value.");
    } catch (const out_of_range&) {
        throw InvalidInputException("That number is too large.");
    }
}

void validateBloodGroup(string& bg) {
    static const vector<string> groups = {"A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"};
    bg = toUpper(bg);
    if (find(groups.begin(), groups.end(), bg) == groups.end())
        throw InvalidInputException("Invalid blood group '" + bg + "'.");
}

void validatePhone(const string& phone) {
    int digits = 0;
    for (unsigned char c : phone) {
        if (isdigit(c)) digits++;
        else if (c != '+' && c != '-' && c != ' ' && c != '(' && c != ')')
            throw InvalidInputException("Phone number contains invalid characters.");
    }
    if (digits < 10 || digits > 15)
        throw InvalidInputException("Phone number must have 10-15 digits.");
}

class BloodDonationSystem {
private:
    vector<Donor> loadDonors() {
        vector<Donor> donors;
        ifstream file("donors.txt");
        if (!file.is_open()) {
            Donor::setTotalDonors(0);
            return donors;
        }

        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string idText, name, ageText, bg, phone;

            if (!getline(ss, idText, ',') ||
                !getline(ss, name, ',') ||
                !getline(ss, ageText, ',') ||
                !getline(ss, bg, ',') ||
                !getline(ss, phone)) {
                continue;
            }

            try {
                donors.emplace_back(name, stoi(ageText), bg, phone, stoi(idText));
            } catch (const exception&) {
                // skip malformed lines
            }
        }
        Donor::setTotalDonors(static_cast<int>(donors.size()));
        return donors;
    }

    const Donor& findById(const vector<Donor>& donors, int id) {
        for (const Donor& d : donors)
            if (d.getDonorId() == id) return d;
        throw DonorNotFoundException("No donor found with ID " + to_string(id) + ".");
    }

    void heading(const string& subtitle = "") {
        cout << "\n";
        cout << "  =====================================================\n";
        cout << "            BLOOD DONATION MANAGEMENT SYSTEM\n";
        cout << "  =====================================================\n";
        if (!subtitle.empty())
            cout << "                  " << subtitle << "\n";
        cout << "  -----------------------------------------------------\n";
    }

    void showPerson(const Person& p) {
        p.display();
    }

public:
    void addDonor() {
        clearScreen();
        heading("REGISTER NEW DONOR");
        vector<Donor> donors = loadDonors();

        int id = readInt("\n  Enter Donor ID: ");
        if (id <= 0) throw InvalidInputException("Donor ID must be greater than zero.");

        string name = readLine("  Enter Full Name: ");
        if (name.empty() || name.find(',') != string::npos)
            throw InvalidInputException("Name cannot be empty or contain a comma.");

        int age = readInt("  Enter Age (18-65): ");
        if (age < 18 || age > 65)
            throw InvalidInputException("Donor age must be between 18 and 65.");

        string bg = readLine("  Enter Blood Group (A+, A-, B+, B-, AB+, AB-, O+, O-): ");
        validateBloodGroup(bg);

        string phone = readLine("  Enter Phone Number: ");
        validatePhone(phone);

        Donor donor(name, age, bg, phone, id);

        for (const Donor& d : donors)
            if (d == donor) throw DuplicateDonorException(id);

        donor.saveToFile();
        donors.push_back(donor);
        Donor::setTotalDonors(static_cast<int>(donors.size()));

        cout << "\n  [SUCCESS] Donor registered and saved to donors.txt!\n";
        showPerson(donor);
    }

    void viewDonors() {
        clearScreen();
        heading("SAVED DONOR RECORDS");
        vector<Donor> donors = loadDonors();

        if (donors.empty()) {
            cout << "\n  No donor records found yet.\n";
            return;
        }

        cout << "\n  Total donors: " << Donor::getTotalDonors() << "\n";
        cout << "  -----------------------------------------------------\n";
        cout << "  ID     NAME                     AGE   GROUP   PHONE\n";
        cout << "  -----------------------------------------------------\n";
        for (const Donor& d : donors)
            cout << d << '\n';
        cout << "  -----------------------------------------------------\n";
    }

    void searchDonor(const string& bg) {
        clearScreen();
        heading("SEARCH BY BLOOD GROUP");
        vector<Donor> donors = loadDonors();

        string target = bg;
        validateBloodGroup(target);

        bool found = false;
        for (const Donor& d : donors) {
            if (toUpper(d.getBloodGroup()) == target) {
                showPerson(d);
                found = true;
            }
        }
        if (!found)
            throw DonorNotFoundException("No donor found for blood group " + target + ".");
    }

    void searchDonor(int id) {
        clearScreen();
        heading("SEARCH BY DONOR ID");
        vector<Donor> donors = loadDonors();
        showPerson(findById(donors, id));
    }

    void run() {
        int choice = 0;

        do {
            choice = 0;
            try {
                clearScreen();
                loadDonors();
                heading("WELCOME");
                cout << "\n";
                cout << "      +------------------------------------------+\n";
                cout << "      |               MAIN MENU                  |\n";
                cout << "      +------------------------------------------+\n";
                cout << "      |  1. Register Donor                       |\n";
                cout << "      |  2. View Saved Donors                    |\n";
                cout << "      |  3. Search by Blood Group                |\n";
                cout << "      |  4. Search by Donor ID                   |\n";
                cout << "      |  5. Exit                                 |\n";
                cout << "      +------------------------------------------+\n";
                cout << "        Registered donors: " << Donor::getTotalDonors() << "\n";

                choice = readInt("\n  Enter your choice (1-5): ");

                switch (choice) {
                    case 1: addDonor(); break;
                    case 2: viewDonors(); break;
                    case 3: searchDonor(readLine("\n  Enter Blood Group to search: ")); break;
                    case 4: searchDonor(readInt("\n  Enter Donor ID to search: ")); break;
                    case 5:
                        clearScreen();
                        heading("GOODBYE");
                        cout << "\n  Thank you for supporting blood donation!\n\n";
                        break;
                    default:
                        throw InvalidInputException("Invalid choice. Please choose 1-5.");
                }
            }
            catch (const EndOfInputException&) {
                cout << "\n  Input closed. Exiting.\n";
                return;
            }
            catch (const DonorException& e) {
                cout << "\n  [ERROR] " << e.what() << "\n";
            }
            catch (const exception& e) {
                cout << "\n  [UNEXPECTED ERROR] " << e.what() << "\n";
            }

            if (choice != 5) pauseScreen();
        } while (choice != 5);
    }
};

int main() {
    BloodDonationSystem app;
    app.run();
    return 0;
}