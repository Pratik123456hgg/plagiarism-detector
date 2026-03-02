#include "detector.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

namespace Detector {

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
