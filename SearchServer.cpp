#include "SearchServer.h"
#include <iostream>
#include <algorithm>

SearchServer::SearchServer(InvertedIndex& idx) : _index(idx) {}

std::vector<std::vector<SearchServer::RelativeIndex>> SearchServer::search(const std::vector<std::string>& queries_input) {
    std::vector<std::vector<RelativeIndex>> result;

    for (const std::string& query : queries_input) {
        std::vector<std::string> words = SplitTextIntoWords(query);
        std::vector<std::string> unique_words = GetUniqueWords(words);

        if (unique_words.empty()) {
            result.push_back({});  // Не найдено ни одного документа
            continue;
        }

        std::sort(unique_words.begin(), unique_words.end(), [this](const std::string& a, const std::string& b) {
            return _index.GetWordCount(a).size() < _index.GetWordCount(b).size();
        });

        std::vector<size_t> matching_docs;

        // Инициализируем matching_docs с идентификаторами всех документов из freq_dictionary
        for (const Entry& entry : _index.GetWordCount(unique_words[0])) {
            matching_docs.push_back(entry.doc_id);
        }

        for (size_t i = 1; i < unique_words.size(); ++i) {
            std::vector<Entry> docs_with_word = _index.GetWordCount(unique_words[i]);
            std::vector<size_t> doc_ids;

            // Извлекаем идентификаторы документов из docs_with_word
            for (const Entry& entry : docs_with_word) {
                doc_ids.push_back(entry.doc_id);
            }

            // Выполняем пересечение идентификаторов документов
            std::vector<size_t> temp_matching_docs;
            std::set_intersection(matching_docs.begin(), matching_docs.end(), doc_ids.begin(), doc_ids.end(), std::back_inserter(temp_matching_docs));
            matching_docs = temp_matching_docs;
        }

        if (matching_docs.empty()) {
            result.push_back({});  // Не найдено ни одного документа
            continue;
        }

        std::vector<RelativeIndex> doc_relevance;
        float max_absolute_relevance = 0.0f;

        for (size_t doc_id : matching_docs) {
            float absolute_relevance = CalculateAbsoluteRelevance(doc_id, unique_words);
            max_absolute_relevance = std::max(max_absolute_relevance, absolute_relevance);
            doc_relevance.push_back({doc_id, absolute_relevance});
        }

        for (RelativeIndex& entry : doc_relevance) {
            entry.rank = entry.rank / max_absolute_relevance; // Рассчитываем относительную релевантность
        }

        std::sort(doc_relevance.begin(), doc_relevance.end(), [](const RelativeIndex& a, const RelativeIndex& b) {
            return a.rank > b.rank;
        });

        result.push_back(doc_relevance);
    }

    return result;
}

float SearchServer::CalculateAbsoluteRelevance(size_t doc_id, const std::vector<std::string>& query_words) {
    float absolute_relevance = 0.0f;

    for (const std::string& word : query_words) {
        std::vector<Entry> word_count = _index.GetWordCount(word);
        for (const Entry& entry : word_count) {
            if (entry.doc_id == doc_id) {
                absolute_relevance += static_cast<float>(entry.count);
            }
        }
    }

    std::cout << "Doc ID: " << doc_id << ", Absolute Relevance: " << absolute_relevance << std::endl;

    return absolute_relevance;
}

std::vector<std::string> SearchServer::SplitTextIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string word;

    while (iss >> word) {

        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        words.push_back(word);
    }

    return words;
}

std::vector<std::string> SearchServer::GetUniqueWords(const std::vector<std::string>& words) {
    std::vector<std::string> unique_words;
    for (const std::string& word : words) {
        if (std::find(unique_words.begin(), unique_words.end(), word) == unique_words.end()) {
            unique_words.push_back(word);
        }
    }
    return unique_words;
}
