# 📄 Plagiarism Detector

A sophisticated text and code plagiarism detection tool built with **C++** (high-performance detection engine) and **Node.js Express** (modern web interface). Detect plagiarism between documents with advanced NLP techniques including n-gram analysis, AST structural comparison, and cosine similarity.

**Features:**
- ✅ **Two Detection Modes**: Compare two files or batch analyze multiple files
- ✅ **Dual Algorithm Support**: N-gram frequency analysis + Abstract Syntax Tree (AST) comparison
- ✅ **Batch Processing**: Analyze multiple files simultaneously with cross-pair detection
- ✅ **Beautiful Web Interface**: Modern, responsive UI with real-time feedback (Express.js)
- ✅ **Detailed Reports**: HTML reports with similarity scores, common terms, and structural analysis
- ✅ **PDF Export**: Download analysis reports as styled PDFs using Puppeteer
- ✅ **Support Multiple Formats**: `.cpp`, `.h`, `.c`, `.cc`, `.cxx`, `.txt`

---

## 🧠 Detection Algorithms

### 1. **N-Gram Frequency Analysis**
- Tokenizes text into cleaned words
- Builds unigram and n-gram frequency maps
- Computes **cosine similarity** between frequency vectors
- Extracts and displays common terms
- Flags plagiarism if similarity **≥ 60%**

### 2. **Abstract Syntax Tree (AST) Analysis** (for code)
- Tokenizes source code into syntactic elements
- Builds ASTs representing program structure
- Compares tree structures using longest common subsequence (LCS)
- Detects structural plagiarism (same logic with renamed variables)
- Calculates structural similarity score

### 3. **Batch Mode Analysis**
- Processes all file pairs in a dataset simultaneously
- Identifies fingerprints and cross-correlations
- Clusters suspicious files by similarity patterns
- Generates comprehensive batch report with recommendations

---

## 🗂️ Project Structure

```
plagiarism-detector/
├── detector.cpp          # Core detection algorithms implementation
├── detector.h            # Header file: class definitions & declarations
├── main.cpp              # CLI entry point for single/batch analysis (compiled to 'detector' binary)
├── server.js             # Express.js web server with dual-mode routes + PDF export
├── package.json          # Node.js dependencies (express, multer, puppeteer)
├── Makefile              # Build configuration (C++17, optimized compilation)
├── templates/
│   └── index.html        # Beautiful responsive UI with PDF download buttons
├── uploads/              # Temporary storage for uploaded files
├── report.html           # Generated single-pair analysis report
├── batch_report.html     # Generated multi-file analysis report
├── test1.cpp → test8.cpp # Sample C++ test files
├── test1.txt → test4.txt # Sample text test files
├── README.md             # This file
├── PDF_EXPORT_IMPLEMENTATION.md # Detailed PDF feature documentation
└── QUICK_START.md        # Quick start guide
```

---

## ⚙️ Requirements

| Dependency | Version | Purpose |
|---|---|---|
| `g++` | C++17 or later | Compile C++ detector with optimizations |
| `make` | GNU Make | Build automation |
| `Node.js` | 18.0+ | JavaScript runtime |
| `npm` | 9.0+ | Node package manager |
| `express` | 4.18+ | Web framework for routing & server setup |
| `multer` | 1.4+ | Middleware for handling multipart file uploads |
| `puppeteer` | 21.0+ | Headless Chrome for HTML-to-PDF conversion |

### Installation

**Ubuntu/Debian:**
```bash
sudo apt update && sudo apt install g++ make nodejs npm
```

**macOS (with Homebrew):**
```bash
brew install gcc make node
```

**Windows:**
- Install [Node.js](https://nodejs.org/) (includes npm)
- Install [MinGW](https://www.mingw-w64.org/) or use WSL for g++ and make

---

## 🚀 Quick Start

### Step 1 — Build the Detector

```bash
cd plagiarism-detector
make
```

Compiles C++17 code with optimizations into `detector` executable.

To rebuild from scratch:
```bash
make clean && make
```

### Step 2 — Install Node Dependencies

```bash
npm install
```

Installs Express, Multer, Puppeteer, and other dependencies from `package.json`.

> ℹ️ **Note:** Puppeteer downloads Chromium on first install (~200MB). This is normal and required for PDF generation.

### Step 3 — Start the Server

```bash
npm start
```

Output:
```
Server running! Open http://localhost:5000 in your browser.
```

**For development with auto-reload:**
```bash
npm run dev
```

### Step 4 — Use the Web Interface

Once the server is running, visit **http://localhost:5000** to access the dual-mode interface:
- **Two-File Mode**: Compare two files for plagiarism
- **Batch Mode**: Upload 5+ files for comprehensive cross-file analysis

---

## 📖 Usage Guide

### Web Interface (Recommended)

**Two-File Mode:**
1. Click "Two File Mode" toggle
2. Upload File 1 and File 2 (drag & drop or click to browse)
3. Click **Analyze** button
4. View detailed similarity report with common terms
5. Click **📄 Download Report** to export as PDF (new feature!)

**Batch Mode:**
1. Click "Batch Mode" toggle
2. Upload 5+ files (supports `.cpp`, `.h`, `.c`, `.txt`, `.cc`, `.cxx`)
3. Set optional threshold slider (default: 60%)
4. Click **Analyze**
5. View comprehensive report with:
   - All file pair comparisons
   - Similarity matrix
   - Suspicious clusters
   - Recommendations for further review
6. Click **📄 Download Report** to export full batch report as PDF (new feature!)

### Command-Line Interface

For direct binary execution without web server:

```bash
./detector <file1> <file2>
```

**Examples:**
```bash
./detector test1.cpp test2.cpp
./detector sample.txt plagiarism.txt
```

Output:
- Generates `report.html` in current directory
- Open in browser to view results

### Batch CLI Mode

```bash
./detector --batch /path/to/directory
```

Analyzes all supported files in directory:
- Creates `batch_report.html`
- Less detailed than web batch mode

---

## 📊 How Results Are Interpreted

### Similarity Scoring

| Score Range | Interpretation | Verdict |
|---|---|---|
| 0—30% | Minimal overlap, different topics/approaches | ✅ **Original** |
| 30—60% | Some common terminology, likely independent | ⚠️ **Low Concern** |
| 60—85% | Significant structural/lexical similarity | ❌ **Likely Plagiarism** |
| 85%+ | Nearly identical documents | ❌ **Definite Plagiarism** |

**Default Threshold:** 60% (reports ≥60% as suspicious)

### Report Contents

Each report includes:

1. **Similarity Score** - Overall cosine similarity percentage
2. **Plagiarism Verdict** - Above/below threshold
3. **Common Terms** - Key overlapping words/phrases (highlighted)
4. **Structure Analysis** - For code: matching control flow patterns
5. **File Metadata** - Names, sizes, line counts

---

## 🔧 Configuration

### Change Plagiarism Threshold

Edit [main.cpp](main.cpp) around line 80:

```cpp
const double PLAGIARISM_THRESHOLD = 60.0;  // Change to desired value
```

Then rebuild:
```bash
make clean && make
```

### Supported File Types

| Extension | Support | Use Case |
|---|---|---|
| `.cpp`, `.cc`, `.cxx` | ✅ Full | C++ files with AST analysis |
| `.h` | ✅ Full | C++ headers |
| `.c` | ✅ Full | C source files |
| `.txt` | ✅ Full | Plain text documents |

> **Note:** Files are processed as text; AST analysis applies to syntactically valid code.

---

## 🔬 Technical Details

### N-Gram Algorithm

1. **Tokenization** - Split text into words, remove special characters
2. **Stop-word Removal** - Exclude common words (the, is, are, etc.)
3. **Frequency Vector** - Count occurrences of each token
4. **Cosine Similarity** - Calculate angle between vectors:
   
   `similarity = (A·B) / (||A|| × ||B||)`

5. **Higher n-grams** - Optional bigram/trigram analysis for phrase-level detection

### AST Structural Analysis

1. **Token Extraction** - Parse code into syntactic elements
2. **Tree Construction** - Build hierarchy of control structures:
   - Functions, loops, conditionals, assignments
3. **Longest Common Subsequence** - Find matching patterns
4. **Structural Score** - Ratio of matching nodes to total

**Effective for detecting:**
- Variable renames (same logic, different names)
- Code reorganization (same structure, different order)
- Comment removal

---

## 🐛 Troubleshooting

| Issue | Solution |
|---|---|
| `Cannot find module 'express'` | Run `npm install` to install dependencies |
| `npm ERR! code EACCES` | Try `sudo npm install` or check folder permissions |
| `make: command not found` | Install make: `sudo apt install make` (Ubuntu) |
| `detector: command not found` | Run `make` to compile binary |
| Port 5000 in use | Kill process: `fuser -k 5000/tcp` (Linux/Mac) or `netstat -ano` (Windows) |
| PDF download fails | Ensure Puppeteer Chromium is installed; try: `npm install puppeteer --save` |
| `Error spawning Chrome` | If on Linux in Docker/VM, Puppeteer's `--no-sandbox` flag is enabled by default |
| Files not uploading | Check `/uploads` folder exists and is writable |
| Very slow batch analysis | Large files or high file count; consider splitting dataset |

---

## 📁 File Reference

| File | Purpose |
|---|---|
| [main.cpp](main.cpp) | Entry point; handles CLI args, calls detector, renders HTML report |
| [detector.cpp](detector.cpp) | Core algorithms: tokenization, AST, frequency analysis, similarity calculation |
| [detector.h](detector.h) | Function declarations and data structures |
| [server.js](server.js) | Express routes: `/` (UI), `/analyze` (two-file), `/batch-analyze` (batch), `/download-report` (PDF), `/download-batch-report` (batch PDF) |
| [templates/index.html](templates/index.html) | Modern responsive frontend with drag-drop upload and PDF download buttons |
| [Makefile](Makefile) | C++17 compilation with `-O2` optimizations |
| [package.json](package.json) | Node.js dependencies: express, multer, puppeteer |
| [PDF_EXPORT_IMPLEMENTATION.md](PDF_EXPORT_IMPLEMENTATION.md) | Detailed documentation of PDF export feature |
| [QUICK_START.md](QUICK_START.md) | Quick start guide for Node.js setup |

---

## 📝 License & Attribution

This project is open-source and available for educational and research purposes. Create, modify, and distribute freely while ensuring academic integrity.

---

## ✨ Recent Enhancements

- ✅ **Node.js/Express Migration** (v2.0)
  - Migrated from Flask (Python) to Express (Node.js)
  - Improved performance with async/await patterns
  - Puppeteer-based PDF generation with Chromium
  - Better error handling and async route management
  - Removed Python dependencies; now requires only Node.js

- ✅ **PDF Export Feature** (v1.1)
  - Download reports as styled PDFs (both single and batch modes)
  - Professional formatting with headers, footers, and timestamps
  - Toast notifications for user feedback
  - Puppeteer-based HTML-to-PDF conversion
  - See [PDF_EXPORT_IMPLEMENTATION.md](PDF_EXPORT_IMPLEMENTATION.md) for details

## 🎯 Future Enhancements

- Database storage of analysis history
- Machine learning-based threshold optimization
- Support for additional languages (Python, Java, etc.)
- Advanced metrics: semantic similarity, plagiarism detection database
- API documentation for integration with other tools
- Fuzzy hashing for detecting heavily modified copies
- Custom PDF filenames and branding
- Embedded similarity gauges in PDF reports
