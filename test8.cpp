/*
 * Academic Record Processing System
 * Modified Submission
 * Author: Copy Student
 * Description: Processes academic records, computes results,
 *              builds transcripts, and evaluates academic standing.
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
#include <memory>

// ─── Configuration Values ────────────────────────────────────────────────────
const double POINTS_SCALE      = 4.0;   // renamed from GPA_SCALE
const int    STUDENT_LIMIT     = 500;   // renamed from MAX_STUDENTS
const int    SUBJECT_LIMIT     = 20;    // renamed from MAX_COURSES
const double MINIMUM_PASS      = 50.0;  // renamed from PASS_THRESHOLD
const double DISTINCTION_SCORE = 75.0;  // renamed from DISTINCTION_MARK
const double HIGH_DIST_SCORE   = 85.0;  // renamed from HIGH_DISTINCTION

// ─── Score Conversion Functions ──────────────────────────────────────────────
std::string scoreToLetter(double mark) {    // renamed from letterGrade
    if (mark >= 90) return "A+";
    if (mark >= 85) return "A";
    if (mark >= 80) return "A-";
    if (mark >= 75) return "B+";
    if (mark >= 70) return "B";
    if (mark >= 65) return "B-";
    if (mark >= 60) return "C+";
    if (mark >= 55) return "C";
    if (mark >= 50) return "C-";
    if (mark >= 45) return "D";
    return "F";
}

double letterToPoints(const std::string& ltr) {    // renamed from gradeToGPA
    static const std::map<std::string, double> pointsMap = {
        {"A+", 4.0}, {"A", 4.0}, {"A-", 3.7},
        {"B+", 3.3}, {"B", 3.0}, {"B-", 2.7},
        {"C+", 2.3}, {"C", 2.0}, {"C-", 1.7},
        {"D",  1.0}, {"F", 0.0}
    };
    auto entry = pointsMap.find(ltr);
    return (entry != pointsMap.end()) ? entry->second : 0.0;
}

// ─── Data Models ─────────────────────────────────────────────────────────────
struct Subject {         // renamed from Course
    std::string subjectCode;    // renamed from code
    std::string subjectName;    // renamed from name
    int         units;          // renamed from creditHours
    double      mark;           // renamed from score
    std::string letterMark;     // renamed from grade
    double      pointsEarned;   // renamed from gpaPoints

    Subject(std::string sc, std::string sn, int u, double m)
        : subjectCode(std::move(sc)), subjectName(std::move(sn)), units(u), mark(m) {
        letterMark   = scoreToLetter(mark);
        pointsEarned = letterToPoints(letterMark);
    }
};

struct AcademicRecord {      // renamed from Student
    int                    studentNo;     // renamed from id
    std::string            givenName;     // renamed from firstName
    std::string            familyName;    // renamed from lastName
    std::string            contactEmail;  // renamed from email
    int                    enrollYear;    // renamed from yearLevel
    std::string            programme;     // renamed from major
    std::vector<Subject>   subjects;      // renamed from courses
    bool                   enrolled;      // renamed from isActive

    AcademicRecord(int sn, std::string gn, std::string fn,
                   std::string em, int yr, std::string prog)
        : studentNo(sn), givenName(std::move(gn)), familyName(std::move(fn)),
          contactEmail(std::move(em)), enrollYear(yr), programme(std::move(prog)),
          enrolled(true) {}

    std::string completeName() const {        // renamed from fullName
        return givenName + " " + familyName;
    }

    double sumUnits() const {                 // renamed from totalCredits
        double total = 0;
        for (const auto& s : subjects) total += s.units;
        return total;
    }

    double computeGPA() const {               // renamed from weightedGPA
        double totalPts  = 0;
        double totalUnits = 0;
        for (const auto& s : subjects) {
            totalPts   += s.pointsEarned * s.units;
            totalUnits += s.units;
        }
        return (totalUnits > 0) ? (totalPts / totalUnits) : 0.0;
    }

    double avgMark() const {                  // renamed from averageScore
        if (subjects.empty()) return 0.0;
        double sum = 0;
        for (const auto& s : subjects) sum += s.mark;
        return sum / subjects.size();
    }

    bool qualifiesForHonours() const {        // renamed from isOnDeansList
        return computeGPA() >= 3.5;
    }

    bool academicWarning() const {            // renamed from isAtRisk
        return computeGPA() < 2.0 || avgMark() < MINIMUM_PASS;
    }

    void registerSubject(const Subject& sub) {      // renamed from addCourse
        if (subjects.size() >= SUBJECT_LIMIT)
            throw std::overflow_error("Subject limit reached for: " + completeName());
        subjects.push_back(sub);
    }
};

// ─── Statistical Analysis ─────────────────────────────────────────────────────
struct AnalysisResult {      // renamed from ClassStats
    double avg;              // renamed from mean
    double midPoint;         // renamed from median
    double deviation;        // renamed from stdDev
    double topScore;         // renamed from highest
    double bottomScore;      // renamed from lowest
    int    count;            // renamed from totalStudents
    int    passCount;        // renamed from passing
    int    failCount;        // renamed from failing

    AnalysisResult() : avg(0), midPoint(0), deviation(0),
                       topScore(0), bottomScore(100),
                       count(0), passCount(0), failCount(0) {}
};

AnalysisResult analyseScores(const std::vector<double>& marks) {   // renamed from computeStats
    AnalysisResult result;
    if (marks.empty()) return result;

    result.count       = static_cast<int>(marks.size());
    result.topScore    = *std::max_element(marks.begin(), marks.end());
    result.bottomScore = *std::min_element(marks.begin(), marks.end());
    result.avg         = std::accumulate(marks.begin(), marks.end(), 0.0) / marks.size();

    std::vector<double> arranged = marks;
    std::sort(arranged.begin(), arranged.end());
    size_t centre   = arranged.size() / 2;
    result.midPoint = (arranged.size() % 2 == 0)
                    ? (arranged[centre - 1] + arranged[centre]) / 2.0
                    : arranged[centre];

    double variance = 0;
    for (double m : marks) variance += (m - result.avg) * (m - result.avg);
    result.deviation = std::sqrt(variance / marks.size());

    for (double m : marks) {
        if (m >= MINIMUM_PASS) ++result.passCount;
        else                   ++result.failCount;
    }

    return result;
}

// ─── Academic Record System ───────────────────────────────────────────────────
class RecordSystem {         // renamed from GradeManager
private:
    std::vector<AcademicRecord>    records;           // renamed from students
    std::map<int, size_t>          recordIndex;       // renamed from idIndex
    std::map<std::string, int>     subjectTally;      // renamed from courseEnrollment
    int                            nextStudentNo;     // renamed from nextId
    std::string                    orgName;           // renamed from institutionName

    bool recordExists(int sn) const {
        return recordIndex.find(sn) != recordIndex.end();
    }

    AcademicRecord& fetchRecord(int sn) {
        auto it = recordIndex.find(sn);
        if (it == recordIndex.end())
            throw std::invalid_argument("Student number not found: " + std::to_string(sn));
        return records[it->second];
    }

public:
    explicit RecordSystem(std::string org = "Default Institute")
        : nextStudentNo(1001), orgName(std::move(org)) {}

    int registerStudent(const std::string& given, const std::string& family,
                        const std::string& email, int year, const std::string& prog) {
        if (records.size() >= STUDENT_LIMIT)
            throw std::overflow_error("Maximum student capacity reached.");

        int sn = nextStudentNo++;
        records.emplace_back(sn, given, family, email, year, prog);
        recordIndex[sn] = records.size() - 1;
        return sn;
    }

    void recordResult(int sn, const std::string& code,
                      const std::string& subjectName, int units, double mark) {
        if (mark < 0 || mark > 100)
            throw std::out_of_range("Mark must be between 0 and 100.");
        AcademicRecord& r = fetchRecord(sn);
        r.registerSubject(Subject(code, subjectName, units, mark));
        subjectTally[code]++;
    }

    void suspendStudent(int sn) {
        fetchRecord(sn).enrolled = false;
    }

    void printTranscript(int sn) const {         // renamed from printStudentReport
        auto it = recordIndex.find(sn);
        if (it == recordIndex.end()) {
            std::cout << "Record not found.\n";
            return;
        }
        const AcademicRecord& r = records[it->second];

        std::cout << "\n╔══════════════════════════════════════╗\n";
        std::cout << "║       ACADEMIC TRANSCRIPT            ║\n";
        std::cout << "╚══════════════════════════════════════╝\n";
        std::cout << "  Name      : " << r.completeName()   << "\n";
        std::cout << "  Number    : " << r.studentNo        << "\n";
        std::cout << "  Email     : " << r.contactEmail     << "\n";
        std::cout << "  Year      : " << r.enrollYear       << "\n";
        std::cout << "  Programme : " << r.programme        << "\n";
        std::cout << "  Status    : " << (r.enrolled ? "Enrolled" : "Suspended") << "\n";
        std::cout << "  ─────────────────────────────────────\n";
        std::cout << std::left
                  << std::setw(8)  << "Code"
                  << std::setw(22) << "Subject"
                  << std::setw(8)  << "Units"
                  << std::setw(8)  << "Mark"
                  << std::setw(6)  << "Grade"
                  << "Points\n";
        std::cout << "  ─────────────────────────────────────\n";

        for (const auto& s : r.subjects) {
            std::cout << std::left
                      << std::setw(8)  << s.subjectCode
                      << std::setw(22) << s.subjectName
                      << std::setw(8)  << s.units
                      << std::setw(8)  << std::fixed << std::setprecision(1) << s.mark
                      << std::setw(6)  << s.letterMark
                      << s.pointsEarned << "\n";
        }

        std::cout << "  ─────────────────────────────────────\n";
        std::cout << "  Cumulative GPA       : "
                  << std::fixed << std::setprecision(2) << r.computeGPA() << "\n";
        std::cout << "  Average Mark         : "
                  << std::fixed << std::setprecision(1) << r.avgMark() << "%\n";
        std::cout << "  Total Units          : " << r.sumUnits() << "\n";
        if (r.qualifiesForHonours())
            std::cout << "  ★  HONOURS STANDING\n";
        if (r.academicWarning())
            std::cout << "  ⚠  ACADEMIC WARNING\n";
        std::cout << "\n";
    }

    void printSummary() const {          // renamed from printClassReport
        std::cout << "\n══════════════════════════════════════════\n";
        std::cout << "  SUMMARY REPORT — " << orgName << "\n";
        std::cout << "══════════════════════════════════════════\n";
        std::cout << "  Total registered : " << records.size() << "\n";

        std::vector<double> gpaList;
        int honoursCount = 0, warningCount = 0;
        for (const auto& r : records) {
            if (!r.enrolled) continue;
            gpaList.push_back(r.computeGPA());
            if (r.qualifiesForHonours()) ++honoursCount;
            if (r.academicWarning())     ++warningCount;
        }

        AnalysisResult stats = analyseScores(gpaList);
        std::cout << "  Mean GPA         : " << std::fixed << std::setprecision(2) << stats.avg        << "\n";
        std::cout << "  Median GPA       : " << std::fixed << std::setprecision(2) << stats.midPoint   << "\n";
        std::cout << "  Std Dev GPA      : " << std::fixed << std::setprecision(2) << stats.deviation  << "\n";
        std::cout << "  Highest GPA      : " << std::fixed << std::setprecision(2) << stats.topScore   << "\n";
        std::cout << "  Lowest GPA       : " << std::fixed << std::setprecision(2) << stats.bottomScore << "\n";
        std::cout << "  Honours Standing : " << honoursCount  << " students\n";
        std::cout << "  Academic Warning : " << warningCount  << " students\n";
        std::cout << "══════════════════════════════════════════\n\n";
    }

    std::vector<AcademicRecord> topRanked(int n = 5) const {     // renamed from getTopStudents
        std::vector<AcademicRecord> ranked = records;
        std::sort(ranked.begin(), ranked.end(),
                  [](const AcademicRecord& a, const AcademicRecord& b) {
                      return a.computeGPA() > b.computeGPA();
                  });
        if (n > static_cast<int>(ranked.size())) n = static_cast<int>(ranked.size());
        return std::vector<AcademicRecord>(ranked.begin(), ranked.begin() + n);
    }

    std::vector<AcademicRecord> warningList() const {            // renamed from getAtRiskStudents
        std::vector<AcademicRecord> list;
        for (const auto& r : records)
            if (r.enrolled && r.academicWarning()) list.push_back(r);
        return list;
    }

    void saveToCSV(const std::string& filepath) const {          // renamed from exportCSV
        std::ofstream outFile(filepath);
        if (!outFile.is_open())
            throw std::runtime_error("Cannot open file: " + filepath);

        outFile << "StudentNo,GivenName,FamilyName,Email,Year,Programme,GPA,AverageMark,Status\n";
        for (const auto& r : records) {
            outFile << r.studentNo        << ","
                    << r.givenName        << ","
                    << r.familyName       << ","
                    << r.contactEmail     << ","
                    << r.enrollYear       << ","
                    << r.programme        << ","
                    << std::fixed << std::setprecision(2) << r.computeGPA()  << ","
                    << std::fixed << std::setprecision(1) << r.avgMark()     << ","
                    << (r.enrolled ? "Enrolled" : "Suspended") << "\n";
        }
        std::cout << "Data saved to: " << filepath << "\n";
    }

    size_t totalRecords() const { return records.size(); }      // renamed from totalStudents
};

// ─── Entry Point ─────────────────────────────────────────────────────────────
int main() {
    RecordSystem rs("National Institute of Technology");

    // Register students (same people, same data, slightly different names)
    int alice = rs.registerStudent("Alice", "Johnson", "alice@nit.edu", 2, "Computer Science");
    int bob   = rs.registerStudent("Bob",   "Smith",   "bob@nit.edu",   3, "Mathematics");
    int carol = rs.registerStudent("Carol", "White",   "carol@nit.edu", 1, "Physics");
    int david = rs.registerStudent("David", "Brown",   "david@nit.edu", 4, "Engineering");
    int eva   = rs.registerStudent("Eva",   "Davis",   "eva@nit.edu",   2, "Chemistry");

    // Alice
    rs.recordResult(alice, "CS101", "Intro to Programming",    3, 92.5);
    rs.recordResult(alice, "CS201", "Data Structures",         3, 88.0);
    rs.recordResult(alice, "MA101", "Calculus I",              4, 95.0);
    rs.recordResult(alice, "PH101", "Physics I",               3, 87.5);
    rs.recordResult(alice, "ENG01", "Technical Writing",       2, 91.0);

    // Bob
    rs.recordResult(bob, "MA201", "Linear Algebra",            4, 72.0);
    rs.recordResult(bob, "MA301", "Differential Equations",    4, 68.5);
    rs.recordResult(bob, "CS101", "Intro to Programming",      3, 65.0);
    rs.recordResult(bob, "ST101", "Statistics I",              3, 74.0);
    rs.recordResult(bob, "PH101", "Physics I",                 3, 70.0);

    // Carol
    rs.recordResult(carol, "PH101", "Physics I",               3, 48.0);
    rs.recordResult(carol, "MA101", "Calculus I",              4, 45.0);
    rs.recordResult(carol, "CH101", "Chemistry I",             3, 52.0);
    rs.recordResult(carol, "ENG01", "Technical Writing",       2, 55.0);

    // David
    rs.recordResult(david, "EN401", "Senior Design Project",   4, 96.0);
    rs.recordResult(david, "EN301", "Thermodynamics",          3, 89.0);
    rs.recordResult(david, "MA301", "Differential Equations",  4, 82.0);
    rs.recordResult(david, "CS201", "Data Structures",         3, 91.0);
    rs.recordResult(david, "PH201", "Physics II",              3, 85.0);

    // Eva
    rs.recordResult(eva, "CH101", "Chemistry I",               3, 78.0);
    rs.recordResult(eva, "CH201", "Organic Chemistry",         3, 82.5);
    rs.recordResult(eva, "MA101", "Calculus I",                4, 76.0);
    rs.recordResult(eva, "BIO01", "Biology I",                 3, 80.0);
    rs.recordResult(eva, "ENG01", "Technical Writing",         2, 85.0);

    // Output transcripts
    rs.printTranscript(alice);
    rs.printTranscript(carol);
    rs.printSummary();

    // Top performers
    std::cout << "TOP 3 STUDENTS:\n";
    for (const auto& r : rs.topRanked(3))
        std::cout << "  " << r.completeName()
                  << " — GPA: " << std::fixed << std::setprecision(2) << r.computeGPA() << "\n";

    // Warning list
    std::cout << "\nACADEMIC WARNING LIST:\n";
    for (const auto& r : rs.warningList())
        std::cout << "  " << r.completeName()
                  << " — GPA: " << std::fixed << std::setprecision(2) << r.computeGPA() << "\n";

    // Save
    try { rs.saveToCSV("academic_records.csv"); }
    catch (const std::exception& e) { std::cerr << "Save error: " << e.what() << "\n"; }

    return 0;
}
