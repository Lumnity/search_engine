#pragma once

#include <vector>
#include <string>
#include <map>

struct Entry {
    size_t doc_id, count;
    bool operator ==(const Entry& other) const {
        return (doc_id == other.doc_id && count == other.count);
    }
};

class InvertedIndex {
public:
    InvertedIndex();

    void UpdateDocumentBase(const std::vector<std::string>& input_docs);

    std::vector<Entry> GetWordCount(const std::string& word);

private:
    std::vector<std::string> docs;
    std::map<std::string, std::vector<Entry>> freq_dictionary;

    std::vector<std::string> SplitTextIntoWords(const std::string& text);
};
