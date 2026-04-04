const express = require('express');
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const { execSync } = require('child_process');
const puppeteer = require('puppeteer');

const app = express();

// Configuration
const UPLOAD_FOLDER = path.join(__dirname, 'uploads');
const DETECTOR_EXEC = path.join(__dirname, 'detector');
const PROJECT_DIR = __dirname;

// Ensure uploads folder exists
if (!fs.existsSync(UPLOAD_FOLDER)) {
  fs.mkdirSync(UPLOAD_FOLDER, { recursive: true });
}

// Check if detector binary exists, compile if needed
if (!fs.existsSync(DETECTOR_EXEC)) {
  console.log('Detector binary not found. Attempting to build...');
  try {
    execSync('make clean', { cwd: PROJECT_DIR, stdio: 'inherit' });
    execSync('make', { cwd: PROJECT_DIR, stdio: 'inherit' });
  } catch (error) {
    console.error('Failed to compile detector:', error.message);
    process.exit(1);
  }
}

// Configure multer for file uploads
const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    cb(null, UPLOAD_FOLDER);
  },
  filename: (req, file, cb) => {
    // Use secure filename: remove path and special chars
    const sanitized = path.basename(file.originalname).replace(/[^a-zA-Z0-9._-]/g, '_');
    cb(null, sanitized);
  },
});

const upload = multer({ storage });

// ============================================
// Route: GET / — Serve the index.html UI
// ============================================
app.get('/', (req, res) => {
  const indexPath = path.join(__dirname, 'templates', 'index.html');
  res.sendFile(indexPath);
});

// ============================================
// Route: POST /analyze — Two-file comparison
// ============================================
app.post('/analyze', upload.fields([{ name: 'file1' }, { name: 'file2' }]), (req, res) => {
  try {
    // Check if both files are present
    if (!req.files || !req.files['file1'] || !req.files['file2']) {
      return res.status(400).send('Missing files in request');
    }

    const file1 = req.files['file1'][0];
    const file2 = req.files['file2'][0];

    if (!file1.path || !file2.path) {
      return res.status(400).send('No file selected');
    }

    const path1 = file1.path;
    const path2 = file2.path;

    // Execute the C++ detector
    try {
      execSync(`${DETECTOR_EXEC} ${path1} ${path2}`, {
        cwd: PROJECT_DIR,
        stdio: 'pipe',
      });
    } catch (error) {
      // Clean up uploaded files
      try {
        if (fs.existsSync(path1)) fs.unlinkSync(path1);
        if (fs.existsSync(path2)) fs.unlinkSync(path2);
      } catch (cleanupErr) {
        // Ignore cleanup errors
      }

      return res.status(500).send(
        `C++ Application Error:<br><pre>${error.stderr || error.message}</pre>`
      );
    }

    // Clean up temporarily uploaded files
    try {
      if (fs.existsSync(path1)) fs.unlinkSync(path1);
      if (fs.existsSync(path2)) fs.unlinkSync(path2);
    } catch (cleanupErr) {
      // Ignore cleanup errors
    }

    // Read and serve the generated report.html
    const reportPath = path.join(PROJECT_DIR, 'report.html');
    if (!fs.existsSync(reportPath)) {
      return res.status(500).send('Error: C++ application did not generate report.html');
    }

    const htmlContent = fs.readFileSync(reportPath, 'utf-8');
    res.setHeader('Content-Type', 'text/html; charset=utf-8');
    res.send(htmlContent);
  } catch (error) {
    console.error('Error in /analyze route:', error);
    res.status(500).send(`Server Error: ${error.message}`);
  }
});

// ============================================
// Route: POST /batch-analyze — Batch mode
// ============================================
app.post('/batch-analyze', upload.array('files'), (req, res) => {
  try {
    // Check if files are present
    if (!req.files || req.files.length === 0) {
      return res.status(400).send('No files selected');
    }

    // Validate files - check file extensions WITHOUT deleting them yet
    const supportedExts = ['.cpp', '.h', '.txt', '.c', '.cc', '.cxx'];
    let validFileCount = 0;

    for (const file of req.files) {
      const ext = path.extname(file.originalname).toLowerCase();
      if (supportedExts.includes(ext)) {
        validFileCount++;
      }
    }

    if (validFileCount < 2) {
      // Clean up uploaded files since they're invalid
      try {
        for (const file of req.files) {
          if (fs.existsSync(file.path)) {
            fs.unlinkSync(file.path);
          }
        }
      } catch (err) {
        // Ignore cleanup errors
      }
      return res
        .status(400)
        .send('Please upload at least 2 supported files (.cpp, .h, .txt, .c, .cc, .cxx)');
    }

    // Execute batch analysis - files are already in UPLOAD_FOLDER from multer
    try {
      execSync(`${DETECTOR_EXEC} --batch ${UPLOAD_FOLDER}`, {
        cwd: PROJECT_DIR,
        stdio: 'pipe',
      });
    } catch (error) {
      // Clean up uploaded files
      try {
        for (const file of req.files) {
          if (fs.existsSync(file.path)) {
            fs.unlinkSync(file.path);
          }
        }
      } catch (cleanupErr) {
        // Ignore cleanup errors
      }

      return res.status(500).send(
        `C++ Application Error:<br><pre>${error.stderr || error.message}</pre>`
      );
    }

    // Read the generated batch_report.html
    const batchReportPath = path.join(PROJECT_DIR, 'batch_report.html');
    if (!fs.existsSync(batchReportPath)) {
      // Clean up uploaded files
      try {
        for (const file of req.files) {
          if (fs.existsSync(file.path)) {
            fs.unlinkSync(file.path);
          }
        }
      } catch (cleanupErr) {
        // Ignore cleanup errors
      }
      return res
        .status(500)
        .send('Error: C++ application did not generate batch_report.html');
    }

    const htmlContent = fs.readFileSync(batchReportPath, 'utf-8');

    // Clean up uploaded files after successfully processing
    try {
      for (const file of req.files) {
        if (fs.existsSync(file.path)) {
          fs.unlinkSync(file.path);
        }
      }
    } catch (cleanupErr) {
      // Log but continue
      console.warn('Could not fully clean uploaded files:', cleanupErr.message);
    }

    // Serve the HTML report
    res.setHeader('Content-Type', 'text/html; charset=utf-8');
    res.send(htmlContent);
  } catch (error) {
    console.error('Error in /batch-analyze route:', error);
    res.status(500).send(`Server Error: ${error.message}`);
  }
});

// ============================================
// Route: GET /download-report — Download 2-file report as PDF
// ============================================
app.get('/download-report', async (req, res) => {
  const reportPath = path.join(__dirname, 'report.html');
  
  if (!fs.existsSync(reportPath)) {
    return res.status(404).json({ error: 'No report found. Run an analysis first.' });
  }

  let browser;
  try {
    const reportHtml = fs.readFileSync(reportPath, 'utf8');
    
    // Helper: extract first regex match or return fallback
    const extract = (pattern, fallback = 'N/A') => {
      const m = reportHtml.match(pattern);
      return m ? m[1].trim() : fallback;
    };

    // Extract main similarity score — tries multiple patterns
    const score = (() => {
      const patterns = [
        /(\d+\.?\d*)\s*%\s*<\/[^>]+>\s*MATCH/i,
        /MATCH[\s\S]{0,50}?(\d+\.?\d*)\s*%/i,
        />(\d+\.?\d*)%<\/[^>]+>\s*[\s\S]{0,30}?MATCH/i,
        /class="[^"]*score[^"]*"[^>]*>\s*(\d+\.?\d*)/i,
        /(\d+\.?\d*)%[\s\S]{0,20}?MATCH/i
      ];
      for (const p of patterns) {
        const m = reportHtml.match(p);
        if (m) return m[1];
      }
      return '0.0';
    })();

    // Extract file names
    const file1 = (() => {
      const patterns = [
        /SOURCE FILE[\s\S]{0,200}?>([\w.\-]+\.(?:cpp|h|c|txt|cc|cxx))</i,
        /source[_\-]?file[^>]*>[\s]*([^<\s]+\.(?:cpp|h|c|txt|cc|cxx))/i
      ];
      for (const p of patterns) { const m = reportHtml.match(p); if (m) return m[1]; }
      return 'File 1';
    })();

    const file2 = (() => {
      const patterns = [
        /TARGET FILE[\s\S]{0,200}?>([\w.\-]+\.(?:cpp|h|c|txt|cc|cxx))</i,
        /target[_\-]?file[^>]*>[\s]*([^<\s]+\.(?:cpp|h|c|txt|cc|cxx))/i
      ];
      for (const p of patterns) { const m = reportHtml.match(p); if (m) return m[1]; }
      return 'File 2';
    })();

    // Extract individual metric scores
    const unigram = extract(/Unigram[\s\S]{0,100}?(\d+\.?\d*)\s*%/i, 'N/A');
    const bigram  = extract(/Bigram[\s\S]{0,100}?(\d+\.?\d*)\s*%/i,  'N/A');
    const trigram = extract(/Trigram[\s\S]{0,100}?(\d+\.?\d*)\s*%/i, 'N/A');
    const ast     = extract(/AST[\s\S]{0,100}?(\d+\.?\d*)\s*%/i,     'N/A');

    // Extract overlapping terms — get all chip/tag text content
    const termMatches = [...reportHtml.matchAll(/class="[^"]*(?:chip|tag|term|badge|token)[^"]*"[^>]*>([\w]+)</gi)];
    const terms = termMatches.length > 0
      ? [...new Set(termMatches.map(m => m[1]))].slice(0, 20).join(', ')
      : extract(/Overlapping Terms?[\s\S]{0,500}?<\/[^>]+>([\w,\s]+)<\//i, 'None found');

    // Extract verdict
    const verdict = (() => {
      if (reportHtml.match(/High Similarity|Plagiarism Detected|Definite Plagiarism/i)) return 'Plagiarism Detected';
      if (reportHtml.match(/Low Similarity|Original/i)) return 'Low Similarity — Original';
      return parseFloat(score) >= 60 ? 'Plagiarism Detected' : 'Low Similarity — Original';
    })();

    const isPlagiarism = parseFloat(score) >= 60;
    const now = new Date().toLocaleString('en-IN', { timeZone: 'Asia/Kolkata' });

    // Debug log
    console.log('PDF DEBUG:', { score, file1, file2, unigram, bigram, trigram, ast, verdict });

    // Build clean professional PDF HTML
    const cleanHtml = `<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body { font-family: 'Segoe UI', Arial, sans-serif; color: #1a1a2e; background: #ffffff; padding: 40px; line-height: 1.6; }
  
  .header { border-bottom: 3px solid #e0f2fe; padding-bottom: 20px; margin-bottom: 30px; }
  .header h1 { font-size: 28px; color: #1a1a2e; margin-bottom: 5px; }
  .header p { color: #666; font-size: 12px; }
  
  .meta { display: grid; grid-template-columns: repeat(3, 1fr); gap: 15px; margin-bottom: 30px; }
  .meta-box { border: 1px solid #e5e7eb; border-radius: 8px; padding: 15px; background: #f9fafb; }
  .meta-box label { font-size: 11px; text-transform: uppercase; color: #888; letter-spacing: 1px; display: block; margin-bottom: 5px; }
  .meta-box p { font-size: 14px; font-weight: 600; color: #1a1a2e; font-family: 'Courier New', monospace; word-break: break-all; }

  .score-section { text-align: center; padding: 40px 20px; background: linear-gradient(135deg, #f9fafb 0%, #f3f4f6 100%); border-radius: 12px; margin-bottom: 30px; border: 1px solid #e5e7eb; }
  .score-number { font-size: 72px; font-weight: 800; color: ${isPlagiarism ? '#dc2626' : '#16a34a'}; line-height: 1; }
  .score-label { font-size: 13px; color: #888; text-transform: uppercase; letter-spacing: 2px; margin-top: 8px; }
  .verdict-badge { display: inline-block; margin-top: 15px; padding: 10px 24px; border-radius: 20px; font-weight: 700; font-size: 15px; background: ${isPlagiarism ? '#fee2e2' : '#dcfce7'}; color: ${isPlagiarism ? '#991b1b' : '#166534'}; border: 2px solid ${isPlagiarism ? '#dc2626' : '#16a34a'}; }

  .section { margin-bottom: 30px; }
  .section h2 { font-size: 15px; font-weight: 700; color: #1a1a2e; border-left: 4px solid #e0f2fe; padding-left: 12px; margin-bottom: 15px; text-transform: uppercase; letter-spacing: 0.5px; }
  
  table { width: 100%; border-collapse: collapse; font-size: 13px; }
  th { background: #e0f2fe; color: #0c4a6e; padding: 11px 14px; text-align: left; font-weight: 600; }
  td { padding: 11px 14px; border-bottom: 1px solid #e5e7eb; }
  tr:nth-child(even) td { background: #f9fafb; }

  .risk-high { color: #991b1b; font-weight: 700; }
  .risk-low { color: #166534; font-weight: 700; }
  .risk-medium { color: #b45309; font-weight: 700; }

  .footer { margin-top: 50px; padding-top: 15px; border-top: 1px solid #e5e7eb; font-size: 11px; color: #999; display: flex; justify-content: space-between; }

  .interpretation-table td:first-child { font-family: monospace; font-size: 12px; }
</style>
</head>
<body>

<div class="header">
  <h1>📄 Plagiarism Analysis Report</h1>
  <p>Generated on ${now} | Plagiarism Detector v2.0</p>
</div>

<div class="meta">
  <div class="meta-box">
    <label>Source File</label>
    <p>${file1}</p>
  </div>
  <div class="meta-box">
    <label>Target File</label>
    <p>${file2}</p>
  </div>
  <div class="meta-box">
    <label>Detection Threshold</label>
    <p>60%</p>
  </div>
</div>

<div class="score-section">
  <div class="score-number">${score}%</div>
  <div class="score-label">Similarity Score</div>
  <div class="verdict-badge">${isPlagiarism ? '⚠ Plagiarism Detected' : '✓ Original / Low Similarity'}</div>
</div>

<div class="section">
  <h2>Detailed Metrics</h2>
  <table>
    <tr><th>Algorithm</th><th>Score</th><th>Description</th></tr>
    <tr><td>Unigram (Words)</td><td><strong>${unigram}%</strong></td><td>Word-level frequency similarity</td></tr>
    <tr><td>Bigram (Phrases)</td><td><strong>${bigram}%</strong></td><td>Two-word phrase overlap</td></tr>
    <tr><td>Trigram (Logic)</td><td><strong>${trigram}%</strong></td><td>Three-word sequence matching</td></tr>
    <tr><td>AST Structure</td><td><strong>${ast}%</strong></td><td>Code structural similarity (renamed variables)</td></tr>
  </table>
</div>

<div class="section">
  <h2>Overlapping Terms</h2>
  <p style="font-family:monospace; background:#f3f4f6; padding:12px; border-radius:8px; line-height:2; font-size:13px;">
    ${terms}
  </p>
</div>

<div class="section">
  <h2>Score Interpretation Guide</h2>
  <table class="interpretation-table">
    <tr><th>Range</th><th>Interpretation</th><th>Status</th></tr>
    <tr><td>0 – 30%</td><td>Minimal overlap, different topics/approaches</td><td class="risk-low">✓ Original</td></tr>
    <tr><td>30 – 60%</td><td>Some common terminology, likely independent</td><td class="risk-medium">⚠ Low Concern</td></tr>
    <tr><td>60 – 85%</td><td>Significant structural/lexical similarity</td><td class="risk-high">✗ Likely Plagiarism</td></tr>
    <tr><td>85 – 100%</td><td>Nearly identical documents</td><td class="risk-high">✗ Definite Plagiarism</td></tr>
  </table>
</div>

<div class="section">
  <h2>Analysis Summary</h2>
  <table>
    <tr><th>Metric</th><th>Result</th></tr>
    <tr><td>Similarity Score</td><td><strong>${score}%</strong></td></tr>
    <tr><td>Verdict</td><td class="${isPlagiarism ? 'risk-high' : 'risk-low'}">${verdict}</td></tr>
    <tr><td>Source File</td><td>${file1}</td></tr>
    <tr><td>Target File</td><td>${file2}</td></tr>
    <tr><td>Detection Algorithms</td><td>N-Gram Cosine Similarity + AST Structural Analysis</td></tr>
    <tr><td>Threshold Used</td><td>60%</td></tr>
    <tr><td>Report Generated</td><td>${now}</td></tr>
  </table>
</div>

<div class="footer">
  <span>Plagiarism Detector v2.0 — C++ Detection Engine + Node.js/Express</span>
  <span>Confidential — For Academic Use Only</span>
</div>

</body>
</html>`;

    browser = await puppeteer.launch({
      headless: 'new',
      args: ['--no-sandbox', '--disable-setuid-sandbox', '--disable-dev-shm-usage', '--disable-gpu']
    });

    const page = await browser.newPage();
    await page.setContent(cleanHtml, { waitUntil: 'networkidle0', timeout: 30000 });
    await new Promise(resolve => setTimeout(resolve, 500));
    
    const pdf = await page.pdf({ 
      format: 'A4', 
      printBackground: true, 
      margin: { top: '15mm', bottom: '15mm', left: '15mm', right: '15mm' } 
    });
    
    await browser.close();

    if (!pdf || pdf.length === 0) {
      return res.status(500).json({ error: 'PDF generation produced empty file.' });
    }

    res.set({
      'Content-Type': 'application/pdf',
      'Content-Disposition': 'attachment; filename="plagiarism_report.pdf"',
      'Content-Length': pdf.length
    });

    res.end(pdf, 'binary');

  } catch (err) {
    if (browser) await browser.close();
    console.error('PDF generation error:', err.message);
    res.status(500).json({ error: 'PDF generation failed: ' + err.message });
  }
});

// ============================================
// Route: GET /download-batch-report — Download batch report as PDF
// ============================================
app.get('/download-batch-report', async (req, res) => {
  const reportPath = path.join(__dirname, 'batch_report.html');
  
  if (!fs.existsSync(reportPath)) {
    return res.status(404).json({ error: 'No batch report found. Run a batch analysis first.' });
  }

  let browser;
  try {
    const batchReportHtml = fs.readFileSync(reportPath, 'utf8');
    
    // Parse batch report to extract statistics
    const totalFilesMatch = batchReportHtml.match(/(\d+)\s*<\/div>\s*<div class="stat-lbl">Total Files/);
    const suspiciousPairsMatch = batchReportHtml.match(/(\d+)\s*<\/div>\s*<div class="stat-lbl">Suspicious Pairs/);
    const clustersMatch = batchReportHtml.match(/(\d+)\s*<\/div>\s*<div class="stat-lbl">Clusters Found/);
    const suspicionRateMatch = batchReportHtml.match(/([\d.]+)%\s*<\/div>\s*<div class="stat-lbl">Suspicion Rate/);
    
    const totalFiles = totalFilesMatch ? parseInt(totalFilesMatch[1]) : 0;
    const suspiciousPairs = suspiciousPairsMatch ? parseInt(suspiciousPairsMatch[1]) : 0;
    const clusterFound = clustersMatch ? parseInt(clustersMatch[1]) : 0;
    const suspicionRate = suspicionRateMatch ? parseFloat(suspicionRateMatch[1]) : 0;

    const now = new Date().toLocaleString('en-IN', { timeZone: 'Asia/Kolkata' });
    const hasSuspicious = suspiciousPairs > 0;

    // Build clean professional batch PDF HTML
    const cleanHtml = `<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body { font-family: 'Segoe UI', Arial, sans-serif; color: #1a1a2e; background: #ffffff; padding: 40px; line-height: 1.6; }
  
  .header { border-bottom: 3px solid #e0f2fe; padding-bottom: 20px; margin-bottom: 30px; }
  .header h1 { font-size: 28px; color: #1a1a2e; margin-bottom: 5px; }
  .header p { color: #666; font-size: 12px; }
  
  .summary-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 15px; margin-bottom: 30px; }
  .stat-card { border: 1px solid #e5e7eb; border-radius: 8px; padding: 20px; background: #f9fafb; text-align: center; }
  .stat-number { font-size: 36px; font-weight: 800; color: #e0f2fe; }
  .stat-label { font-size: 12px; color: #888; text-transform: uppercase; letter-spacing: 1px; margin-top: 8px; }

  .alert { padding: 15px 20px; border-radius: 8px; margin-bottom: 30px; }
  .alert-success { background: #dcfce7; border: 1px solid #16a34a; color: #166534; }
  .alert-warning { background: #fee2e2; border: 1px solid #dc2626; color: #991b1b; }

  .section { margin-bottom: 30px; }
  .section h2 { font-size: 15px; font-weight: 700; color: #1a1a2e; border-left: 4px solid #e0f2fe; padding-left: 12px; margin-bottom: 15px; text-transform: uppercase; letter-spacing: 0.5px; }
  
  table { width: 100%; border-collapse: collapse; font-size: 13px; }
  th { background: #e0f2fe; color: #0c4a6e; padding: 11px 14px; text-align: left; font-weight: 600; }
  td { padding: 11px 14px; border-bottom: 1px solid #e5e7eb; }
  tr:nth-child(even) td { background: #f9fafb; }

  .risk-high { color: #991b1b; font-weight: 700; }
  .risk-low { color: #166534; font-weight: 700; }

  .footer { margin-top: 50px; padding-top: 15px; border-top: 1px solid #e5e7eb; font-size: 11px; color: #999; display: flex; justify-content: space-between; }

  @media print { body { padding: 30px; } }
</style>
</head>
<body>

<div class="header">
  <h1>📦 Batch Plagiarism Analysis Report</h1>
  <p>Generated on ${now} | Plagiarism Detector v2.0</p>
</div>

<div class="alert ${hasSuspicious ? 'alert-warning' : 'alert-success'}">
  ${hasSuspicious 
    ? `⚠ <strong>Warning:</strong> ${suspiciousPairs} suspicious file pair(s) detected with similarity ≥ 60%. ${clusterFound > 0 ? clusterFound + ' cluster(s) identified.' : ''}`
    : `✓ <strong>Clear:</strong> All files passed plagiarism screening. No suspicious similarities detected.`
  }
</div>

<div class="summary-grid">
  <div class="stat-card">
    <div class="stat-number">${totalFiles}</div>
    <div class="stat-label">Total Files</div>
  </div>
  <div class="stat-card">
    <div class="stat-number">${suspiciousPairs}</div>
    <div class="stat-label">Suspicious Pairs</div>
  </div>
  <div class="stat-card">
    <div class="stat-number">${clusterFound}</div>
    <div class="stat-label">Clusters Found</div>
  </div>
  <div class="stat-card">
    <div class="stat-number">${suspicionRate.toFixed(1)}%</div>
    <div class="stat-label">Suspicion Rate</div>
  </div>
</div>

<div class="section">
  <h2>Analysis Summary</h2>
  <table>
    <tr><th>Metric</th><th>Value</th></tr>
    <tr><td>Files Analyzed</td><td><strong>${totalFiles}</strong></td></tr>
    <tr><td>Total File Pairs</td><td><strong>${totalFiles > 1 ? Math.floor((totalFiles * (totalFiles - 1)) / 2) : 0}</strong></td></tr>
    <tr><td>Suspicious Pairs (≥60%)</td><td class="${hasSuspicious ? 'risk-high' : 'risk-low'}">${suspiciousPairs}</td></tr>
    <tr><td>Files in Clusters</td><td><strong>${clusterFound > 0 ? 'Multiple' : 'None'}</strong></td></tr>
    <tr><td>Detection Threshold</td><td>60%</td></tr>
    <tr><td>Overall Suspicion Rate</td><td class="${hasSuspicious ? 'risk-high' : 'risk-low'}">${suspicionRate.toFixed(1)}%</td></tr>
    <tr><td>Detection Algorithms</td><td>N-Gram Cosine Similarity + AST Structural Analysis</td></tr>
    <tr><td>Report Generated</td><td>${now}</td></tr>
  </table>
</div>

<div class="section">
  <h2>Recommendations</h2>
  <table>
    <tr><th>Scenario</th><th>Recommended Action</th></tr>
    <tr><td>No suspicious pairs</td><td>✓ No further action required. All files passed screening.</td></tr>
    <tr><td>1-2 suspicious pairs</td><td>⚠ Manually review flagged pairs. Possible accidental overlap.</td></tr>
    <tr><td>3+ suspicious pairs</td><td>✗ Investigate immediately. Pattern suggests systematic plagiarism.</td></tr>
    <tr><td>Clusters detected</td><td>✗ High priority. Multiple files show similarity. Review all cluster members.</td></tr>
  </table>
</div>

<div class="footer">
  <span>Plagiarism Detector v2.0 — Batch Analysis | C++ Engine + Node.js/Express</span>
  <span>Confidential — For Academic Use Only</span>
</div>

</body>
</html>`;

    browser = await puppeteer.launch({
      headless: 'new',
      args: ['--no-sandbox', '--disable-setuid-sandbox', '--disable-dev-shm-usage', '--disable-gpu']
    });

    const page = await browser.newPage();
    await page.setContent(cleanHtml, { waitUntil: 'networkidle0', timeout: 30000 });
    await new Promise(resolve => setTimeout(resolve, 500));
    
    const pdf = await page.pdf({ 
      format: 'A4', 
      printBackground: true, 
      margin: { top: '15mm', bottom: '15mm', left: '15mm', right: '15mm' } 
    });
    
    await browser.close();

    if (!pdf || pdf.length === 0) {
      return res.status(500).json({ error: 'PDF generation produced empty file.' });
    }

    res.set({
      'Content-Type': 'application/pdf',
      'Content-Disposition': 'attachment; filename="batch_plagiarism_report.pdf"',
      'Content-Length': pdf.length
    });

    res.end(pdf, 'binary');

  } catch (err) {
    if (browser) await browser.close();
    console.error('PDF generation error:', err.message);
    res.status(500).json({ error: 'PDF generation failed: ' + err.message });
  }
});

// ============================================
// Start the server
// ============================================
const PORT = 5000;
app.listen(PORT, '0.0.0.0', () => {
  console.log(`\n\nServer running! Open http://localhost:${PORT} in your browser.\n\n`);
});
