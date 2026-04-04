/*
 * Book Rental Management System
 * Submitted Implementation
 * Description: Handles book inventory, user accounts, rental tracking,
 *              penalty computation, and catalogue search.
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

// ─── Configuration ────────────────────────────────────────────────────────────
const int    LOAN_PERIOD       = 14;       // same value, renamed
const double DAILY_PENALTY     = 2.50;     // same value, renamed
const int    MAX_LOANS_USER    = 5;        // same value, renamed
const int    USER_CAPACITY     = 300;      // same value, renamed
const int    CATALOGUE_SIZE    = 1000;     // same value, renamed

// ─── Enums ────────────────────────────────────────────────────────────────────
// STRUCTURAL DIFFERENCE 1: Added extra enum value (LOST) and used switch instead of if-chains
enum class ItemStatus {
    AVAILABLE,
    ONLOAN,
    RESERVED,
    DAMAGED,
    LOST             // ← extra value, breaks AST branch count
};

enum class AccountType {
    STUDENT,
    STAFF,           // renamed from FACULTY
    COMMUNITY        // renamed from PUBLIC
};

// ─── Utility Functions ────────────────────────────────────────────────────────
// STRUCTURAL DIFFERENCE 2: switch statement instead of if-chain → different AST node types
std::string itemStatusLabel(ItemStatus s) {
    switch (s) {                              // switch replaces if-chain
        case ItemStatus::AVAILABLE: return "Available";
        case ItemStatus::ONLOAN:    return "On Loan";
        case ItemStatus::RESERVED:  return "Reserved";
        case ItemStatus::DAMAGED:   return "Damaged";
        case ItemStatus::LOST:      return "Lost";
        default:                    return "Unknown";
    }
}

// STRUCTURAL DIFFERENCE 3: switch statement instead of if-chain
std::string accountTypeLabel(AccountType t) {
    switch (t) {
        case AccountType::STUDENT:   return "Student";
        case AccountType::STAFF:     return "Staff";
        case AccountType::COMMUNITY: return "Community";
        default:                     return "Unknown";
    }
}

// STRUCTURAL DIFFERENCE 4: added tiered penalty logic (extra if-else branches)
double calculatePenalty(int daysLate) {
    if (daysLate <= 0)  return 0.0;
    if (daysLate <= 7)  return daysLate * DAILY_PENALTY;           // standard rate
    if (daysLate <= 14) return 7 * DAILY_PENALTY + (daysLate - 7) * DAILY_PENALTY * 1.5;  // 1.5x after 7 days
    return 7 * DAILY_PENALTY + 7 * DAILY_PENALTY * 1.5            // 2x after 14 days
           + (daysLate - 14) * DAILY_PENALTY * 2.0;
}

// ─── Catalogue Item ───────────────────────────────────────────────────────────
struct CatalogueItem {        // renamed from Book
    int         itemId;       // renamed from bookId
    std::string title;
    std::string author;
    std::string isbn;
    std::string category;     // renamed from genre
    int         yearPublished; // renamed from publishYear
    ItemStatus  itemStatus;   // renamed from status
    int         loanedToUserId; // renamed from borrowedByMemberId
    int         loanStartDay;   // renamed from borrowDay

    CatalogueItem(int id, std::string t, std::string a,
                  std::string i, std::string cat, int yr)
        : itemId(id), title(std::move(t)), author(std::move(a)),
          isbn(std::move(i)), category(std::move(cat)), yearPublished(yr),
          itemStatus(ItemStatus::AVAILABLE), loanedToUserId(-1), loanStartDay(0) {}

    bool isAvailable() const {
        return itemStatus == ItemStatus::AVAILABLE;
    }

    std::string summary() const {     // renamed from shortInfo
        return "[" + std::to_string(itemId) + "] " + title + " by " + author;
    }
};

// ─── User Account ─────────────────────────────────────────────────────────────
struct UserAccount {            // renamed from Member
    int                  userId;       // renamed from memberId
    std::string          fullName;     // renamed from name
    std::string          emailAddr;    // renamed from email
    AccountType          accountType;  // renamed from type
    std::vector<int>     loanedItemIds; // renamed from borrowedBookIds
    double               outstandingFines; // renamed from totalFines
    bool                 accountActive;    // renamed from isActive

    // STRUCTURAL DIFFERENCE 5: constructor uses different initializer order + extra validation
    UserAccount(int id, std::string fn, std::string em, AccountType at)
        : userId(id), fullName(std::move(fn)), emailAddr(std::move(em)),
          accountType(at), outstandingFines(0.0), accountActive(true) {
        if (fullName.empty())
            throw std::invalid_argument("User name cannot be empty.");   // extra validation block
    }

    bool eligibleForLoan() const {    // renamed from canBorrow
        return accountActive
            && loanedItemIds.size() < MAX_LOANS_USER
            && outstandingFines < 10.0;
    }

    void loanItem(int itemId) {       // renamed from borrowBook
        if (loanedItemIds.size() >= MAX_LOANS_USER)
            throw std::overflow_error("Loan limit reached for: " + fullName);
        loanedItemIds.push_back(itemId);
    }

    void returnItem(int itemId) {     // renamed from returnBook
        auto pos = std::find(loanedItemIds.begin(), loanedItemIds.end(), itemId);
        if (pos == loanedItemIds.end())
            throw std::invalid_argument("Item not on loan to this user.");
        loanedItemIds.erase(pos);
    }

    void applyPenalty(double amount) {  // renamed from addFine
        outstandingFines += amount;
    }

    void settleAccount() {              // renamed from clearFines
        outstandingFines = 0.0;
    }
};

// ─── Loan Record ──────────────────────────────────────────────────────────────
struct LoanRecord {           // renamed from BorrowRecord
    int    loanId;            // renamed from recordId
    int    userId;            // renamed from memberId
    int    itemId;            // renamed from bookId
    int    issueDay;          // renamed from borrowDay
    int    dueDay;            // NEW FIELD ← extra field breaks AST
    int    closeDay;          // renamed from returnDay
    bool   closed;            // renamed from returned
    double penalty;           // renamed from fine

    // STRUCTURAL DIFFERENCE 6: constructor computes dueDay — extra assignment node
    LoanRecord(int lid, int uid, int iid, int day)
        : loanId(lid), userId(uid), itemId(iid),
          issueDay(day), dueDay(day + LOAN_PERIOD),   // ← extra computation
          closeDay(-1), closed(false), penalty(0.0) {}

    int daysLate(int today) const {   // renamed from daysOverdue
        int elapsed = today - issueDay;
        return std::max(0, elapsed - LOAN_PERIOD);
    }
};

// ─── Rental Management System ─────────────────────────────────────────────────
class RentalSystem {             // renamed from LibrarySystem
private:
    std::vector<CatalogueItem>  catalogue;       // renamed from books
    std::vector<UserAccount>    users;           // renamed from members
    std::vector<LoanRecord>     loanHistory;     // renamed from records
    std::map<int, size_t>       itemIndex;       // renamed from bookIndex
    std::map<int, size_t>       userIndex;       // renamed from memberIndex
    int                         nextItemId;      // renamed from nextBookId
    int                         nextUserId;      // renamed from nextMemberId
    int                         nextLoanId;      // renamed from nextRecordId
    int                         today;           // renamed from currentDay
    std::string                 branchName;      // renamed from libraryName

    CatalogueItem& lookupItem(int id) {          // renamed from getBook
        auto it = itemIndex.find(id);
        if (it == itemIndex.end())
            throw std::invalid_argument("Item ID not found: " + std::to_string(id));
        return catalogue[it->second];
    }

    UserAccount& lookupUser(int id) {            // renamed from getMember
        auto it = userIndex.find(id);
        if (it == userIndex.end())
            throw std::invalid_argument("User ID not found: " + std::to_string(id));
        return users[it->second];
    }

public:
    explicit RentalSystem(std::string branch = "Main Branch")
        : nextItemId(1), nextUserId(101),
          nextLoanId(5001), today(1),
          branchName(std::move(branch)) {}

    // ── Catalogue Operations ─────────────────────────────────────────────────
    int addItem(const std::string& title, const std::string& author,
                const std::string& isbn,  const std::string& category, int year) {
        if (catalogue.size() >= CATALOGUE_SIZE)
            throw std::overflow_error("Catalogue full.");
        int id = nextItemId++;
        catalogue.emplace_back(id, title, author, isbn, category, year);
        itemIndex[id] = catalogue.size() - 1;
        return id;
    }

    // STRUCTURAL DIFFERENCE 7: extra status check — can mark LOST in addition to DAMAGED
    void removeItem(int itemId) {
        CatalogueItem& item = lookupItem(itemId);
        if (item.itemStatus == ItemStatus::ONLOAN)
            throw std::logic_error("Cannot remove an item currently on loan.");
        if (item.itemStatus == ItemStatus::LOST)       // extra branch
            throw std::logic_error("Item already marked as lost.");
        item.itemStatus = ItemStatus::DAMAGED;
    }

    void markAsLost(int itemId) {                      // NEW METHOD ← extra method node
        CatalogueItem& item = lookupItem(itemId);
        item.itemStatus = ItemStatus::LOST;
    }

    // ── User Operations ───────────────────────────────────────────────────────
    int registerUser(const std::string& name,
                     const std::string& email, AccountType type) {
        if (users.size() >= USER_CAPACITY)
            throw std::overflow_error("User capacity reached.");
        int id = nextUserId++;
        users.emplace_back(id, name, email, type);
        userIndex[id] = users.size() - 1;
        return id;
    }

    void suspendUser(int userId) {              // renamed from deactivateMember
        lookupUser(userId).accountActive = false;
    }

    // ── Loan / Return ─────────────────────────────────────────────────────────
    int issueItem(int userId, int itemId) {     // renamed from borrowBook
        UserAccount&   u = lookupUser(userId);
        CatalogueItem& item = lookupItem(itemId);

        if (!u.eligibleForLoan())
            throw std::logic_error("User not eligible: check fines or loan limit.");
        if (!item.isAvailable())
            throw std::logic_error("Item is not available.");

        u.loanItem(itemId);
        item.itemStatus     = ItemStatus::ONLOAN;
        item.loanedToUserId = userId;
        item.loanStartDay   = today;

        int lid = nextLoanId++;
        loanHistory.emplace_back(lid, userId, itemId, today);
        return lid;
    }

    double closeItem(int userId, int itemId) {   // renamed from returnBook
        UserAccount&   u    = lookupUser(userId);
        CatalogueItem& item = lookupItem(itemId);

        u.returnItem(itemId);
        item.itemStatus     = ItemStatus::AVAILABLE;
        item.loanedToUserId = -1;

        double penalty = 0.0;
        for (auto& rec : loanHistory) {
            if (rec.userId == userId && rec.itemId == itemId && !rec.closed) {
                int late    = rec.daysLate(today);
                penalty     = calculatePenalty(late);
                rec.penalty = penalty;
                rec.closed  = true;
                rec.closeDay = today;
                break;
            }
        }

        if (penalty > 0) u.applyPenalty(penalty);
        return penalty;
    }

    void advanceDay(int days = 1) {
        today += days;
    }

    void settleUserFines(int userId, double amount) {   // renamed from payFine
        UserAccount& u = lookupUser(userId);
        u.outstandingFines = std::max(0.0, u.outstandingFines - amount);
    }

    // ── Reports ───────────────────────────────────────────────────────────────
    void printUserReport(int userId) const {     // renamed from printMemberReport
        auto it = userIndex.find(userId);
        if (it == userIndex.end()) {
            std::cout << "User not found.\n";
            return;
        }
        const UserAccount& u = users[it->second];

        std::cout << "\n╔══════════════════════════════════════╗\n";
        std::cout << "║          USER ACCOUNT REPORT         ║\n";
        std::cout << "╚══════════════════════════════════════╝\n";
        std::cout << "  Name        : " << u.fullName   << "\n";
        std::cout << "  ID          : " << u.userId     << "\n";
        std::cout << "  Email       : " << u.emailAddr  << "\n";
        std::cout << "  Account     : " << accountTypeLabel(u.accountType) << "\n";
        std::cout << "  Fines       : $" << std::fixed << std::setprecision(2) << u.outstandingFines << "\n";
        std::cout << "  Status      : " << (u.accountActive ? "Active" : "Suspended") << "\n";
        std::cout << "  Items Loaned: " << u.loanedItemIds.size() << "\n";
        std::cout << "  Can Loan    : " << (u.eligibleForLoan() ? "Yes" : "No") << "\n\n";
    }

    // STRUCTURAL DIFFERENCE 8: extra category count loop before stats printout
    void printSystemStats() const {             // renamed from printLibraryStats
        int available = 0, onLoan = 0, damaged = 0, lost = 0;
        for (const auto& item : catalogue) {
            if      (item.itemStatus == ItemStatus::AVAILABLE) ++available;
            else if (item.itemStatus == ItemStatus::ONLOAN)    ++onLoan;
            else if (item.itemStatus == ItemStatus::DAMAGED)   ++damaged;
            else if (item.itemStatus == ItemStatus::LOST)      ++lost;   // extra branch
        }

        // STRUCTURAL DIFFERENCE 9: extra category frequency map loop
        std::map<std::string, int> categoryCount;
        for (const auto& item : catalogue)
            categoryCount[item.category]++;

        std::cout << "\n══════════════════════════════════════════\n";
        std::cout << "  SYSTEM STATS — " << branchName << "\n";
        std::cout << "══════════════════════════════════════════\n";
        std::cout << "  Total Items    : " << catalogue.size() << "\n";
        std::cout << "  Available      : " << available  << "\n";
        std::cout << "  On Loan        : " << onLoan     << "\n";
        std::cout << "  Damaged        : " << damaged    << "\n";
        std::cout << "  Lost           : " << lost       << "\n";
        std::cout << "  Total Users    : " << users.size()       << "\n";
        std::cout << "  Total Loans    : " << loanHistory.size() << "\n";
        std::cout << "  Current Day    : " << today              << "\n";

        std::cout << "  Categories     :\n";
        for (const auto& pair : categoryCount)
            std::cout << "    " << pair.first << ": " << pair.second << " items\n";

        std::cout << "══════════════════════════════════════════\n\n";
    }

    void printOverdueItems() const {             // renamed from printOverdueBooks
        std::cout << "\nOVERDUE ITEMS (Day " << today << "):\n";
        bool any = false;
        for (const auto& rec : loanHistory) {
            if (!rec.closed && rec.daysLate(today) > 0) {
                any = true;
                auto uit = userIndex.find(rec.userId);
                auto iit = itemIndex.find(rec.itemId);
                if (uit != userIndex.end() && iit != itemIndex.end()) {
                    const UserAccount&   u    = users[uit->second];
                    const CatalogueItem& item = catalogue[iit->second];
                    int    late    = rec.daysLate(today);
                    double penalty = calculatePenalty(late);
                    std::cout << "  " << item.summary()
                              << " — User: " << u.fullName
                              << " — Late: " << late << " days"
                              << " — Due: " << rec.dueDay
                              << " — Penalty: $" << std::fixed << std::setprecision(2) << penalty << "\n";
                }
            }
        }
        if (!any) std::cout << "  No overdue items.\n";
        std::cout << "\n";
    }

    std::vector<CatalogueItem> findByAuthor(const std::string& author) const {  // renamed
        std::vector<CatalogueItem> result;
        for (const auto& item : catalogue)
            if (item.author.find(author) != std::string::npos)
                result.push_back(item);
        return result;
    }

    // STRUCTURAL DIFFERENCE 10: added sort-by-year in genre/category search
    std::vector<CatalogueItem> findByCategory(const std::string& cat) const {   // renamed
        std::vector<CatalogueItem> result;
        for (const auto& item : catalogue)
            if (item.category == cat && item.isAvailable())
                result.push_back(item);
        std::sort(result.begin(), result.end(),          // extra sort block
                  [](const CatalogueItem& a, const CatalogueItem& b) {
                      return a.yearPublished > b.yearPublished;
                  });
        return result;
    }

    void saveUsersCSV(const std::string& filepath) const {    // renamed from exportMemberCSV
        std::ofstream outFile(filepath);
        if (!outFile.is_open())
            throw std::runtime_error("Cannot open: " + filepath);
        outFile << "UserID,FullName,Email,AccountType,Fines,Status\n";
        for (const auto& u : users) {
            outFile << u.userId      << ","
                    << u.fullName    << ","
                    << u.emailAddr   << ","
                    << accountTypeLabel(u.accountType) << ","
                    << std::fixed << std::setprecision(2) << u.outstandingFines << ","
                    << (u.accountActive ? "Active" : "Suspended") << "\n";
        }
        std::cout << "Saved to: " << filepath << "\n";
    }

    size_t totalItems() const { return catalogue.size(); }    // renamed
    size_t totalUsers() const { return users.size(); }        // renamed
};

// ─── Entry Point ─────────────────────────────────────────────────────────────
int main() {
    RentalSystem rs("Westside Branch");

    // Add catalogue items
    int b1 = rs.addItem("Clean Code",               "Robert Martin", "978-0132350884", "Programming", 2008);
    int b2 = rs.addItem("The Pragmatic Programmer",  "Andrew Hunt",   "978-0201616224", "Programming", 1999);
    int b3 = rs.addItem("Design Patterns",           "Gang of Four",  "978-0201633610", "Programming", 1994);
    int b4 = rs.addItem("Dune",                      "Frank Herbert", "978-0441013593", "Sci-Fi",      1965);
    int b5 = rs.addItem("1984",                      "George Orwell", "978-0451524935", "Fiction",     1949);
    int b6 = rs.addItem("Sapiens",                   "Yuval Harari",  "978-0062316097", "History",     2011);

    // Register users
    int u1 = rs.registerUser("Alice Kumar",  "alice@rent.com",  AccountType::STUDENT);
    int u2 = rs.registerUser("Bob Patel",    "bob@rent.com",    AccountType::STAFF);
    int u3 = rs.registerUser("Carol Singh",  "carol@rent.com",  AccountType::COMMUNITY);
    int u4 = rs.registerUser("David Mehta",  "david@rent.com",  AccountType::STUDENT);

    // Issue items
    int r1 = rs.issueItem(u1, b1);
    int r2 = rs.issueItem(u1, b2);
    int r3 = rs.issueItem(u2, b3);
    int r4 = rs.issueItem(u3, b4);
    int r5 = rs.issueItem(u4, b5);

    // Mark one as lost
    rs.markAsLost(b6);

    // Advance time
    rs.advanceDay(20);

    // Close loans
    double pen1 = rs.closeItem(u1, b1);
    double pen3 = rs.closeItem(u2, b3);

    std::cout << "Alice's penalty: $" << std::fixed << std::setprecision(2) << pen1 << "\n";
    std::cout << "Bob's penalty:   $" << std::fixed << std::setprecision(2) << pen3 << "\n";

    // Settle fines
    rs.settleUserFines(u1, pen1);

    // Reports
    rs.printUserReport(u1);
    rs.printUserReport(u3);
    rs.printSystemStats();
    rs.printOverdueItems();

    // Search
    std::cout << "Items by 'Martin':\n";
    for (const auto& item : rs.findByAuthor("Martin"))
        std::cout << "  " << item.summary() << "\n";

    std::cout << "\nAvailable Sci-Fi (newest first):\n";
    for (const auto& item : rs.findByCategory("Sci-Fi"))
        std::cout << "  " << item.summary() << "\n";

    // Save
    try { rs.saveUsersCSV("users.csv"); }
    catch (const std::exception& e) { std::cerr << "Error: " << e.what() << "\n"; }

    return 0;
}
