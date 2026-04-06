#include "detector.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace fs = std::filesystem;

static std::string basename(const std::string &path) {
  size_t pos = path.find_last_of("/\\");
  return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

static std::string fmt(double v) {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(1) << v;
  return ss.str();
}

static std::string escapeHTML(const std::string &data) {
  std::string buffer;
  buffer.reserve(data.size() * 1.1);
  for (size_t pos = 0; pos != data.size(); ++pos) {
    switch (data[pos]) {
    case '&':
      buffer.append("&amp;");
      break;
    case '\"':
      buffer.append("&quot;");
      break;
    case '\'':
      buffer.append("&apos;");
      break;
    case '<':
      buffer.append("&lt;");
      break;
    case '>':
      buffer.append("&gt;");
      break;
    default:
      buffer.append(&data[pos], 1);
      break;
    }
  }
  return buffer;
}

static std::unordered_set<std::string> extractLines(const std::string &text) {
  std::unordered_set<std::string> lines;
  std::istringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    auto start = line.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
      auto end = line.find_last_not_of(" \t\r\n");
      std::string t = line.substr(start, end - start + 1);
      if (t.length() > 3)
        lines.insert(t);
    }
  }
  return lines;
}

static std::string
getHighlighted(const std::string &text,
               const std::unordered_set<std::string> &otherLines) {
  std::ostringstream res;
  std::istringstream ss(text);
  std::string line;
  int lineNum = 1;
  while (std::getline(ss, line)) {
    std::string t = line;
    auto start = line.find_first_not_of(" \t\r\n");
    bool highlight = false;
    if (start != std::string::npos) {
      auto end = line.find_last_not_of(" \t\r\n");
      t = line.substr(start, end - start + 1);
      if (t.length() > 3 && otherLines.count(t))
        highlight = true;
    }

    res << "<div class=\"code-line" << (highlight ? " hl" : "") << "\">";
    res << "<span class=\"ln\">" << lineNum++ << "</span>";
    res << "<span class=\"code-content\">" << escapeHTML(line) << "</span>";
    res << "</div>\n";
  }
  return res.str();
}

// Forward declaration for batch report
void generateBatchReport(const Detector::BatchSimilarityMatrix &batchResult);

// Helper: Generate 2-file report (existing mode, unchanged)
void generateTwoFileReport(const std::string &file1, const std::string &file2) {
  std::string text1 = Detector::readFile(file1);
  std::string text2 = Detector::readFile(file2);

  if (text1.empty() && text2.empty()) {
    std::cerr << "Both files are empty or could not be read." << std::endl;
    return;
  }

 auto ast1 = Detector::buildAST(text1);
  auto ast2 = Detector::buildAST(text2);
  auto astResult = Detector::calculateASTSimilarity(ast1, ast2);
  delete ast1;
  delete ast2;

  std::string cleaned1 = Detector::cleanText(text1);
  std::string cleaned2 = Detector::cleanText(text2);
  auto words1 = Detector::tokenize(cleaned1);
  auto words2 = Detector::tokenize(cleaned2);
  auto freq1 = Detector::getWordFrequencies(cleaned1);
  auto freq2 = Detector::getWordFrequencies(cleaned2);
  auto bigram1 = Detector::getNgramFrequencies(words1, 2);
  auto bigram2 = Detector::getNgramFrequencies(words2, 2);
  auto trigram1 = Detector::getNgramFrequencies(words1, 3);
  auto trigram2 = Detector::getNgramFrequencies(words2, 3);

  auto unigramResult = Detector::calculateDetailedSimilarity(freq1, freq2);
  auto bigramResult = Detector::calculateDetailedSimilarity(bigram1, bigram2);
  auto trigramResult = Detector::calculateDetailedSimilarity(trigram1, trigram2);

  double astPct = astResult.score * 100.0;
  double unigramPct = unigramResult.score * 100.0;
  double bigramPct = bigramResult.score * 100.0;
  double trigramPct = trigramResult.score * 100.0;

  bool hasBigrams = !bigram1.empty() && !bigram2.empty();
  bool hasTrigrams = !trigram1.empty() && !trigram2.empty();

  double textScore;
  if (hasBigrams && hasTrigrams) {
    textScore = 0.35 * unigramPct + 0.40 * bigramPct + 0.25 * trigramPct;
  } else if (hasBigrams) {
    textScore = 0.45 * unigramPct + 0.55 * bigramPct;
  } else {
    textScore = unigramPct;
  }

  double combinedScore = 0.70 * astPct + 0.30 * textScore;
  const double PLAGIARISM_THRESHOLD = 60.0;
  bool isPlagiarized = combinedScore >= PLAGIARISM_THRESHOLD;

  std::string name1 = basename(file1);
  std::string name2 = basename(file2);

  std::string scoreColor = combinedScore >= 75.0   ? "#ef4444"
                           : combinedScore >= 40.0 ? "#eab308"
                                                   : "#10b981";

  std::string verdictClass = isPlagiarized ? "plagiarized" : "original";
  std::string verdictText =
      isPlagiarized ? "High Plagiarism Risk" : "Low Similarity";
  std::string verdictIcon =
      isPlagiarized
          ? "<svg width=\"18\" height=\"18\" fill=\"none\" "
            "stroke=\"currentColor\" stroke-width=\"2\" viewBox=\"0 0 24 "
            "24\"><path d=\"M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 "
            "0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z\"></path><line x1=\"12\" "
            "y1=\"9\" x2=\"12\" y2=\"13\"></line><line x1=\"12\" y1=\"17\" "
            "x2=\"12.01\" y2=\"17\"></line></svg>"
          : "<svg width=\"18\" height=\"18\" fill=\"none\" "
            "stroke=\"currentColor\" stroke-width=\"2\" viewBox=\"0 0 24 "
            "24\"><path d=\"M22 11.08V12a10 10 0 1 "
            "1-5.93-9.14\"></path><polyline "
            "points=\"22 4 12 14.01 9 11.01\"></polyline></svg>";

  // Fix circular gauge at 100%: calculate correct circumference and clamp score
  const double SVG_RADIUS = 70.0;
  const double SVG_CIRCUMFERENCE = 2.0 * M_PI * SVG_RADIUS;  // ~439.82
  const double clampedScore = std::min(combinedScore, 100.0);
  double dashOffset = SVG_CIRCUMFERENCE - (clampedScore / 100.0) * SVG_CIRCUMFERENCE;

  auto lines1 = extractLines(text1);
  auto lines2 = extractLines(text2);
  std::string marked1 = getHighlighted(text1, lines2);
  std::string marked2 = getHighlighted(text2, lines1);

  std::ofstream out("report.html");
  if (!out.is_open()) {
    std::cerr << "Error: Could not create report.html" << std::endl;
    return;
  }

  out << R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Plagiarism Analysis Report</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&display=swap" rel="stylesheet">
  <style>
    *{margin:0;padding:0;box-sizing:border-box;}
    body{font-family:'Inter',sans-serif;background:linear-gradient(135deg,#e8edf5 0%,#f5f7fa 50%,#eef1f8 100%);
      min-height:100vh;padding:2rem 1.5rem;color:#1a1a2e;display:flex;justify-content:center;align-items:flex-start;}
    
    .orb{position:fixed;border-radius:50%;filter:blur(110px);opacity:.06;pointer-events:none;z-index:0;animation:pulse 10s ease-in-out infinite;}
    .orb1{width:500px;height:500px;background:#4f46e5;top:-100px;left:-100px;}
    .orb2{width:400px;height:400px;background:#0891b2;bottom:-100px;right:-100px;animation-delay:3s;}
    @keyframes pulse{0%,100%{transform:scale(1);}50%{transform:scale(1.1);}}
    
    .container{position:relative;z-index:1;width:100%;max-width:1100px;padding-bottom:3rem;}
    
    .header-bar{display:flex;justify-content:space-between;align-items:center;margin-bottom:2rem;}
    .header-bar h1{font-size:1.6rem;font-weight:800;letter-spacing:-.02em;color:#1a1a2e;}
    
    .btn{display:inline-flex;align-items:center;gap:.5rem;padding:.6rem 1.2rem;background:#2563eb;border:1px solid #1e40af;border-radius:8px;color:#ffffff;text-decoration:none;font-size:.9rem;font-weight:600;transition:all .2s;}
    .btn:hover{background:#1d4ed8;transform:translateY(-1px);}

    .card{background:rgba(255,255,255,0.85);backdrop-filter:blur(12px);-webkit-backdrop-filter:blur(12px);border:1px solid rgba(255,255,255,0.6);border-radius:16px;padding:2rem;margin-bottom:1.5rem;box-shadow:0 8px 32px rgba(0,0,0,0.08);}
    
    .grid-2{display:grid;grid-template-columns:1fr 1.5fr;gap:2rem;}
    
    .files-compare{display:flex;align-items:center;justify-content:center;gap:1.5rem;margin-bottom:2.5rem;padding:1.5rem;background:#f9fafb;border-radius:12px;}
    .file-box{flex:1;text-align:center;width:100%;overflow:hidden;}
    .file-box .lbl{display:block;font-size:.7rem;color:#6b7280;text-transform:uppercase;letter-spacing:.05em;margin-bottom:.4rem;}
    .file-box .name{font-size:.95rem;font-family:monospace;color:#1a1a2e;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;background:#f3f4f6;padding:.5rem;border-radius:6px;border:1px solid #e5e7eb;}
    .vs-circ{width:36px;height:36px;flex-shrink:0;background:linear-gradient(135deg,#4f46e5,#0891b2);border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:.75rem;font-weight:800;color:#ffffff;box-shadow:0 4px 12px rgba(79,70,229,.3);}

    .score-ui{text-align:center;display:flex;flex-direction:column;align-items:center;justify-content:center;}
    .ring{position:relative;width:160px;height:160px;}
    .ring svg{width:160px;height:160px;transform:rotate(-90deg);}
    .ring circle{fill:none;stroke-width:10;}
    .ring .bg{stroke:#e5e7eb;}
    .ring .prog{stroke-dasharray:439.82;stroke-linecap:round;transition:stroke-dashoffset 1.5s ease;}
    .ring-inner{position:absolute;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center;}
    .ring-pct{font-size:1.8rem;font-weight:800;line-height:1;letter-spacing:-.03em;color:#1a1a2e;}
    .ring-lbl{font-size:.7rem;color:#6b7280;text-transform:uppercase;letter-spacing:.05em;margin-top:.2rem;}
    
    .verdict{margin-top:1.5rem;display:inline-flex;align-items:center;gap:.5rem;padding:.5rem 1rem;border-radius:50px;font-size:.85rem;font-weight:600;}
    .plagiarized{background:#fee2e2;border:1px solid #fecaca;color:#991b1b;}
    .original{background:#dcfce7;border:1px solid #bbf7d0;color:#166534;}

    .analytics h3{font-size:.9rem;color:#4a5568;margin-bottom:1.2rem;text-transform:uppercase;letter-spacing:.05em;}
    .metric-group{margin-bottom:2rem;}
    .metric{display:grid;grid-template-columns:130px 1fr 50px;align-items:center;gap:1rem;margin-bottom:1rem;}
    .m-label{font-size:.85rem;color:#6b7280;font-weight:500;}
    .bar-bg{background:#e2e8f0;border-radius:50px;height:8px;overflow:hidden;}
    .bar-fg{height:100%;border-radius:50px;}
    .m-val{font-size:.85rem;font-weight:700;color:#1a1a2e;text-align:right;}
    
    .insights{margin-top:1.5rem;}
    .insights h3{font-size:.85rem;color:#4a5568;margin-bottom:1rem;}
    .tags{display:flex;flex-wrap:wrap;gap:.5rem;}
    .tag{background:#eef2ff;border:1px solid #e0e7ff;color:#4338ca;padding:.3rem .8rem;border-radius:6px;font-family:monospace;font-size:.8rem;}
    
    .split-title{display:flex;align-items:center;gap:.5rem;font-size:1.1rem;font-weight:700;margin-bottom:1rem;color:#1a1a2e;}
    .code-split{display:grid;grid-template-columns:1fr 1fr;gap:2px;background:#f3f4f6;border-radius:12px;overflow:hidden;border:1px solid #e5e7eb;}
    .code-pane{background:#fafafa;height:500px;overflow:auto;font-family:monospace;font-size:.8rem;line-height:1.6;}
    .pane-header{background:#f3f4f6;padding:.75rem 1rem;font-size:.8rem;font-weight:600;color:#4a5568;border-bottom:1px solid #e5e7eb;position:sticky;top:0;}
    .code-line{display:flex;padding:0 .5rem;}
    .code-line:hover{background:#f9fafb;}
    .hl{background:rgba(255,220,0,0.2);}
    .hl .code-content{color:#b8860b;font-weight:500;}
    .code-line .ln{width:35px;flex-shrink:0;text-align:right;padding-right:10px;color:#9ca3af;user-select:none;border-right:1px solid #e5e7eb;margin-right:10px;}
    .code-content{white-space:pre;color:#1f2937;word-break:break-all;}

    @media(max-width:768px){
      .grid-2{grid-template-columns:1fr;}
      .code-split{grid-template-columns:1fr;height:auto;}
      .code-pane{height:300px;}
      .metric{grid-template-columns:100px 1fr 40px;}
    }
  </style>
</head>
<body>
  <div class="orb orb1"></div>
  <div class="orb orb2"></div>

  <div class="container">
    <div class="header-bar">
      <h1>Plagiarism Analysis Report</h1>
      <a href="/" class="btn">
        <svg width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M19 12H5M12 19l-7-7 7-7"/></svg>
        Analyze Another
      </a>
    </div>

    <div class="files-compare">
      <div class="file-box">
        <span class="lbl">Source File</span>
        <div class="name">)HTML";
  out << name1;
  out << R"HTML(</div>
      </div>
      <div class="vs-circ">VS</div>
      <div class="file-box">
        <span class="lbl">Target File</span>
        <div class="name">)HTML";
  out << name2;
  out << R"HTML(</div>
      </div>
    </div>

    <div class="card grid-2">
      <div class="score-ui">
        <div class="ring">
          <svg viewBox="0 0 160 160">
            <circle class="bg" cx="80" cy="80" r="70"/>
            <circle class="prog" cx="80" cy="80" r="70" stroke=")HTML";
  out << scoreColor;
  out << R"HTML(" stroke-dashoffset=")HTML";
  out << fmt(dashOffset);
  out << R"HTML("/>
          </svg>
          <div class="ring-inner">
            <div class="ring-pct" style="color:)HTML";
  out << scoreColor;
  out << R"HTML(">)HTML" << fmt(combinedScore) << R"HTML(%</div>
            <div class="ring-lbl">Match</div>
          </div>
        </div>
        <div class="verdict )HTML";
  out << verdictClass;
  out << R"HTML(">)HTML" << verdictIcon << R"HTML( <span>)HTML" << verdictText
      << R"HTML(</span></div>
      </div>

      <div class="analytics">
        <div class="metric-group">
          <h3>Text Similarity Analysis</h3>
          <div class="metric">
            <div class="m-label">Unigram (Words)</div>
            <div class="bar-bg"><div class="bar-fg" style="width:)HTML";
  out << fmt(unigramPct);
  out << R"HTML(%;background:linear-gradient(90deg,#4f46e5,#818cf8);"></div></div>
            <div class="m-val">)HTML"
      << fmt(unigramPct) << R"HTML(%</div>
          </div>
          <div class="metric">
            <div class="m-label">Bigram (Phrases)</div>
            <div class="bar-bg"><div class="bar-fg" style="width:)HTML";
  out << fmt(bigramPct);
  out << R"HTML(%;background:linear-gradient(90deg,#0891b2,#22d3ee);"></div></div>
            <div class="m-val">)HTML"
      << fmt(bigramPct) << R"HTML(%</div>
          </div>
          <div class="metric">
            <div class="m-label">Trigram (Logic)</div>
            <div class="bar-bg"><div class="bar-fg" style="width:)HTML";
  out << fmt(trigramPct);
  out << R"HTML(%;background:linear-gradient(90deg,#0ea5e9,#38bdf8);"></div></div>
            <div class="m-val">)HTML"
      << fmt(trigramPct) << R"HTML(%</div>
          </div>
        </div>

        <div class="metric-group">
          <h3>Code Structure Analysis</h3>
          <div class="metric">
            <div class="m-label">AST Structure</div>
            <div class="bar-bg"><div class="bar-fg" style="width:)HTML";
  out << fmt(astPct);
  out << R"HTML(%;background:linear-gradient(90deg,#10b981,#34d399);"></div></div>
            <div class="m-val">)HTML"
      << fmt(astPct) << R"HTML(%</div>
          </div>
        </div>
      </div>
    </div>

    <div class="card insights">
      <h3>Plagiarism Match Insights (Overlapping Terms)</h3>
      <div class="tags">)HTML";

  int count = 0;
  for (const auto &term : unigramResult.commonTerms) {
    if (count++ >= 30) {
      out << R"HTML(<span class="tag" style="background:transparent;border:0;color:#64748b;">&#8230;</span>)HTML";
      break;
    }
    out << R"HTML(<span class="tag">)HTML" << term << R"HTML(</span>)HTML";
  }
  if (unigramResult.commonTerms.empty()) {
    out << R"HTML(<span style="color:#64748b;font-size:0.85rem;font-style:italic;">No matching identifiers found.</span>)HTML";
  }

  out << R"HTML(
      </div>
    </div>
    
    <div class="split-title">
      <svg width="20" height="20" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><line x1="16" y1="13" x2="8" y2="13"></line><line x1="16" y1="17" x2="8" y2="17"></line><polyline points="10 9 9 9 8 9"></polyline></svg>
      Code Comparison Viewer
    </div>
    <div class="code-split">
      <div class="code-pane">
        <div class="pane-header">)HTML"
      << name1 << R"HTML(</div>
        )HTML";
  out << marked1;
  out << R"HTML(
      </div>
      <div class="code-pane">
        <div class="pane-header">)HTML"
      << name2 << R"HTML(</div>
        )HTML";
  out << marked2;
  out << R"HTML(
      </div>
    </div>
    
    <div style="text-align:center;margin-top:2.5rem;">
      <a href="/" class="btn" style="padding:1rem 2rem;font-size:1rem;background:linear-gradient(135deg,#4f46e5,#0891b2);border:none;box-shadow:0 10px 20px rgba(79,70,229,.3);">Analyze Another Document Pair</a>
    </div>

  </div>
</body>
</html>)HTML";

  out.close();
  std::cout << "Report generated: report.html" << std::endl;
}

// Helper: Get all supported files from directory
std::vector<std::string> getFilesFromDirectory(const std::string &dirPath) {
  std::vector<std::string> files;
  if (!fs::is_directory(dirPath)) {
    std::cerr << "Error: " << dirPath << " is not a valid directory." << std::endl;
    return files;
  }

  for (const auto &entry : fs::directory_iterator(dirPath)) {
    if (entry.is_regular_file()) {
      std::string ext = entry.path().extension().string();
      if (ext == ".cpp" || ext == ".h" || ext == ".txt" || ext == ".c" ||
          ext == ".cc" || ext == ".cxx") {
        files.push_back(entry.path().string());
      }
    }
  }
  
  if (files.empty()) {
    std::cerr << "No supported files (.cpp, .h, .txt, .c) found in " << dirPath
             << std::endl;
  }
  
  return files;
}

void generateBatchReport(const Detector::BatchSimilarityMatrix &batchResult);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage:" << std::endl;
    std::cerr << "  Two-file mode:  " << argv[0] << " <file1> <file2>"
             << std::endl;
    std::cerr << "  Batch mode:     " << argv[0] << " --batch <directory>"
             << std::endl;
    return 1;
  }

  // Check for batch mode
  if (argc >= 3 && std::string(argv[1]) == "--batch") {
    std::string dirPath = argv[2];
    std::cout << "Running batch analysis on directory: " << dirPath << std::endl;

    auto files = getFilesFromDirectory(dirPath);
    if (files.empty()) {
      return 1;
    }

    std::cout << "Found " << files.size() << " files. Starting analysis..."
             << std::endl;

    auto batchResult = Detector::conductBatchAnalysis(files);
    generateBatchReport(batchResult);
    return 0;
  }

  // Standard 2-file mode
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <file1> <file2>" << std::endl;
    return 1;
  }

  generateTwoFileReport(argv[1], argv[2]);
  return 0;
}

void generateBatchReport(const Detector::BatchSimilarityMatrix &batchResult) {
  std::ofstream out("batch_report.html");
  if (!out.is_open()) {
    std::cerr << "Error: Could not create batch_report.html" << std::endl;
    return;
  }

  out << R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Batch Plagiarism Analysis Report</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&display=swap" rel="stylesheet">
  <style>
    *{margin:0;padding:0;box-sizing:border-box;}
    body{font-family:'Inter',sans-serif;background:linear-gradient(135deg,#e8edf5 0%,#f5f7fa 50%,#eef1f8 100%);
      min-height:100vh;padding:2rem 1.5rem;color:#1a1a2e;}
    
    .orb{position:fixed;border-radius:50%;filter:blur(110px);opacity:.06;pointer-events:none;z-index:0;animation:pulse 10s ease-in-out infinite;}
    .orb1{width:600px;height:600px;background:#4f46e5;top:-100px;left:-100px;}
    .orb2{width:500px;height:500px;background:#0891b2;bottom:-100px;right:-100px;animation-delay:3s;}
    @keyframes pulse{0%,100%{transform:scale(1);}50%{transform:scale(1.1);}}
    
    .container{position:relative;z-index:1;width:100%;max-width:1200px;margin:0 auto;padding-bottom:3rem;}
    
    .header{margin-bottom:2.5rem;}
    .header h1{font-size:2rem;font-weight:800;color:#1a1a2e;margin-bottom:.5rem;}
    .header p{color:#4a5568;font-size:1rem;}
    
    .card{background:rgba(255,255,255,0.85);backdrop-filter:blur(12px);border:1px solid rgba(255,255,255,0.6);border-radius:16px;padding:2rem;margin-bottom:1.5rem;box-shadow:0 8px 32px rgba(0,0,0,0.08);}
    
    .summary{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:1.5rem;margin-bottom:2rem;}
    .stat-box{background:#f9fafb;border:1px solid #e5e7eb;border-radius:12px;padding:1.5rem;text-align:center;}
    .stat-val{font-size:2.5rem;font-weight:800;color:#2563eb;line-height:1;}
    .stat-lbl{font-size:.85rem;color:#6b7280;margin-top:.5rem;text-transform:uppercase;letter-spacing:.05em;}
    
    .cluster-section{margin-bottom:2rem;}
    .cluster-title{display:flex;align-items:center;gap:.75rem;font-size:1.2rem;font-weight:700;margin-bottom:1.25rem;color:#1a1a2e;}
    .cluster-badge{background:#ef4444;color:#fff;padding:.3rem .8rem;border-radius:6px;font-size:.8rem;font-weight:600;}
    
    .member-list{background:#f9fafb;border-radius:12px;padding:1.5rem;margin-bottom:1.5rem;border:1px solid #e5e7eb;}
    .member-item{display:flex;align-items:center;justify-content:space-between;padding:.75rem;border-bottom:1px solid #e5e7eb;}
    .member-item:last-child{border-bottom:none;}
    .member-name{display:flex;align-items:center;gap:.5rem;font-family:monospace;color:#1a1a2e;}
    .badge-original{background:#dcfce7;border:1px solid #bbf7d0;color:#166534;padding:.25rem .6rem;border-radius:4px;font-size:.75rem;font-weight:600;}
    
    .pairs-table{width:100%;border-collapse:collapse;font-size:.9rem;}
    .pairs-table th{background:#e8edf5;padding:.75rem;text-align:left;color:#1a1a2e;font-weight:600;border-bottom:1px solid #d8e4f0;}
    .pairs-table td{padding:.75rem;border-bottom:1px solid #e5e7eb;color:#1a1a2e;}
    .pairs-table tbody tr:hover{background:#f9fafb;}
    .score-cell{font-weight:600;color:#dc2626;}
    
    .matrix-container{overflow-x:auto;background:#f9fafb;border-radius:12px;padding:1.5rem;border:1px solid #e5e7eb;}
    .matrix-table{width:100%;border-collapse:collapse;font-size:.8rem;font-family:monospace;}
    .matrix-table th,.matrix-table td{padding:.5rem;text-align:center;border:1px solid #e5e7eb;}
    .matrix-table th{background:#e8edf5;color:#1a1a2e;font-weight:600;}
    .matrix-table td{color:#1a1a2e;}
    .matrix-high{background:#fee2e2;color:#991b1b;font-weight:600;}
    .matrix-med{background:#fef3c7;color:#b45309;}
    .matrix-low{background:#dcfce7;color:#166534;}
    
    @media(max-width:768px){
      .summary{grid-template-columns:1fr;}
      .pairs-table{font-size:.8rem;}
      .matrix-table{font-size:.7rem;}
    }
  </style>
</head>
<body>
  <div class="orb orb1"></div>
  <div class="orb orb2"></div>

  <div class="container">
    <div class="header" style="display:flex;justify-content:space-between;align-items:flex-start;gap:1rem;">
      <div>
        <h1>Batch Plagiarism Analysis Report</h1>
        <p>Analyzed )HTML";
  out << batchResult.totalFiles;
  out << R"HTML( files • )HTML";
  out << batchResult.totalSuspiciousPairs;
  out << R"HTML( suspicious pairs • )HTML";
  out << batchResult.clusters.size();
  out << R"HTML( clusters detected</p>
      </div>
      <a href="/" class="btn" style="white-space:nowrap;">
        <svg width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M19 12H5M12 19l-7-7 7-7"></path></svg>
        Analyze Another
      </a>
    </div>

    <div class="card">
      <div class="summary">
        <div class="stat-box">
          <div class="stat-val">)HTML";
  out << batchResult.totalFiles;
  out << R"HTML(</div>
          <div class="stat-lbl">Total Files</div>
        </div>
        <div class="stat-box">
          <div class="stat-val">)HTML";
  out << batchResult.totalSuspiciousPairs;
  out << R"HTML(</div>
          <div class="stat-lbl">Suspicious Pairs</div>
        </div>
        <div class="stat-box">
          <div class="stat-val">)HTML";
  out << batchResult.clusters.size();
  out << R"HTML(</div>
          <div class="stat-lbl">Clusters Found</div>
        </div>
        <div class="stat-box">
          <div class="stat-val">)HTML";
  if (batchResult.totalFiles > 1) {
    out << fmt(100.0 * batchResult.totalSuspiciousPairs / (batchResult.totalFiles * (batchResult.totalFiles - 1) / 2));
  } else {
    out << "0";
  }
  out << R"HTML(%</div>
          <div class="stat-lbl">Suspicion Rate</div>
        </div>
      </div>
    </div>

    )HTML";

  int clusterNum = 1;
  for (const auto &cluster : batchResult.clusters) {
    out << R"HTML(
    <div class="card cluster-section">
      <div class="cluster-title">
        <span class="cluster-badge">Cluster )HTML";
    out << clusterNum++;
    out << R"HTML(</span>
        <span>)HTML" << cluster.members.size() << R"HTML( files</span>
      </div>
      
      <div class="member-list">
        <div style="font-size:.9rem;font-weight:600;color:#4a5568;margin-bottom:1rem;text-transform:uppercase;letter-spacing:.05em;">Member Files</div>)HTML";

    for (const auto &member : cluster.members) {
      out << R"HTML(
        <div class="member-item">
          <div class="member-name">
            <span>)HTML" << escapeHTML(member.filename) << R"HTML(</span>
            )HTML";
      if (member.isProbableOriginal) {
        out << R"HTML(<span class="badge-original">Probable Original</span>)HTML";
      }
      out << R"HTML(
          </div>
          <div style="color:#6b7280;font-size:.85rem;">)HTML";
      out << member.degree;
      out << R"HTML( match)HTML" << (member.degree != 1 ? "es" : "")
          << R"HTML(</div>
        </div>)HTML";
    }

    out << R"HTML(
      </div>
      
      <div style="margin-top:1.5rem;">
        <div style="font-size:.9rem;font-weight:600;color:#4a5568;margin-bottom:1rem;text-transform:uppercase;letter-spacing:.05em;">Pairwise Similarity</div>
        <table class="pairs-table">
          <thead>
            <tr>
              <th>File 1</th>
              <th>File 2</th>
              <th>Similarity</th>
              <th>Status</th>
            </tr>
          </thead>
          <tbody>)HTML";

    for (const auto &pair : cluster.internalPairs) {
      double score = 100.0 * pair.similarity;
      std::string status = score >= 60.0 ? "Plagiarism" : "Suspicious";
      std::string statusBg = score >= 60.0 ? "#fee2e2" : "#fef3c7";
      std::string statusBorder = score >= 60.0 ? "#fecaca" : "#fcd34d";
      std::string statusText = score >= 60.0 ? "#991b1b" : "#b45309";
      
      out << R"HTML(
            <tr>
              <td>)HTML" << escapeHTML(batchResult.filenames[pair.fileIdx1])
          << R"HTML(</td>
              <td>)HTML" << escapeHTML(batchResult.filenames[pair.fileIdx2])
          << R"HTML(</td>
              <td class="score-cell">)HTML" << fmt(score) << R"HTML(%</td>
              <td><span style="background:)HTML" << statusBg << R"HTML(;border:1px solid )HTML"
          << statusBorder << R"HTML(;color:)HTML" << statusText << R"HTML(;padding:.25rem .6rem;border-radius:4px;font-size:.75rem;font-weight:600;">)HTML"
          << status << R"HTML(</span></td>
            </tr>)HTML";
    }

    out << R"HTML(
          </tbody>
        </table>
      </div>
    </div>)HTML";
  }

  out << R"HTML(

    <!-- Full Similarity Matrix -->
    <div class="card">
      <div style="font-size:1.1rem;font-weight:700;margin-bottom:1.5rem;color:#1a1a2e;">Complete Similarity Matrix (All Files)</div>
      <div class="matrix-container">
        <table class="matrix-table">
          <thead>
            <tr>
              <th style="text-align:right;">File</th>)HTML";

  for (size_t i = 0; i < batchResult.filenames.size(); ++i) {
    out << R"HTML(<th>)HTML" << (i + 1) << R"HTML(</th>)HTML";
  }
  out << R"HTML(
            </tr>
          </thead>
          <tbody>)HTML";

  for (size_t i = 0; i < batchResult.filenames.size(); ++i) {
    out << R"HTML(
            <tr>
              <td style="text-align:right;font-weight:600;">)HTML" << (i + 1)
        << R"HTML(</td>)HTML";
    for (size_t j = 0; j < batchResult.filenames.size(); ++j) {
      double score = 100.0 * batchResult.matrix[i][j];
      std::string scoreClass = "matrix-low";
      if (score >= 60.0) scoreClass = "matrix-high";
      else if (score >= 30.0) scoreClass = "matrix-med";
      
      out << R"HTML(<td class=")HTML" << scoreClass << R"HTML(">)HTML"
          << fmt(score) << R"HTML(%</td>)HTML";
    }
    out << R"HTML(
            </tr>)HTML";
  }

  out << R"HTML(
          </tbody>
        </table>
      </div>
      
      <div style="margin-top:1rem;padding:1rem;background:rgba(15,23,42,.4);border-radius:8px;font-size:.85rem;color:#94a3b8;">
        <div style="display:flex;gap:1rem;flex-wrap:wrap;">
          <div style="display:flex;align-items:center;gap:.5rem;">
            <span style="width:20px;height:20px;background:#fee2e2;border:1px solid #fecaca;border-radius:4px;"></span>
            <span style="color:#1a1a2e;">High Risk (≥60%)</span>
          </div>
          <div style="display:flex;align-items:center;gap:.5rem;">
            <span style="width:20px;height:20px;background:#fef3c7;border:1px solid #fcd34d;border-radius:4px;"></span>
            <span style="color:#1a1a2e;">Medium Risk (30-60%)</span>
          </div>
          <div style="display:flex;align-items:center;gap:.5rem;">
            <span style="width:20px;height:20px;background:#dcfce7;border:1px solid #bbf7d0;border-radius:4px;"></span>
            <span style="color:#1a1a2e;">Low Risk (<30%)</span>
          </div>
        </div>
      </div>
    </div>

    <div style="text-align:center;margin-top:3rem;padding-top:2rem;border-top:1px solid #e5e7eb;">
      <p style="color:#4a5568;font-size:.9rem;">Generated by C++ Plagiarism Detector | Fingerprint threshold: 30% | Full analysis threshold: 60%</p>
    </div>

  </div>
</body>
</html>)HTML";

  out.close();
  std::cout << "Report generated: batch_report.html" << std::endl;
}
