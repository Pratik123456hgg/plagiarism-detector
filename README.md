# 📄 Plagiarism Detector

A text and code plagiarism detection tool built with **C++** (core engine) and **Python Flask** (web interface). Upload any two text or code files and instantly get a similarity score, plagiarism verdict, and a list of overlapping terms — all presented in a clean HTML report.

---

## 🧠 How It Works

1. The C++ engine reads two files, cleans the text, and computes **word frequency vectors**.
2. It calculates a **cosine-similarity-based score** to determine how alike the two documents are.
3. If the similarity is **≥ 60%**, it flags the pair as plagiarised.
4. Results are rendered in a styled `report.html` which is served back by Flask.

---

## 🗂️ Project Structure

```
plagiarism-detector/
├── main.cpp          # Entry point — reads files, runs detection, generates report.html
├── detector.cpp      # Core detection logic (text cleaning, word freq, similarity)
├── detector.h        # Header for the Detector class
├── server.py         # Flask web server (handles file uploads, runs detector)
├── Makefile          # Build script for the C++ executable
├── templates/
│   └── index.html    # Frontend upload UI served by Flask
├── uploads/          # Temporary folder for uploaded files (auto-created)
├── report.html       # Generated HTML report (overwritten on each run)
├── test1.txt … test6.cpp  # Sample test files
└── venv/             # Python virtual environment
```

---

## ⚙️ Requirements

| Dependency | Purpose |
|---|---|
| `g++` (C++11 or later) | Compile the C++ detector |
| `make` | Build automation |
| `python3` | Run the Flask web server |
| `flask` | Python web framework |
| `werkzeug` | File upload utilities (Flask dependency) |

Install system packages if not already present:
```bash
sudo apt install g++ make python3 python3-pip python3-venv
```

---

## 🚀 Running the Project

### Step 1 — Build the C++ Detector

```bash
cd /path/to/plagiarism-detector
make
```

This compiles `main.cpp` and `detector.cpp` into the `detector` executable.
To rebuild from scratch:
```bash
make clean && make
```

### Step 2 — Set Up the Python Virtual Environment

```bash
# Create a virtual environment (skip if venv/ already exists)
python3 -m venv venv

# Activate it
source venv/bin/activate

# Install dependencies inside the venv
pip install flask werkzeug
```

> ⚠️ **Important:** Always use `pip` from inside the activated venv, not the system `pip`. On Debian/Ubuntu 12+ systems, installing packages directly with `pip` outside a venv will fail with an `externally-managed-environment` error.

### Step 3 — Start the Web Server

```bash
source venv/bin/activate   # if not already activated
python3 server.py
```

You should see:
```
Server running! Open http://localhost:5000 in your browser.
```

### Step 4 — Use the Application

Open **http://localhost:5000** in your browser, upload two text/code files, and click **Analyze**. The plagiarism report will appear immediately.

---

## 🖥️ CLI Usage (Without the Web Server)

You can also run the C++ detector directly from the command line:

```bash
./detector <file1> <file2>
```

**Example:**
```bash
./detector test1.txt test2.txt
```

This generates a `report.html` in the current directory. Open it in any browser to view results.

---

## 🔧 Troubleshooting

| Problem | Fix |
|---|---|
| `python: command not found` | Use `python3` instead of `python` |
| `pip install` fails with `externally-managed-environment` | Activate the venv first: `source venv/bin/activate` |
| `detector: No such file or directory` | Run `make` to compile the C++ binary |
| Port 5000 already in use | Kill the existing process: `fuser -k 5000/tcp` |

---

## 📊 Similarity Threshold

| Score | Verdict |
|---|---|
| < 60% | ✅ Not Plagiarised |
| ≥ 60% | ❌ Plagiarism Detected |

The threshold can be changed in `main.cpp`:
```cpp
const double PLAGIARISM_THRESHOLD = 60.0;
```

---

## 📝 License

This project is open-source and free to use for educational purposes.
