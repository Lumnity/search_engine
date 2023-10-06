#include "ConverterJSON.h"
#include <iostream>
#include <fstream>
#include <iomanip>

ConverterJSON::ConverterJSON() {
    LoadConfig();
}

std::vector<std::string> ConverterJSON::GetTextDocuments() {
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

int ConverterJSON::GetResponsesLimit() {
    return config["config"]["max_responses"];
}

std::vector<std::string> ConverterJSON::GetRequests() {
    std::vector<std::string> requests;
    if (requestsJSON.count("requests") > 0) {
        for (const std::string& request : requestsJSON["requests"]) {
            requests.push_back(request);
        }
    }
    return requests;
}

void ConverterJSON::putAnswers(std::vector<std::vector<std::pair<int, float>>> answers) {
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

void ConverterJSON::LoadConfig() {
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
