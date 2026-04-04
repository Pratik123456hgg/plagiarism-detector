#include "detector.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

namespace Detector {

// --- File Reading ---

std::string readFile(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file " << filepath << std::endl;
    return "";
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

  // --- AST Logic Implementations ---

std::vector<std::string> tokenizeCode(const std::string &code) {
  std::vector<std::string> tokens;
  std::string current;
  for (size_t i = 0; i < code.size(); ++i) {
    char c = code[i];
    if (std::isalnum(c) || c == '_') {
      current += c;
    } else {
      if (!current.empty()) {
        tokens.push_back(current);
        current.clear();
      }
      if (!std::isspace(c)) {
        tokens.push_back(std::string(1, c));
      }
    }
  }
  if (!current.empty()) {
    tokens.push_back(current);
  }
  return tokens;
}

ASTNode *buildAST(const std::string &code) {
  auto tokens = tokenizeCode(code);
  ASTNode *root = new ASTNode{NodeType::PROGRAM, "Program", {}};
  std::vector<ASTNode *> stack = {root};
  ASTNode *lastStatement = nullptr;

  for (size_t i = 0; i < tokens.size(); ++i) {
    std::string t = tokens[i];

    if (t == "if") {
      lastStatement = new ASTNode{NodeType::IF_CONDITION, "If-Condition", {}};
      lastStatement->children.push_back(
          new ASTNode{NodeType::EXPRESSION, "Condition", {}});
      stack.back()->children.push_back(lastStatement);
    } else if (t == "switch") {
      // Detect switch statements as a different control flow from if-else
      lastStatement = new ASTNode{NodeType::EXPRESSION, "Switch-Statement", {}};
      lastStatement->children.push_back(
          new ASTNode{NodeType::EXPRESSION, "SwitchExpr", {}});
      stack.back()->children.push_back(lastStatement);
    } else if (t == "for") {
      lastStatement = new ASTNode{NodeType::LOOP_FOR, "Loop: for", {}};
      stack.back()->children.push_back(lastStatement);
    } else if (t == "while") {
      lastStatement = new ASTNode{NodeType::LOOP_WHILE, "Loop: while", {}};
      stack.back()->children.push_back(lastStatement);
    } else if (t == "do") {
      // Distinguish do-while from regular while
      lastStatement = new ASTNode{NodeType::EXPRESSION, "Loop: do-while", {}};
      stack.back()->children.push_back(lastStatement);
    } else if (t == "return") {
      lastStatement = new ASTNode{NodeType::RETURN, "Return", {}};
      stack.back()->children.push_back(lastStatement);
    } else if (t == "=" || t == "++" || t == "--" || t == "+=" || t == "-=") {
      if (stack.size() > 0 && stack.back()->children.empty() && lastStatement) {
        // Approximate as part of the previous statement instead of a new node
      } else {
        lastStatement = new ASTNode{NodeType::ASSIGNMENT, "Assignment", {}};
        stack.back()->children.push_back(lastStatement);
      }
    } else if (t == "int" || t == "void" || t == "double" || t == "float" ||
               t == "char" || t == "bool" || t == "auto" || t == "string") {
      if (i + 2 < tokens.size() && tokens[i + 2] == "(") {
        lastStatement = new ASTNode{
            NodeType::FUNCTION, "Function: " + tokens[i + 1] + "()", {}};
        stack.back()->children.push_back(lastStatement);
        i += 2; // Skip name and '('
      } else {
        lastStatement =
            new ASTNode{NodeType::EXPRESSION,
                        "Declaration: " + tokens[i] + " " +
                            (i + 1 < tokens.size() ? tokens[i + 1] : ""),
                        {}};
        stack.back()->children.push_back(lastStatement);
      }
    } else if (t == "struct" || t == "class") {
      // Detect struct/class definitions
      if (i + 1 < tokens.size()) {
        lastStatement = new ASTNode{NodeType::EXPRESSION, 
                                    std::string(t) + ": " + tokens[i + 1], {}};
        stack.back()->children.push_back(lastStatement);
        i += 1; // Skip struct/class name
      }
    } else if (t == "{") {
      ASTNode *body = new ASTNode{NodeType::EXPRESSION, "Body", {}};
      if (lastStatement) {
        lastStatement->children.push_back(body);
        stack.push_back(body);
        lastStatement = nullptr; // Block consumed
      } else {
        stack.back()->children.push_back(body);
        stack.push_back(body);
      }
    } else if (t == "}") {
      if (stack.size() > 1) {
        stack.pop_back();
      }
      lastStatement = nullptr;
    }
  }
  return root;
}

SimilarityResult calculateASTSimilarity(ASTNode *root1, ASTNode *root2) {
  // ===== REFINEMENT 1 & 5: Extract sequence with node type information =====
  auto getSeqWithType = [](ASTNode *node) {
    std::vector<std::pair<std::string, std::string>> seq; // {label, nodeType}
    std::function<void(ASTNode *, int)> traverse = [&](ASTNode *n, int d) {
      if (!n)
        return;
      
      // Determine node type for penalty calculations
      std::string nodeType = "OTHER";
      if (n->type == NodeType::IF_CONDITION) {
        nodeType = "IF_CONDITION";
      } else if (n->type == NodeType::LOOP_FOR) {
        nodeType = "LOOP_FOR";
      } else if (n->type == NodeType::LOOP_WHILE) {
        nodeType = "LOOP_WHILE";
      } else if (n->type == NodeType::FUNCTION) {
        nodeType = "FUNCTION";
      } else if (n->label.find("Switch-Statement") != std::string::npos) {
        nodeType = "SWITCH";
      } else if (n->label.find("do-while") != std::string::npos) {
        nodeType = "LOOP_DO_WHILE";
      }
      
      std::string indent(d * 2, ' ');
      // Filter out overly generic wrappers so we match structural essence
      if (n->label != "Program" && n->label != "Body" &&
          n->label != "Condition") {
        seq.push_back({indent + "├── " + n->label, nodeType});
      }
      for (auto *c : n->children) {
        traverse(c, d + 1);
      }
    };
    traverse(node, 0);
    return seq;
  };

  auto seq1 = getSeqWithType(root1);
  auto seq2 = getSeqWithType(root2);

  if (seq1.empty() && seq2.empty())
    return {1.0, {}};
  if (seq1.empty() || seq2.empty())
    return {0.0, {}};

  size_t n = seq1.size();
  size_t m = seq2.size();
  
  // Simple LCS matching - exact label matches only
  std::vector<std::vector<size_t>> dp(n + 1, std::vector<size_t>(m + 1, 0));

  auto clean = [](const std::string &s) {
    std::string result = s;
    auto pos = result.find("├── ");
    if (pos != std::string::npos)
      result = result.substr(pos + 6);
    auto colon = result.find(':');
    if (colon != std::string::npos)
      result = result.substr(0, colon);
    return result;
  };

  // Build DP table - exact matches only
  for (size_t i = 1; i <= n; ++i) {
    for (size_t j = 1; j <= m; ++j) {
      const auto &label1 = seq1[i - 1].first;
      const auto &label2 = seq2[j - 1].first;
      
      std::string t1 = clean(label1);
      std::string t2 = clean(label2);
      
      if (t1 == t2 && !t1.empty()) {
        dp[i][j] = dp[i - 1][j - 1] + 1;
      }
      else {
        dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
      }
    }
  }

  double matchedNodes = dp[n][m];
  double maxSize = std::max(n, m);
  double minSize = std::min(n, m);
  
  // Base LCS score
  double baseScore = matchedNodes / maxSize;
  
  // Calculate final score
  double finalScore = baseScore;
  
  // If it's not a perfect match, apply penalties
  if (baseScore < 0.99) {
    // Simple size mismatch penalty: if file sizes differ significantly, reduce score
    double sizeMismatchPenalty = 1.0;
    if (minSize > 0) {
      double sizeRatio = minSize / static_cast<double>(maxSize);
      // If sizes differ by more than 20%, apply penalty
      if (sizeRatio < 0.8) {
        sizeMismatchPenalty = 0.75 + 0.25 * sizeRatio;  // Scale from 0.75 to 1.0
      }
    }
    
    // Conservative scaling: reduce inflated scores (but not for near-perfect matches)
    double conservativeScale = 0.85;
    
    finalScore = baseScore * sizeMismatchPenalty * conservativeScale;
  }

  // Backtrace to find common nodes
  std::vector<std::string> common;
  size_t i = n, j = m;
  while (i > 0 && j > 0) {
    const auto &label1 = seq1[i - 1].first;
    const auto &label2 = seq2[j - 1].first;
    
    std::string t1 = clean(label1);
    std::string t2 = clean(label2);
    
    if (t1 == t2 && !t1.empty()) {
      common.push_back(label1);
      i--;
      j--;
    } else if (i > 0 && j > 0 && dp[i - 1][j] > dp[i][j - 1]) {
      i--;
    } else if (j > 0) {
      j--;
    } else {
      break;
    }
  }
  std::reverse(common.begin(), common.end());

  return {finalScore, common};
}

// --- N-Gram Logic Implementations ---

std::string cleanText(const std::string &text) {
  std::string cleaned;
  for (char c : text) {
    if (std::isalnum(c) || std::isspace(c)) {
      cleaned += std::tolower(c);
    }
  }
  return cleaned;
}

std::vector<std::string> tokenize(const std::string &cleanedText) {
  std::vector<std::string> words;
  std::stringstream ss(cleanedText);
  std::string word;
  while (ss >> word) {
    if (!word.empty())
      words.push_back(word);
  }
  return words;
}

std::unordered_map<std::string, int> getWordFrequencies(const std::string &text,
                                                        bool removeStopwords) {
  std::unordered_map<std::string, int> frequencies;
  std::stringstream ss(text);
  std::string word;

  std::set<std::string> stopwords = {
      "a",  "an", "the",  "and",  "or",  "but",  "is",  "are", "was",  "were",
      "to", "in", "on",   "with", "of",  "it",   "for", "as",  "that", "this",
      "by", "at", "from", "be",   "not", "have", "has", "had", "will", "would"};

  while (ss >> word) {
    if (removeStopwords && stopwords.count(word)) {
      continue;
    }
    frequencies[word]++;
  }
  return frequencies;
}

std::unordered_map<std::string, int>
getNgramFrequencies(const std::vector<std::string> &words, int n) {
  std::unordered_map<std::string, int> freq;
  if ((int)words.size() < n)
    return freq;
  for (size_t i = 0; i + (size_t)n <= words.size(); ++i) {
    std::string ngram = words[i];
    for (int j = 1; j < n; ++j) {
      ngram += " " + words[i + j];
    }
    freq[ngram]++;
  }
  return freq;
}

SimilarityResult
calculateDetailedSimilarity(const std::unordered_map<std::string, int> &freq1,
                            const std::unordered_map<std::string, int> &freq2) {
  long long dotProduct = 0;
  long long mag1 = 0;
  long long mag2 = 0;
  std::vector<std::string> commonTerms;

  for (const auto &pair : freq1) {
    mag1 += pair.second * pair.second;
    auto it = freq2.find(pair.first);
    if (it != freq2.end()) {
      dotProduct += pair.second * it->second;
      commonTerms.push_back(pair.first);
    }
  }

  for (const auto &pair : freq2) {
    mag2 += pair.second * pair.second;
  }

  double score = 0.0;
  if (mag1 != 0 && mag2 != 0) {
    score =
        static_cast<double>(dotProduct) / (std::sqrt(mag1) * std::sqrt(mag2));
  }

  return {score, commonTerms};
}

// --- Batch Mode Implementations ---

FileFingerprint createFingerprint(const std::string &filepath,
                                 const std::string &filename) {
  std::string content = readFile(filepath);
  std::string cleaned = cleanText(content);
  auto freq = getWordFrequencies(cleaned, true);

  // Compute magnitude for cosine similarity
  double mag = 0.0;
  for (const auto &pair : freq) {
    mag += pair.second * pair.second;
  }
  mag = std::sqrt(mag);

  return {filepath, filename, freq, mag};
}

double computeFingerprintSimilarity(const FileFingerprint &fp1,
                                   const FileFingerprint &fp2) {
  if (fp1.norm == 0.0 || fp2.norm == 0.0)
    return 0.0;

  long long dotProduct = 0;
  for (const auto &pair : fp1.unigramFreq) {
    auto it = fp2.unigramFreq.find(pair.first);
    if (it != fp2.unigramFreq.end()) {
      dotProduct += pair.second * it->second;
    }
  }

  return static_cast<double>(dotProduct) / (fp1.norm * fp2.norm);
}

std::vector<FilePair>
filterCandidatePairs(const std::vector<FileFingerprint> &fingerprints) {
  std::vector<FilePair> candidates;
  for (size_t i = 0; i < fingerprints.size(); ++i) {
    for (size_t j = i + 1; j < fingerprints.size(); ++j) {
      double similarity = computeFingerprintSimilarity(fingerprints[i], fingerprints[j]);
      if (similarity >= 0.30) {
        candidates.push_back({(int)i, (int)j, similarity});
      }
    }
  }
  return candidates;
}

BatchSimilarityMatrix conductBatchAnalysis(
    const std::vector<std::string> &filepaths, double fingerprint_threshold,
    double full_analysis_threshold) {
  BatchSimilarityMatrix result;
  result.totalFiles = filepaths.size();

  // Create fingerprints for all files
  std::vector<FileFingerprint> fingerprints;
  for (const auto &filepath : filepaths) {
    size_t lastSlash = filepath.find_last_of("/\\");
    std::string filename = (lastSlash != std::string::npos)
                               ? filepath.substr(lastSlash + 1)
                               : filepath;
    result.filenames.push_back(filename);
    fingerprints.push_back(createFingerprint(filepath, filename));
  }

  // Initialize similarity matrix
  result.matrix.assign(filepaths.size(),
                       std::vector<double>(filepaths.size(), 0.0));
  for (size_t i = 0; i < filepaths.size(); ++i) {
    result.matrix[i][i] = 1.0;
  }

  // Get candidate pairs via fingerprinting
  auto candidates = filterCandidatePairs(fingerprints);

  // Run full analysis on candidate pairs
  for (const auto &pair : candidates) {
    std::string text1 = readFile(filepaths[pair.fileIdx1]);
    std::string text2 = readFile(filepaths[pair.fileIdx2]);

    auto ast1 = buildAST(text1);
    auto ast2 = buildAST(text2);
    auto astResult = calculateASTSimilarity(ast1, ast2);
    delete ast1;
    delete ast2;

    std::string cleaned1 = cleanText(text1);
    std::string cleaned2 = cleanText(text2);
    auto words1 = tokenize(cleaned1);
    auto words2 = tokenize(cleaned2);

    auto freq1 = getWordFrequencies(cleaned1);
    auto freq2 = getWordFrequencies(cleaned2);
    auto bigram1 = getNgramFrequencies(words1, 2);
    auto bigram2 = getNgramFrequencies(words2, 2);
    auto trigram1 = getNgramFrequencies(words1, 3);
    auto trigram2 = getNgramFrequencies(words2, 3);

    auto unigramResult = calculateDetailedSimilarity(freq1, freq2);
    auto bigramResult = calculateDetailedSimilarity(bigram1, bigram2);
    auto trigramResult = calculateDetailedSimilarity(trigram1, trigram2);

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
    double normalizedScore = combinedScore / 100.0;

    result.matrix[pair.fileIdx1][pair.fileIdx2] = normalizedScore;
    result.matrix[pair.fileIdx2][pair.fileIdx1] = normalizedScore;

    if (normalizedScore >= full_analysis_threshold / 100.0) {
      result.suspiciousPairs.push_back(pair);
    }
  }

  result.totalSuspiciousPairs = result.suspiciousPairs.size();

  // Find connected components (clusters)
  findConnectedComponents(result.suspiciousPairs, result.clusters,
                         result.filenames);

  return result;
}

void findConnectedComponents(const std::vector<FilePair> &pairs,
                            std::vector<PlagiarismCluster> &clusters,
                            const std::vector<std::string> &filenames) {
  int n = filenames.size();
  std::vector<bool> visited(n, false);
  std::vector<std::vector<int>> graph(n);
  std::unordered_map<int, int> degree;

  // Build adjacency list
  for (const auto &pair : pairs) {
    graph[pair.fileIdx1].push_back(pair.fileIdx2);
    graph[pair.fileIdx2].push_back(pair.fileIdx1);
    degree[pair.fileIdx1]++;
    degree[pair.fileIdx2]++;
  }

  // BFS-based connected components
  for (int i = 0; i < n; ++i) {
    if (!visited[i] && !graph[i].empty()) {
      PlagiarismCluster cluster;
      std::vector<int> component;
      std::vector<int> queue;
      queue.push_back(i);
      visited[i] = true;

      while (!queue.empty()) {
        int u = queue.back();
        queue.pop_back();
        component.push_back(u);

        for (int v : graph[u]) {
          if (!visited[v]) {
            visited[v] = true;
            queue.push_back(v);
          }
        }
      }

      // Mark probable original as node with highest degree
      int maxDegree = 0;
      int originalIdx = component[0];
      for (int idx : component) {
        if (degree[idx] > maxDegree) {
          maxDegree = degree[idx];
          originalIdx = idx;
        }
      }

      // Create cluster nodes
      for (int idx : component) {
        cluster.members.push_back(
            {idx, filenames[idx], idx == originalIdx, degree[idx]});
      }

      // Add internal pairs to cluster
      for (const auto &pair : pairs) {
        if ((std::find(component.begin(), component.end(), pair.fileIdx1) !=
             component.end()) &&
            (std::find(component.begin(), component.end(), pair.fileIdx2) !=
             component.end())) {
          cluster.internalPairs.push_back(pair);
        }
      }

      clusters.push_back(cluster);
    }
  }
}

} // namespace Detector
