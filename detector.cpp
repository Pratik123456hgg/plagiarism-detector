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
    } else if (t == "for") {
      lastStatement = new ASTNode{NodeType::LOOP_FOR, "Loop: for", {}};
      stack.back()->children.push_back(lastStatement);
    } else if (t == "while") {
      lastStatement = new ASTNode{NodeType::LOOP_WHILE, "Loop: while", {}};
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
  auto getSeq = [](ASTNode *node) {
    std::vector<std::string> seq;
    std::function<void(ASTNode *, int)> traverse = [&](ASTNode *n, int d) {
      if (!n)
        return;
      std::string indent(d * 2, ' ');
      // Filter out overly generic wrappers so we match structural essence
      if (n->label != "Program" && n->label != "Body" &&
          n->label != "Condition") {
        seq.push_back(indent + "├── " + n->label);
      }
      for (auto *c : n->children) {
        traverse(c, d + 1);
      }
    };
    traverse(node, 0);
    return seq;
  };

  auto seq1 = getSeq(root1);
  auto seq2 = getSeq(root2);

  if (seq1.empty() && seq2.empty())
    return {1.0, {}};
  if (seq1.empty() || seq2.empty())
    return {0.0, {}};

  size_t n = seq1.size();
  size_t m = seq2.size();
  std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

  auto clean = [](std::string s) {
    auto pos = s.find("├── ");
    if (pos != std::string::npos)
      s = s.substr(pos + 6);
    auto colon = s.find(':');
    if (colon != std::string::npos)
      s = s.substr(0, colon);
    return s;
  };

  for (size_t i = 1; i <= n; ++i) {
    for (size_t j = 1; j <= m; ++j) {
      std::string t1 = clean(seq1[i - 1]);
      std::string t2 = clean(seq2[j - 1]);
      if (t1 == t2 && !t1.empty()) {
        dp[i][j] = dp[i - 1][j - 1] + 1;
      } else {
        dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
      }
    }
  }

  double score = static_cast<double>(dp[n][m]) / std::max(n, m);

  std::vector<std::string> common;
  size_t i = n, j = m;
  while (i > 0 && j > 0) {
    std::string t1 = clean(seq1[i - 1]);
    std::string t2 = clean(seq2[j - 1]);
    if (t1 == t2 && !t1.empty()) {
      common.push_back(seq1[i - 1]);
      i--;
      j--;
    } else if (dp[i - 1][j] > dp[i][j - 1]) {
      i--;
    } else {
      j--;
    }
  }
  std::reverse(common.begin(), common.end());

  return {score, common};
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

} // namespace Detector
