#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>


using json = nlohmann::json;

class ConverterJSON {
public:
    ConverterJSON() {
        LoadConfig();
    }

    std::vector<std::string> GetTextDocuments() {
        std::vector<std::string> documents;
        for (const std::string& filePath : config["files"]) {
            std::ifstream fileStream(filePath);
            if (fileStream.is_open()) {
                std::string document((std::istreambuf_iterator<char>(fileStream)),
                                     std::istreambuf_iterator<char>());
                documents.push_back(document);
                fileStream.close();
            } else {
                std::cerr << "Failed to open file: " << filePath << std::endl;
            }
        }
        return documents;
    }

    int GetResponsesLimit() {
        return config["config"]["max_responses"];
    }

    std::vector<std::string> GetRequests() {
        std::vector<std::string> requests;
        if (requestsJSON.count("requests") > 0) {
            for (const std::string& request : requestsJSON["requests"]) {
                requests.push_back(request);
            }
        }
        return requests;
    }

    void putAnswers(std::vector<std::vector<std::pair<int, float>>> answers) {
        json answersJSON;
        int requestId = 1;
        for (const auto& result : answers) {
            json response;
            response["result"] = !result.empty();
            if (!result.empty()) {
                json relevance;
                int rank = 0;
                for (const auto& docRank : result) {
                    relevance["docid"] = docRank.first;
                    relevance["rank"] = docRank.second;
                    response["relevance"].push_back(relevance);
                    rank++;
                }
            }
            answersJSON["request" + std::to_string(requestId)] = response;
            requestId++;
        }

        std::ofstream outputFile("answers.json");
        outputFile << std::setw(4) << answersJSON << std::endl;
        outputFile.close();
    }

private:
    json config;
    json requestsJSON;

    void LoadConfig() {
        std::ifstream configFile("config.json");
        if (configFile.is_open()) {
            configFile >> config;
            configFile.close();
        } else {
            std::cerr << "Failed to open config file." << std::endl;
        }

        std::ifstream requestsFile("requests.json");
        if (requestsFile.is_open()) {
            requestsFile >> requestsJSON;
            requestsFile.close();
        } else {
            std::cerr << "Failed to open requests file." << std::endl;
        }
    }
};

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <gtest/gtest.h>

struct Entry {
    size_t doc_id, count;
    bool operator ==(const Entry& other) const {
        return (doc_id == other.doc_id && count == other.count);
    }
};

class InvertedIndex {
public:
    InvertedIndex() = default;

    void UpdateDocumentBase(const std::vector<std::string>& input_docs) {
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

    std::vector<Entry> GetWordCount(const std::string& word) {
        if (freq_dictionary.find(word) != freq_dictionary.end()) {
            return freq_dictionary[word];
        } else {
            return {};
        }
    }

private:
    std::vector<std::string> docs;
    std::map<std::string, std::vector<Entry>> freq_dictionary;

    std::vector<std::string> SplitTextIntoWords(const std::string& text) {
        std::vector<std::string> words;
        std::istringstream iss(text);
        std::string word;

        while (iss >> word) {

            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            words.push_back(word);
        }

        return words;
    }
};




using namespace std;

class SearchServer {
public:
    struct RelativeIndex {
        size_t doc_id;
        float rank;

        bool operator ==(const RelativeIndex& other) const {
            return (doc_id == other.doc_id && rank == other.rank);
        }
    };

    SearchServer(InvertedIndex& idx) : _index(idx) {}

    std::vector<std::vector<RelativeIndex>> search(const std::vector<std::string>& queries_input) {
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

private:
    InvertedIndex& _index;

    std::vector<std::string> SplitTextIntoWords(const std::string& text) {
        std::vector<std::string> words;
        std::istringstream iss(text);
        std::string word;

        while (iss >> word) {

            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            words.push_back(word);
        }

        return words;
    }

    std::vector<std::string> GetUniqueWords(const std::vector<std::string>& words) {
        std::vector<std::string> unique_words;
        for (const std::string& word : words) {
            if (std::find(unique_words.begin(), unique_words.end(), word) == unique_words.end()) {
                unique_words.push_back(word);
            }
        }
        return unique_words;
    }

    float CalculateAbsoluteRelevance(size_t doc_id, const std::vector<std::string>& query_words) {
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
};




void TestInvertedIndexFunctionality(
        const std::vector<std::string>& docs,
        const std::vector<std::string>& requests,
        const std::vector<std::vector<Entry>>& expected
) {
    std::vector<std::vector<Entry>> result;
    InvertedIndex idx;
    idx.UpdateDocumentBase(docs);
    for (auto& request : requests) {
        std::vector<Entry> word_count = idx.GetWordCount(request);
        result.push_back(word_count);

        // Выводим результаты на экран
        std::cout << "Results for request: " << request << std::endl;
        for (const Entry& entry : word_count) {
            std::cout << "doc_id: " << entry.doc_id << ", count: " << entry.count << std::endl;
        }
    }

    // Сравниваем результаты с ожидаемыми значениями
    ASSERT_EQ(result, expected);
}

TEST(TestCaseInvertedIndex, TestBasic) {
    const std::vector<std::string> docs = {
            "london is the capital of great britain",
            "big ben is the nickname for the Great bell of the striking clock"
    };
    const std::vector<std::string> requests = {"london", "the"};
    const std::vector<std::vector<Entry>> expected = {
            {
                    {0, 1}
            }, {
                    {0, 1}, {1, 3}
            }
    };
    TestInvertedIndexFunctionality(docs, requests, expected);
}

TEST(TestCaseInvertedIndex, TestBasic2) {
    const std::vector<std::string> docs = {
            "milk milk milk milk water water water",
            "milk water water",
            "milk milk milk milk milk water water water water water",
            "americano cappuccino"
    };
    const std::vector<std::string> requests = {"milk", "water", "cappuccino"};
    const std::vector<std::vector<Entry>> expected = {
            {
                    {0, 4}, {1, 1}, {2, 5}
            }, {
                    {0, 3}, {1, 2}, {2, 5}
            }, {
                    {3, 1}
            }
    };
    TestInvertedIndexFunctionality(docs, requests, expected);
}

// Добавленные тестовые случаи
TEST(TestCaseSearchServer, TestSimple) {
    const std::vector<std::string> docs = {
            "milk milk milk milk water water water",
            "milk water water",
            "milk milk milk milk milk water water water water water",
            "americano cappuccino"
    };
    const std::vector<std::string> request = {"milk water", "sugar"};
    const std::vector<std::vector<SearchServer::RelativeIndex>> expected = {
            {
                    {2, 1},
                    {0, 0.7},
                    {1, 0.3}
            },
            {
            }
    };
    InvertedIndex idx;
    idx.UpdateDocumentBase(docs);
    SearchServer srv(idx);
    std::vector<std::vector<SearchServer::RelativeIndex>> result = srv.search(request);
    ASSERT_EQ(result, expected);
}

TEST(TestCaseSearchServer, TestTop5) {
    const std::vector<std::string> docs = {
            "london is the capital of great britain",
            "paris is the capital of france",
            "berlin is the capital of germany",
            "rome is the capital of italy",
            "madrid is the capital of spain",
            "lisboa is the capital of portugal",
            "bern is the capital of switzerland",
            "moscow is the capital of russia",
            "kiev is the capital of ukraine",
            "minsk is the capital of belarus",
            "astana is the capital of kazakhstan",
            "beijing is the capital of china",
            "tokyo is the capital of japan",
            "bangkok is the capital of thailand",
            "welcome to moscow the capital of russia the third rome",
            "amsterdam is the capital of netherlands",
            "helsinki is the capital of finland",
            "oslo is the capital of norway",
            "stockholm is the capital of sweden",
            "riga is the capital of latvia",
            "tallinn is the capital of estonia",
            "warsaw is the capital of poland",
    };
    const std::vector<std::string> request = {"moscow is the capital of russia"};
    const std::vector<std::vector<SearchServer::RelativeIndex>> expected = {
            {
                    {7, 1},
                    {14, 1},
                    {0, 0.666666687},
                    {1, 0.666666687},
                    {2, 0.666666687}
            }
    };
    InvertedIndex idx;
    idx.UpdateDocumentBase(docs);
    SearchServer srv(idx);
    std::vector<std::vector<SearchServer::RelativeIndex>> result = srv.search(request);

    for (size_t i = 0; i < result.size(); ++i) {
        for (size_t j = 0; j < result[i].size(); ++j) {
            // Сравниваем с плавающей запятой с допустимой погрешностью 1e-5
            ASSERT_NEAR(result[i][j].rank, expected[i][j].rank, 1e-5);
        }
    }
}
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
//int main(int argc, char** argv) {
//    ::testing::InitGoogleTest(&argc, argv);
//    return RUN_ALL_TESTS();
//}
