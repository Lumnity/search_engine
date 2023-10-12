#pragma once

#include "InvertedIndex.h"
#include <vector>
#include <string>

class SearchServer {
public:
    struct RelativeIndex {
        size_t doc_id;
        float rank;

        bool operator ==(const RelativeIndex& other) const {
            return (doc_id == other.doc_id && rank == other.rank);
        }
    };

    SearchServer(InvertedIndex& idx);

    std::vector<std::vector<RelativeIndex>> search(const std::vector<std::string>& queries_input);

private:
    InvertedIndex& _index;

    std::vector<std::string> SplitTextIntoWords(const std::string& text);

    std::vector<std::string> GetUniqueWords(const std::vector<std::string>& words);

    float CalculateAbsoluteRelevance(size_t doc_id, const std::vector<std::string>& query_words);
};
