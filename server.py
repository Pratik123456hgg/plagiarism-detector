import os
import subprocess
import shutil
from flask import Flask, render_template, request, make_response, jsonify
from werkzeug.utils import secure_filename
from datetime import datetime

try:
    from weasyprint import HTML, CSS
    WEASYPRINT_AVAILABLE = True
except ImportError:
    WEASYPRINT_AVAILABLE = False

app = Flask(__name__)

# Configure a temporary directory within the project to hold uploaded files
UPLOAD_FOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'uploads')
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

# Ensure the C++ executable exists
DETECTOR_EXEC = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'detector')
PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))

if not os.path.exists(DETECTOR_EXEC):
    # Try to build it if it doesn't exist
    subprocess.run(['make', 'clean'], cwd=PROJECT_DIR)
    subprocess.run(['make'], cwd=PROJECT_DIR)

@app.route('/', methods=['GET'])
def index():
    # Render the beautiful upload UI
    return render_template('index.html')

@app.route('/analyze', methods=['POST'])
def analyze():
    # Check if both files are part of the request
    if 'file1' not in request.files or 'file2' not in request.files:
        return "Missing files in request", 400

    file1 = request.files['file1']
    file2 = request.files['file2']

    if file1.filename == '' or file2.filename == '':
        return "No file selected", 400

    # Secure the filenames and save them temporarily
    filename1 = secure_filename(file1.filename)
    filename2 = secure_filename(file2.filename)
    
    path1 = os.path.join(app.config['UPLOAD_FOLDER'], filename1)
    path2 = os.path.join(app.config['UPLOAD_FOLDER'], filename2)

    file1.save(path1)
    file2.save(path2)

    # Execute the C++ Detector program with the uploaded file paths
    # Set cwd so that the generated 'report.html' is placed correctly
    process = subprocess.run(
        [DETECTOR_EXEC, path1, path2], 
        cwd=PROJECT_DIR,
        capture_output=True, 
        text=True
    )

    # Cleanup the temporarily uploaded texts
    if os.path.exists(path1):
        os.remove(path1)
    if os.path.exists(path2):
        os.remove(path2)

    if process.returncode != 0:
        return f"C++ Application Error:<br><pre>{process.stderr}</pre>", 500

    # Read the dynamically generated report.html created by the C++ app
    report_path = os.path.join(PROJECT_DIR, 'report.html')
    if not os.path.exists(report_path):
        return "Error: C++ application did not return a report.html file.", 500
    
    with open(report_path, 'r', encoding='utf-8') as f:
        html_content = f.read()

    # Serve the HTML content directly to the browser
    response = make_response(html_content)
    response.headers['Content-Type'] = 'text/html'
    return response

@app.route('/batch-analyze', methods=['POST'])
def batch_analyze():
    # Check if files are in request
    if 'files' not in request.files:
        return "Missing files in request", 400
    
    uploaded_files = request.files.getlist('files')
    if len(uploaded_files) == 0:
        return "No files selected", 400
    
    # Clear and prepare uploads folder
    if os.path.exists(app.config['UPLOAD_FOLDER']):
        shutil.rmtree(app.config['UPLOAD_FOLDER'])
    os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)
    
    # Save all uploaded files securely
    saved_paths = []
    for file in uploaded_files:
        if file.filename == '':
            continue
        
        # Only accept supported filetypes
        ext = os.path.splitext(file.filename)[1].lower()
        if ext not in ['.cpp', '.h', '.txt', '.c', '.cc', '.cxx']:
            continue
        
        filename = secure_filename(file.filename)
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)
        saved_paths.append(filepath)
    
    if len(saved_paths) < 2:
        return "Please upload at least 2 supported files (.cpp, .h, .txt, .c, .cc, .cxx)", 400
    
    # Execute batch analysis
    process = subprocess.run(
        [DETECTOR_EXEC, '--batch', app.config['UPLOAD_FOLDER']], 
        cwd=PROJECT_DIR,
        capture_output=True, 
        text=True
    )
    
    if process.returncode != 0:
        return f"C++ Application Error:<br><pre>{process.stderr}</pre>", 500
    
    # Read the generated batch_report.html
    batch_report_path = os.path.join(PROJECT_DIR, 'batch_report.html')
    if not os.path.exists(batch_report_path):
        return "Error: C++ application did not generate batch_report.html", 500
    
    with open(batch_report_path, 'r', encoding='utf-8') as f:
        html_content = f.read()
    
    # Clean up uploads after processing
    if os.path.exists(app.config['UPLOAD_FOLDER']):
        shutil.rmtree(app.config['UPLOAD_FOLDER'])
    os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)
    
    # Serve the HTML report
    response = make_response(html_content)
    response.headers['Content-Type'] = 'text/html'
    return response

@app.route('/download-report', methods=['GET'])
def download_report():
    """Convert report.html to PDF and return as download"""
    if not WEASYPRINT_AVAILABLE:
        return jsonify({'error': 'PDF generation library not available. Please install weasyprint.'}), 500
    
    report_path = os.path.join(PROJECT_DIR, 'report.html')
    if not os.path.exists(report_path):
        return jsonify({'error': 'No report found. Please run an analysis first.'}), 404
    
    try:
        with open(report_path, 'r', encoding='utf-8') as f:
            html_content = f.read()
        
        # Add print-friendly CSS to the HTML before conversion
        html_with_print_css = add_print_styles(html_content)
        
        # Convert HTML to PDF using WeasyPrint
        pdf_bytes = HTML(string=html_with_print_css).write_pdf()
        
        # Create response with PDF file
        response = make_response(pdf_bytes)
        response.headers['Content-Type'] = 'application/pdf'
        response.headers['Content-Disposition'] = 'attachment; filename="plagiarism_report.pdf"'
        response.headers['Content-Length'] = len(pdf_bytes)
        
        return response
    
    except Exception as e:
        return jsonify({'error': f'PDF generation failed: {str(e)}'}), 500

@app.route('/download-batch-report', methods=['GET'])
def download_batch_report():
    """Convert batch_report.html to PDF and return as download"""
    if not WEASYPRINT_AVAILABLE:
        return jsonify({'error': 'PDF generation library not available. Please install weasyprint.'}), 500
    
    batch_report_path = os.path.join(PROJECT_DIR, 'batch_report.html')
    if not os.path.exists(batch_report_path):
        return jsonify({'error': 'No batch report found. Please run a batch analysis first.'}), 404
    
    try:
        with open(batch_report_path, 'r', encoding='utf-8') as f:
            html_content = f.read()
        
        # Add print-friendly CSS to the HTML before conversion
        html_with_print_css = add_print_styles(html_content)
        
        # Convert HTML to PDF using WeasyPrint
        pdf_bytes = HTML(string=html_with_print_css).write_pdf()
        
        # Create response with PDF file
        response = make_response(pdf_bytes)
        response.headers['Content-Type'] = 'application/pdf'
        response.headers['Content-Disposition'] = 'attachment; filename="batch_plagiarism_report.pdf"'
        response.headers['Content-Length'] = len(pdf_bytes)
        
        return response
    
    except Exception as e:
        return jsonify({'error': f'PDF generation failed: {str(e)}'}), 500

def add_print_styles(html_content):
    """Add print-friendly CSS styles to HTML content"""
    print_css = '''
    <style>
        @media print {
            body { background: white !important; color: black !important; }
            .orb { display: none !important; }
            .btn { display: none !important; }
            .page { padding: 0 !important; }
            .card { background: white !important; border: 1px solid #ddd !important; }
        }
        @page {
            size: A4;
            margin: 1cm;
            @bottom-center {
                content: "Generated by Plagiarism Detector | " string(date);
                font-size: 0.8em;
                color: #999;
            }
        }
    </style>
    '''
    
    # Insert print CSS before closing </head> tag
    if '</head>' in html_content:
        html_content = html_content.replace('</head>', print_css + '</head>')
    else:
        # If no head tag, add at the beginning (shouldn't happen with valid HTML)
        html_content = print_css + html_content
    
    return html_content


if __name__ == '__main__':
    # Run the local Flask server
    print("\n\nServer running! Open http://localhost:5000 in your browser.\n\n")
    app.run(host='0.0.0.0', port=5000, debug=True)
