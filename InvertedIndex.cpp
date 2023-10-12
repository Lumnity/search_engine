#include "../include/InvertedIndex.h"
#include <algorithm>
#include <sstream>
#include <iostream>

InvertedIndex::InvertedIndex() {

}

void InvertedIndex::UpdateDocumentBase(const std::vector<std::string>& input_docs) {
    docs = input_docs;
    freq_dictionary.clear();

    for (size_t doc_id = 0; doc_id < docs.size(); ++doc_id) {
        std::map<std::string, int> word_count;
        std::vector<std::string> words = SplitTextIntoWords(docs[doc_id]);

        for (const std::string& word : words) {
            word_count[word]++;
        }

        for (const auto& pair : word_count) {
            freq_dictionary[pair.first].emplace_back(Entry{static_cast<size_t>(doc_id), static_cast<size_t>(pair.second)});
        }
    }
}

std::vector<Entry> InvertedIndex::GetWordCount(const std::string& word) {
    if (freq_dictionary.find(word) != freq_dictionary.end()) {
        return freq_dictionary[word];
    } else {
        return {};
    }
}

std::vector<std::string> InvertedIndex::SplitTextIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string word;

    while (iss >> word) {
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        words.push_back(word);
    }

    return words;
}
