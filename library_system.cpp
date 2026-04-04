/*
 * Library Book Management System
 * Original Implementation
 * Description: Manages books, members, borrowing records,
 *              fines calculation, and availability tracking.
 */

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>
#include <ctime>

// ─── Constants ───────────────────────────────────────────────────────────────
const int    MAX_BORROW_DAYS   = 14;
const double FINE_PER_DAY      = 2.50;
const int    MAX_BOOKS_MEMBER  = 5;
const int    MAX_MEMBERS       = 300;
const int    MAX_BOOKS         = 1000;

// ─── Enums ────────────────────────────────────────────────────────────────────
enum class BookStatus {
    AVAILABLE,
    BORROWED,
    RESERVED,
    DAMAGED
};

enum class MemberType {
    STUDENT,
    FACULTY,
    PUBLIC
};

// ─── Utility Functions ────────────────────────────────────────────────────────
std::string statusToString(BookStatus s) {
    if (s == BookStatus::AVAILABLE) return "Available";
    if (s == BookStatus::BORROWED)  return "Borrowed";
    if (s == BookStatus::RESERVED)  return "Reserved";
    return "Damaged";
}

std::string memberTypeToString(MemberType t) {
    if (t == MemberType::STUDENT) return "Student";
    if (t == MemberType::FACULTY) return "Faculty";
    return "Public";
}

double computeFine(int daysOverdue) {
    if (daysOverdue <= 0) return 0.0;
    return daysOverdue * FINE_PER_DAY;
}

// ─── Book ─────────────────────────────────────────────────────────────────────
struct Book {
    int         bookId;
    std::string title;
    std::string author;
    std::string isbn;
    std::string genre;
    int         publishYear;
    BookStatus  status;
    int         borrowedByMemberId;
    int         borrowDay;   // day number for simplicity

    Book(int id, std::string t, std::string a,
         std::string i, std::string g, int yr)
        : bookId(id), title(std::move(t)), author(std::move(a)),
          isbn(std::move(i)), genre(std::move(g)), publishYear(yr),
          status(BookStatus::AVAILABLE), borrowedByMemberId(-1), borrowDay(0) {}

    bool isAvailable() const {
        return status == BookStatus::AVAILABLE;
    }

    std::string shortInfo() const {
        return "[" + std::to_string(bookId) + "] " + title + " by " + author;
    }
};

// ─── Member ───────────────────────────────────────────────────────────────────
struct Member {
    int                  memberId;
    std::string          name;
    std::string          email;
    MemberType           type;
    std::vector<int>     borrowedBookIds;
    double               totalFines;
    bool                 isActive;

    Member(int id, std::string n, std::string e, MemberType t)
        : memberId(id), name(std::move(n)), email(std::move(e)),
          type(t), totalFines(0.0), isActive(true) {}

    bool canBorrow() const {
        return isActive && borrowedBookIds.size() < MAX_BOOKS_MEMBER && totalFines < 10.0;
    }

    void borrowBook(int bookId) {
        if (borrowedBookIds.size() >= MAX_BOOKS_MEMBER)
            throw std::overflow_error("Borrow limit reached for: " + name);
        borrowedBookIds.push_back(bookId);
    }

    void returnBook(int bookId) {
        auto it = std::find(borrowedBookIds.begin(), borrowedBookIds.end(), bookId);
        if (it == borrowedBookIds.end())
            throw std::invalid_argument("Book not borrowed by this member.");
        borrowedBookIds.erase(it);
    }

    void addFine(double amount) {
        totalFines += amount;
    }

    void clearFines() {
        totalFines = 0.0;
    }
};

// ─── Borrow Record ────────────────────────────────────────────────────────────
struct BorrowRecord {
    int    recordId;
    int    memberId;
    int    bookId;
    int    borrowDay;
    int    returnDay;
    bool   returned;
    double fine;

    BorrowRecord(int rid, int mid, int bid, int day)
        : recordId(rid), memberId(mid), bookId(bid),
          borrowDay(day), returnDay(-1), returned(false), fine(0.0) {}

    int daysOverdue(int currentDay) const {
        int daysOut = currentDay - borrowDay;
        return std::max(0, daysOut - MAX_BORROW_DAYS);
    }
};

// ─── Library System ───────────────────────────────────────────────────────────
class LibrarySystem {
private:
    std::vector<Book>         books;
    std::vector<Member>       members;
    std::vector<BorrowRecord> records;
    std::map<int, size_t>     bookIndex;
    std::map<int, size_t>     memberIndex;
    int                       nextBookId;
    int                       nextMemberId;
    int                       nextRecordId;
    int                       currentDay;
    std::string               libraryName;

    Book& getBook(int id) {
        auto it = bookIndex.find(id);
        if (it == bookIndex.end())
            throw std::invalid_argument("Book ID not found: " + std::to_string(id));
        return books[it->second];
    }

    Member& getMember(int id) {
        auto it = memberIndex.find(id);
        if (it == memberIndex.end())
            throw std::invalid_argument("Member ID not found: " + std::to_string(id));
        return members[it->second];
    }

public:
    explicit LibrarySystem(std::string name = "City Library")
        : nextBookId(1), nextMemberId(101),
          nextRecordId(5001), currentDay(1),
          libraryName(std::move(name)) {}

    // ── Book Operations ──────────────────────────────────────────────────────
    int addBook(const std::string& title, const std::string& author,
                const std::string& isbn,  const std::string& genre, int year) {
        if (books.size() >= MAX_BOOKS)
            throw std::overflow_error("Book collection full.");
        int id = nextBookId++;
        books.emplace_back(id, title, author, isbn, genre, year);
        bookIndex[id] = books.size() - 1;
        return id;
    }

    void removeBook(int bookId) {
        Book& b = getBook(bookId);
        if (b.status == BookStatus::BORROWED)
            throw std::logic_error("Cannot remove a borrowed book.");
        b.status = BookStatus::DAMAGED;
    }

    // ── Member Operations ────────────────────────────────────────────────────
    int registerMember(const std::string& name,
                       const std::string& email, MemberType type) {
        if (members.size() >= MAX_MEMBERS)
            throw std::overflow_error("Member capacity reached.");
        int id = nextMemberId++;
        members.emplace_back(id, name, email, type);
        memberIndex[id] = members.size() - 1;
        return id;
    }

    void deactivateMember(int memberId) {
        getMember(memberId).isActive = false;
    }

    // ── Borrow / Return ──────────────────────────────────────────────────────
    int borrowBook(int memberId, int bookId) {
        Member& m = getMember(memberId);
        Book&   b = getBook(bookId);

        if (!m.canBorrow())
            throw std::logic_error("Member cannot borrow: check fines or limit.");
        if (!b.isAvailable())
            throw std::logic_error("Book is not available.");

        m.borrowBook(bookId);
        b.status             = BookStatus::BORROWED;
        b.borrowedByMemberId = memberId;
        b.borrowDay          = currentDay;

        int rid = nextRecordId++;
        records.emplace_back(rid, memberId, bookId, currentDay);
        return rid;
    }

    double returnBook(int memberId, int bookId) {
        Member& m = getMember(memberId);
        Book&   b = getBook(bookId);

        m.returnBook(bookId);
        b.status             = BookStatus::AVAILABLE;
        b.borrowedByMemberId = -1;

        double fine = 0.0;
        for (auto& r : records) {
            if (r.memberId == memberId && r.bookId == bookId && !r.returned) {
                int overdue = r.daysOverdue(currentDay);
                fine        = computeFine(overdue);
                r.fine      = fine;
                r.returned  = true;
                r.returnDay = currentDay;
                break;
            }
        }

        if (fine > 0) m.addFine(fine);
        return fine;
    }

    void advanceDay(int days = 1) {
        currentDay += days;
    }

    void payFine(int memberId, double amount) {
        Member& m = getMember(memberId);
        m.totalFines = std::max(0.0, m.totalFines - amount);
    }

    // ── Reports ──────────────────────────────────────────────────────────────
    void printMemberReport(int memberId) const {
        auto it = memberIndex.find(memberId);
        if (it == memberIndex.end()) {
            std::cout << "Member not found.\n";
            return;
        }
        const Member& m = members[it->second];

        std::cout << "\n╔══════════════════════════════════════╗\n";
        std::cout << "║         MEMBER REPORT                ║\n";
        std::cout << "╚══════════════════════════════════════╝\n";
        std::cout << "  Name   : " << m.name   << "\n";
        std::cout << "  ID     : " << m.memberId << "\n";
        std::cout << "  Email  : " << m.email  << "\n";
        std::cout << "  Type   : " << memberTypeToString(m.type) << "\n";
        std::cout << "  Fines  : $" << std::fixed << std::setprecision(2) << m.totalFines << "\n";
        std::cout << "  Status : " << (m.isActive ? "Active" : "Inactive") << "\n";
        std::cout << "  Books Borrowed: " << m.borrowedBookIds.size() << "\n";
        std::cout << "  Can Borrow    : " << (m.canBorrow() ? "Yes" : "No") << "\n\n";
    }

    void printLibraryStats() const {
        int available = 0, borrowed = 0, damaged = 0;
        for (const auto& b : books) {
            if (b.status == BookStatus::AVAILABLE) ++available;
            else if (b.status == BookStatus::BORROWED)  ++borrowed;
            else if (b.status == BookStatus::DAMAGED)   ++damaged;
        }

        std::cout << "\n══════════════════════════════════════════\n";
        std::cout << "  LIBRARY STATS — " << libraryName << "\n";
        std::cout << "══════════════════════════════════════════\n";
        std::cout << "  Total Books    : " << books.size()   << "\n";
        std::cout << "  Available      : " << available      << "\n";
        std::cout << "  Borrowed       : " << borrowed       << "\n";
        std::cout << "  Damaged        : " << damaged        << "\n";
        std::cout << "  Total Members  : " << members.size() << "\n";
        std::cout << "  Total Records  : " << records.size() << "\n";
        std::cout << "  Current Day    : " << currentDay     << "\n";
        std::cout << "══════════════════════════════════════════\n\n";
    }

    void printOverdueBooks() const {
        std::cout << "\nOVERDUE BOOKS (Day " << currentDay << "):\n";
        bool any = false;
        for (const auto& r : records) {
            if (!r.returned && r.daysOverdue(currentDay) > 0) {
                any = true;
                auto mit = memberIndex.find(r.memberId);
                auto bit = bookIndex.find(r.bookId);
                if (mit != memberIndex.end() && bit != bookIndex.end()) {
                    const Member& m = members[mit->second];
                    const Book&   b = books[bit->second];
                    int overdue     = r.daysOverdue(currentDay);
                    double fine     = computeFine(overdue);
                    std::cout << "  " << b.shortInfo()
                              << " — Borrower: " << m.name
                              << " — Overdue: " << overdue << " days"
                              << " — Fine: $" << std::fixed << std::setprecision(2) << fine << "\n";
                }
            }
        }
        if (!any) std::cout << "  No overdue books.\n";
        std::cout << "\n";
    }

    std::vector<Book> searchByAuthor(const std::string& author) const {
        std::vector<Book> result;
        for (const auto& b : books)
            if (b.author.find(author) != std::string::npos)
                result.push_back(b);
        return result;
    }

    std::vector<Book> searchByGenre(const std::string& genre) const {
        std::vector<Book> result;
        for (const auto& b : books)
            if (b.genre == genre && b.isAvailable())
                result.push_back(b);
        return result;
    }

    void exportMemberCSV(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open())
            throw std::runtime_error("Cannot open: " + filename);
        file << "MemberID,Name,Email,Type,Fines,Status\n";
        for (const auto& m : members) {
            file << m.memberId << ","
                 << m.name     << ","
                 << m.email    << ","
                 << memberTypeToString(m.type) << ","
                 << std::fixed << std::setprecision(2) << m.totalFines << ","
                 << (m.isActive ? "Active" : "Inactive") << "\n";
        }
        std::cout << "Exported to: " << filename << "\n";
    }

    size_t totalBooks()   const { return books.size(); }
    size_t totalMembers() const { return members.size(); }
};

// ─── Main ─────────────────────────────────────────────────────────────────────
int main() {
    LibrarySystem lib("Central City Library");

    // Add books
    int b1 = lib.addBook("Clean Code",              "Robert Martin", "978-0132350884", "Programming", 2008);
    int b2 = lib.addBook("The Pragmatic Programmer","Andrew Hunt",   "978-0201616224", "Programming", 1999);
    int b3 = lib.addBook("Design Patterns",         "Gang of Four",  "978-0201633610", "Programming", 1994);
    int b4 = lib.addBook("Dune",                    "Frank Herbert", "978-0441013593", "Sci-Fi",      1965);
    int b5 = lib.addBook("1984",                    "George Orwell", "978-0451524935", "Fiction",     1949);
    int b6 = lib.addBook("Sapiens",                 "Yuval Harari",  "978-0062316097", "History",     2011);

    // Register members
    int m1 = lib.registerMember("Alice Kumar",  "alice@lib.com",  MemberType::STUDENT);
    int m2 = lib.registerMember("Bob Patel",    "bob@lib.com",    MemberType::FACULTY);
    int m3 = lib.registerMember("Carol Singh",  "carol@lib.com",  MemberType::PUBLIC);
    int m4 = lib.registerMember("David Mehta",  "david@lib.com",  MemberType::STUDENT);

    // Borrow books
    int r1 = lib.borrowBook(m1, b1);
    int r2 = lib.borrowBook(m1, b2);
    int r3 = lib.borrowBook(m2, b3);
    int r4 = lib.borrowBook(m3, b4);
    int r5 = lib.borrowBook(m4, b5);

    // Advance time — simulate overdue
    lib.advanceDay(20);

    // Return some books
    double fine1 = lib.returnBook(m1, b1);
    double fine3 = lib.returnBook(m2, b3);

    std::cout << "Alice's fine: $" << std::fixed << std::setprecision(2) << fine1 << "\n";
    std::cout << "Bob's fine:   $" << std::fixed << std::setprecision(2) << fine3 << "\n";

    // Pay fine
    lib.payFine(m1, fine1);

    // Reports
    lib.printMemberReport(m1);
    lib.printMemberReport(m3);
    lib.printLibraryStats();
    lib.printOverdueBooks();

    // Search
    std::cout << "Books by 'Martin':\n";
    for (const auto& b : lib.searchByAuthor("Martin"))
        std::cout << "  " << b.shortInfo() << "\n";

    std::cout << "\nAvailable Sci-Fi:\n";
    for (const auto& b : lib.searchByGenre("Sci-Fi"))
        std::cout << "  " << b.shortInfo() << "\n";

    // Export
    try { lib.exportMemberCSV("members.csv"); }
    catch (const std::exception& e) { std::cerr << "Error: " << e.what() << "\n"; }

    return 0;
}
