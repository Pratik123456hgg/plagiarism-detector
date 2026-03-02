#include "detector.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

// Returns just the filename from a full path
static std::string basename(const std::string &path) {
  size_t pos = path.find_last_of("/\\");
  return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

// Formats a double as a string with 1 decimal place
static std::string fmt(double v) {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(1) << v;
  return ss.str();
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <file1> <file2>" << std::endl;
    return 1;
  }

  std::string file1 = argv[1];
  std::string file2 = argv[2];

  std::string text1 = Detector::readFile(file1);
  std::string text2 = Detector::readFile(file2);

  if (text1.empty() && text2.empty()) {
    std::cerr << "Both files are empty or could not be read." << std::endl;
    return 1;
  }

  // --- 1. AST Structural Matching ---
  auto ast1 = Detector::buildAST(text1);
  auto ast2 = Detector::buildAST(text2);
  auto astResult = Detector::calculateASTSimilarity(ast1, ast2);
  delete ast1;
  delete ast2;

  // --- 2. N-Gram Text Matching ---
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
  auto trigramResult =
      Detector::calculateDetailedSimilarity(trigram1, trigram2);

  double astPct = astResult.score * 100.0;
  double unigramPct = unigramResult.score * 100.0;
  double bigramPct = bigramResult.score * 100.0;
  double trigramPct = trigramResult.score * 100.0;

  // --- Calculate Combined Score ---
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

  // 70% Structure, 30% Text (Words & Phrases)
  double combinedScore = 0.70 * astPct + 0.30 * textScore;

  const double PLAGIARISM_THRESHOLD = 60.0;
  bool isPlagiarized = combinedScore >= PLAGIARISM_THRESHOLD;

  std::string name1 = basename(file1);
  std::string name2 = basename(file2);

  // Color based on severity
  std::string scoreColor = combinedScore >= 75.0   ? "#ef4444"
                           : combinedScore >= 50.0 ? "#f59e0b"
                                                   : "#22c55e";

  std::string verdictClass = isPlagiarized ? "plagiarized" : "original";
  std::string verdictText =
      isPlagiarized ? "Plagiarism Detected" : "No Plagiarism Found";
  std::string verdictIcon = isPlagiarized ? "&#9888;" : "&#10003;";

  // SVG ring: circumference = 2 * pi * 65 ≈ 408.4
  double dashOffset = 408.4 * (1.0 - combinedScore / 100.0);

  std::ofstream out("report.html");
  if (!out.is_open()) {
    std::cerr << "Error: Could not create report.html" << std::endl;
    return 1;
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
    body{font-family:'Inter',sans-serif;background:linear-gradient(135deg,#0f0c29 0%,#302b63 50%,#24243e 100%);
      min-height:100vh;padding:2rem 1rem;color:#fff;display:flex;justify-content:center;align-items:flex-start;}
    .orb{position:fixed;border-radius:50%;filter:blur(90px);opacity:.14;pointer-events:none;z-index:0;
      animation:pulse 9s ease-in-out infinite;}
    .orb1{width:500px;height:500px;background:#7c3aed;top:-120px;left:-120px;}
    .orb2{width:400px;height:400px;background:#2563eb;bottom:-80px;right:-80px;animation-delay:4.5s;}
    @keyframes pulse{0%,100%{transform:scale(1);}50%{transform:scale(1.18);}}
    .container{position:relative;z-index:1;width:100%;max-width:700px;padding-bottom:3rem;}
    .back-btn{display:inline-flex;align-items:center;gap:.5rem;padding:.6rem 1.4rem;
      background:rgba(255,255,255,.07);border:1px solid rgba(255,255,255,.14);border-radius:50px;
      color:rgba(255,255,255,.8);text-decoration:none;font-size:.88rem;font-weight:500;
      margin-bottom:1.5rem;backdrop-filter:blur(10px);transition:all .2s;}
    .back-btn:hover{background:rgba(255,255,255,.14);transform:translateX(-3px);}
    .card{background:rgba(255,255,255,.05);backdrop-filter:blur(20px);
      border:1px solid rgba(255,255,255,.09);border-radius:24px;padding:2.5rem;margin-bottom:1rem;}
    .header{text-align:center;margin-bottom:2rem;}
    .header h1{font-size:1.7rem;font-weight:700;
      background:linear-gradient(135deg,#a78bfa,#60a5fa);
      -webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;margin-bottom:.4rem;}
    .header p{color:rgba(255,255,255,.4);font-size:.85rem;}
    .files-row{display:flex;align-items:center;gap:.75rem;margin-bottom:2rem;flex-wrap:wrap;}
    .file-chip{flex:1;min-width:0;background:rgba(255,255,255,.07);border:1px solid rgba(255,255,255,.1);
      border-radius:12px;padding:.8rem 1rem;font-size:.83rem;color:rgba(255,255,255,.75);
      overflow:hidden;text-overflow:ellipsis;white-space:nowrap;}
    .file-chip .lbl{display:block;font-size:.68rem;color:rgba(255,255,255,.35);
      text-transform:uppercase;letter-spacing:.06em;margin-bottom:.2rem;}
    .vs{flex-shrink:0;width:34px;height:34px;background:linear-gradient(135deg,#7c3aed,#2563eb);
      border-radius:50%;display:flex;align-items:center;justify-content:center;
      font-size:.65rem;font-weight:700;}
    .score-wrap{text-align:center;margin-bottom:2rem;}
    .ring{position:relative;display:inline-block;width:150px;height:150px;margin-bottom:1rem;}
    .ring svg{width:150px;height:150px;transform:rotate(-90deg);}
    .ring circle{fill:none;stroke-width:10;}
    .ring .bg{stroke:rgba(255,255,255,.07);}
    .ring .prog{stroke-dasharray:408.4;stroke-linecap:round;transition:stroke-dashoffset 1.2s ease;}
    .ring-inner{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);text-align:center;}
    .ring-pct{font-size:2rem;font-weight:800;line-height:1;}
    .ring-lbl{font-size:.62rem;color:rgba(255,255,255,.4);text-transform:uppercase;letter-spacing:.08em;}
    .verdict{display:inline-flex;align-items:center;gap:.5rem;padding:.65rem 1.6rem;
      border-radius:50px;font-size:.95rem;font-weight:600;}
    .plagiarized{background:rgba(239,68,68,.12);border:1px solid rgba(239,68,68,.35);color:#fca5a5;}
    .original{background:rgba(34,197,94,.12);border:1px solid rgba(34,197,94,.35);color:#86efac;}
    .section-title{font-size:.7rem;font-weight:600;text-transform:uppercase;letter-spacing:.09em;
      color:rgba(255,255,255,.35);margin-bottom:1.2rem;display:flex;align-items:center;justify-content:space-between;}
    .metric{display:grid;grid-template-columns:140px 1fr 52px;align-items:center;gap:1rem;margin-bottom:.9rem;}
    .metric-name{font-size:.83rem;color:rgba(255,255,255,.7);font-weight:500;}
    .metric-name small{display:block;font-size:.7rem;color:rgba(255,255,255,.3);font-weight:400;}
    .bar-track{background:rgba(255,255,255,.07);border-radius:50px;height:7px;overflow:hidden;}
    .bar-fill{height:100%;border-radius:50px;}
    .metric-val{font-size:.83rem;font-weight:600;text-align:right;color:rgba(255,255,255,.8);}
    .terms-wrap{display:flex;flex-wrap:wrap;gap:.45rem;}
    .term{padding:.28rem .7rem;background:rgba(124,58,237,.18);border:1px solid rgba(124,58,237,.32);
      border-radius:50px;font-size:.78rem;color:#c4b5fd;font-family:monospace;}
    .none{color:rgba(255,255,255,.3);font-style:italic;font-size:.88rem;}
    .footer-btn{display:block;text-align:center;padding:1rem;
      background:linear-gradient(135deg,#7c3aed,#2563eb);border-radius:14px;
      color:#fff;text-decoration:none;font-weight:600;font-size:1rem;
      transition:opacity .2s;margin-top:1rem;}
    .footer-btn:hover{opacity:.82;}
    .sub-split{margin-top:2rem;padding-top:1.5rem;border-top:1px solid rgba(255,255,255,.08);}
    @media(max-width:480px){.metric{grid-template-columns:100px 1fr 44px;}.card{padding:1.5rem;}}
  </style>
</head>
<body>
  <div class="orb orb1"></div>
  <div class="orb orb2"></div>
  <div class="container">
    <a href="/" class="back-btn">&#8592; Analyze Again</a>
    <div class="card">
      <div class="header">
        <h1>Plagiarism Analysis Report</h1>
        <p>Text N-Grams &middot; AST Structural Match</p>
      </div>

      <div class="files-row">
        <div class="file-chip"><span class="lbl">Source File</span>)HTML";
  out << name1;
  out << R"HTML(</div>
        <div class="vs">VS</div>
        <div class="file-chip"><span class="lbl">Target File</span>)HTML";
  out << name2;
  out << R"HTML(</div>
      </div>

      <div class="score-wrap">
        <div class="ring">
          <svg viewBox="0 0 150 150">
            <circle class="bg" cx="75" cy="75" r="65"/>
            <circle class="prog" cx="75" cy="75" r="65" stroke=")HTML";
  out << scoreColor;
  out << R"HTML(" stroke-dashoffset=")HTML";
  out << fmt(dashOffset);
  out << R"HTML("/>
          </svg>
          <div class="ring-inner">
            <div class="ring-pct" style="color:)HTML";
  out << scoreColor;
  out << R"HTML(">)HTML" << fmt(combinedScore) << R"HTML(%</div>
            <div class="ring-lbl">combined</div>
          </div>
        </div>
        <div><span class="verdict )HTML";
  out << verdictClass;
  out << R"HTML(">)HTML" << verdictIcon << " " << verdictText
      << R"HTML(</span></div>
      </div>

      <div class="section-title">&#128202; Text &amp; Phrase Match <span style="font-size:0.6rem;opacity:0.5;font-weight:400;">30% Weight</span></div>
      
      <div class="metric">
        <div class="metric-name">Word-Level<small>Unigrams</small></div>
        <div class="bar-track"><div class="bar-fill" style="width:)HTML";
  out << fmt(unigramPct);
  out << R"HTML(%;background:linear-gradient(90deg,#7c3aed,#a78bfa);"></div></div>
        <div class="metric-val">)HTML"
      << fmt(unigramPct) << R"HTML(%</div>
      </div>

      <div class="metric">
        <div class="metric-name">Phrase-Level<small>Bigrams (2-word)</small></div>
        <div class="bar-track"><div class="bar-fill" style="width:)HTML";
  out << fmt(bigramPct);
  out << R"HTML(%;background:linear-gradient(90deg,#2563eb,#60a5fa);"></div></div>
        <div class="metric-val">)HTML"
      << fmt(bigramPct) << R"HTML(%</div>
      </div>

      <div class="metric">
        <div class="metric-name">Phrase-Level<small>Trigrams (3-word)</small></div>
        <div class="bar-track"><div class="bar-fill" style="width:)HTML";
  out << fmt(trigramPct);
  out << R"HTML(%;background:linear-gradient(90deg,#0891b2,#38bdf8);"></div></div>
        <div class="metric-val">)HTML"
      << fmt(trigramPct) << R"HTML(%</div>
      </div>

      <div class="section-title" style="margin-top:1.5rem;">&#9961; Code Structure Match <span style="font-size:0.6rem;opacity:0.5;font-weight:400;">70% Weight</span></div>
      
      <div class="metric">
        <div class="metric-name">AST Sequence<small>Logical flow</small></div>
        <div class="bar-track"><div class="bar-fill" style="width:)HTML";
  out << fmt(astPct);
  out << R"HTML(%;background:linear-gradient(90deg,#10b981,#34d399);"></div></div>
        <div class="metric-val">)HTML"
      << fmt(astPct) << R"HTML(%</div>
      </div>
    </div>

    <!-- Overlapping Terms Section -->
    <div class="card">
      <div class="section-title">&#128269; Overlapping Plagiarism Matches</div>
      
      <div style="font-size:0.75rem; color:rgba(255,255,255,.5); margin-bottom:0.6rem;">COMMON IDENTIFIERS &amp; TEXT TERMS</div>
      <div class="terms-wrap">)HTML";

  int count = 0;
  for (const auto &term : unigramResult.commonTerms) {
    if (count++ >= 40) {
      out << R"HTML(<span class="term" style="background:transparent;border:0;color:#999;">&#8230;</span>)HTML";
      break;
    }
    out << R"HTML(<span class="term">)HTML" << term << R"HTML(</span>)HTML";
  }
  if (unigramResult.commonTerms.empty()) {
    out << R"HTML(<span class="none">No overlapping text found.</span>)HTML";
  }

  out << R"HTML(
      </div>

      <div class="sub-split">
        <div style="font-size:0.75rem; color:rgba(255,255,255,.5); margin-bottom:0.6rem;">STRUCTURAL AST LOGIC FLOW</div>
        <div class="terms-wrap">)HTML";

  count = 0;
  for (const auto &term : astResult.commonTerms) {
    if (count++ >= 15) {
      out << R"HTML(<span class="term" style="background:transparent;border:0;color:#999;">&#8230;</span>)HTML";
      break;
    }
    // Clean string for UI
    std::string safeTerm = term;
    auto pos = safeTerm.find("├── ");
    if (pos != std::string::npos)
      safeTerm = safeTerm.substr(pos + 6);
    out << R"HTML(<span class="term" style="background:rgba(16,185,129,.15);border-color:rgba(16,185,129,.35);color:#6ee7b7;">)HTML"
        << safeTerm << R"HTML(</span>)HTML";
  }
  if (astResult.commonTerms.empty()) {
    out << R"HTML(<span class="none">No structural overlap found.</span>)HTML";
  }

  out << R"HTML(
        </div>
      </div>
    </div>

    <a href="/" class="footer-btn">&#43; Analyze Another Document Pair</a>
  </div>
</body>
</html>)HTML";

  out.close();
  std::cout << "Report generated: report.html" << std::endl;
  return 0;
}
