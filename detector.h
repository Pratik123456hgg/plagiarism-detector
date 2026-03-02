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

// Reads the entire content of a file into a string
std::string readFile(const std::string &filepath);

// Cleans the text by converting to lowercase and removing punctuation
std::string cleanText(const std::string &text);

// Tokenizes cleaned text into a vector of words (no stopword removal)
std::vector<std::string> tokenize(const std::string &cleanedText);

// Creates a frequency map of words in the given text, optionally removing
// stopwords
std::unordered_map<std::string, int>
getWordFrequencies(const std::string &text, bool removeStopwords = true);

// Builds an n-gram frequency map from a token vector (n=2 bigrams, n=3 trigrams)
std::unordered_map<std::string, int>
getNgramFrequencies(const std::vector<std::string> &words, int n);

// Calculates the cosine similarity and returns overlapping terms
SimilarityResult
calculateDetailedSimilarity(const std::unordered_map<std::string, int> &freq1,
                            const std::unordered_map<std::string, int> &freq2);
} // namespace Detector

#endif // DETECTOR_H
