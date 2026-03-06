#ifndef DETECTOR_H
#define DETECTOR_H

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace Detector {

struct SimilarityResult {
  double score;
  std::vector<std::string> commonTerms;
};

// --- AST Logic Declarations ---

enum class NodeType {
  PROGRAM,
  FUNCTION,
  IF_CONDITION,
  LOOP_FOR,
  LOOP_WHILE,
  RETURN,
  ASSIGNMENT,
  EXPRESSION
};

struct ASTNode {
  NodeType type;
  std::string label;
  std::vector<ASTNode *> children;

  ~ASTNode() {
    for (auto *c : children) {
      delete c;
    }
  }
};

std::string readFile(const std::string &filepath);

std::vector<std::string> tokenizeCode(const std::string &code);
ASTNode *buildAST(const std::string &code);
SimilarityResult calculateASTSimilarity(ASTNode *root1, ASTNode *root2);

// --- N-Gram Logic Declarations ---

std::string cleanText(const std::string &text);
std::vector<std::string> tokenize(const std::string &cleanedText);
std::unordered_map<std::string, int>
getWordFrequencies(const std::string &text, bool removeStopwords = true);
std::unordered_map<std::string, int>
getNgramFrequencies(const std::vector<std::string> &words, int n);
SimilarityResult
calculateDetailedSimilarity(const std::unordered_map<std::string, int> &freq1,
                            const std::unordered_map<std::string, int> &freq2);
                            
} // namespace Detector

#endif // DETECTOR_H
