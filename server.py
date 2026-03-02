import os
import subprocess
from flask import Flask, render_template, request, make_response
from werkzeug.utils import secure_filename

app = Flask(__name__)

# Configure a temporary directory within the project to hold uploaded files
UPLOAD_FOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'uploads')
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

# Ensure the C++ executable exists
DETECTOR_EXEC = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'detector')
if not os.path.exists(DETECTOR_EXEC):
    # Try to build it if it doesn't exist
    subprocess.run(['make', 'clean'], cwd=os.path.dirname(DETECTOR_EXEC))
    subprocess.run(['make'], cwd=os.path.dirname(DETECTOR_EXEC))

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
        cwd=os.path.dirname(DETECTOR_EXEC),
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
    report_path = os.path.join(os.path.dirname(DETECTOR_EXEC), 'report.html')
    if not os.path.exists(report_path):
        return "Error: C++ application did not return a report.html file.", 500
    
    with open(report_path, 'r', encoding='utf-8') as f:
        html_content = f.read()

    # Serve the HTML content directly to the browser
    response = make_response(html_content)
    response.headers['Content-Type'] = 'text/html'
    return response

if __name__ == '__main__':
    # Run the local Flask server
    print("\n\nServer running! Open http://localhost:5000 in your browser.\n\n")
    app.run(host='0.0.0.0', port=5000, debug=True)
