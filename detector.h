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

// Creates a frequency map of words in the given text, optionally removing
// stopwords
std::unordered_map<std::string, int>
getWordFrequencies(const std::string &text, bool removeStopwords = true);

// Calculates the cosine similarity and returns overlapping terms
SimilarityResult
calculateDetailedSimilarity(const std::unordered_map<std::string, int> &freq1,
                            const std::unordered_map<std::string, int> &freq2);
} // namespace Detector

#endif // DETECTOR_H
