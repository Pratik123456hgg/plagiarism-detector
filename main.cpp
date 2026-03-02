#include "detector.h"
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <file1.txt> <file2.txt>"
              << std::endl;
    return 1;
  }

  std::string file1 = argv[1];
  std::string file2 = argv[2];

  std::string text1 = Detector::readFile(file1);
  std::string text2 = Detector::readFile(file2);

  if (text1.empty() && text2.empty()) {
    std::cout << "Both files are empty or could not be read." << std::endl;
    return 1;
  }

  std::string cleaned1 = Detector::cleanText(text1);
  std::string cleaned2 = Detector::cleanText(text2);

  auto freq1 = Detector::getWordFrequencies(cleaned1);
  auto freq2 = Detector::getWordFrequencies(cleaned2);

  auto result = Detector::calculateDetailedSimilarity(freq1, freq2);
  double similarityPercentage = result.score * 100.0;

  // Threshold for plagiarism
  const double PLAGIARISM_THRESHOLD = 60.0;
  bool isPlagiarized = similarityPercentage >= PLAGIARISM_THRESHOLD;

  std::string reportFile = "report.html";
  std::ofstream htmlOut(reportFile);
  if (!htmlOut.is_open()) {
    std::cerr << "Error: Could not create HTML report file." << std::endl;
    return 1;
  }

  htmlOut
      << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
      << "    <meta charset=\"UTF-8\">\n    <title>Plagiarism Report</title>\n"
      << "    <style>\n"
      << "        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, "
         "sans-serif; background-color: #f4f7f6; color: #333; margin: 0; "
         "padding: 2rem; }\n"
      << "        .container { max-width: 600px; margin: 0 auto; background: "
         "#fff; padding: 2.5rem; border-radius: 12px; box-shadow: 0 10px 30px "
         "rgba(0,0,0,0.1); }\n"
      << "        h1 { text-align: center; color: #2c3e50; border-bottom: 2px "
         "solid #eee; padding-bottom: 1rem; margin-top: 0; }\n"
      << "        .info { margin: 2rem 0; font-size: 1.1rem; }\n"
      << "        .info p { margin: 0.8rem 0; }\n"
      << "        .info strong { color: #555; display: inline-block; width: "
         "60px; }\n"
      << "        .file-name { background: #f0f4f8; padding: 0.2rem 0.5rem; "
         "border-radius: 4px; font-family: monospace; color: #0277bd; }\n"
      << "        .percentage { font-size: 2.5rem; font-weight: bold; "
         "text-align: center; margin: 2rem 0; color: #1976d2; }\n"
      << "        .result { padding: 1.5rem; text-align: center; "
         "border-radius: 8px; margin-top: 2rem; }\n"
      << "        .yes { background-color: #ffebee; color: #c62828; border: "
         "2px solid #ef9a9a; }\n"
      << "        .no { background-color: #e8f5e9; color: #2e7d32; border: 2px "
         "solid #a5d6a7; }\n"
      << "        .result h2 { margin: 0; font-size: 1.8rem; }\n"
      << "        .common-terms { margin-top: 2rem; background: #fafafa; "
         "padding: 1.5rem; border-radius: 8px; border: 1px solid #eaeaea; }\n"
      << "        .common-terms h3 { margin-top: 0; color: #444; font-size: "
         "1.3rem; margin-bottom: 1rem; }\n"
      << "        .term { display: inline-block; background: #e0f7fa; color: "
         "#006064; padding: 0.3rem 0.6rem; margin: 0.2rem; border-radius: 6px; "
         "font-size: 0.95rem; }\n"
      << "    </style>\n</head>\n<body>\n"
      << "    <div class=\"container\">\n"
      << "        <h1>Plagiarism Analysis Report</h1>\n"
      << "        <div class=\"info\">\n"
      << "            <p><strong>File 1:</strong> <span class=\"file-name\">"
      << file1 << "</span></p>\n"
      << "            <p><strong>File 2:</strong> <span class=\"file-name\">"
      << file2 << "</span></p>\n"
      << "        </div>\n"
      << "        <div class=\"percentage\">\n"
      << "            Similarity: " << std::fixed << std::setprecision(2)
      << similarityPercentage << "%\n"
      << "        </div>\n"
      << "        <div class=\"result " << (isPlagiarized ? "yes" : "no")
      << "\">\n"
      << "            <h2>Plagiarism Detected: "
      << (isPlagiarized ? "YES" : "NO") << "</h2>\n"
      << "        </div>\n"
      << "        <div class=\"common-terms\">\n"
      << "            <h3>Overlapping Terms:</h3>\n"
      << "            <p>";

  int count = 0;
  for (const auto &term : result.commonTerms) {
    if (count++ >= 30) {
      htmlOut << " <span class=\"term\">...</span>";
      break;
    } // limit to 30 terms
    htmlOut << "<span class=\"term\">" << term << "</span> ";
  }
  if (result.commonTerms.empty()) {
    htmlOut << "None found.";
  }

  htmlOut << "</p>\n        </div>\n"
          << "    </div>\n</body>\n</html>\n";

  htmlOut.close();
  std::cout << "Report generated successfully: " << reportFile << std::endl;

  return 0;
}
