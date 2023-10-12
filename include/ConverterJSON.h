#pragma once

#include <vector>
#include <string>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class ConverterJSON {
public:
    ConverterJSON();

    std::vector<std::string> GetTextDocuments();

    int GetResponsesLimit();

    std::vector<std::string> GetRequests();

    void putAnswers(std::vector<std::vector<std::pair<int, float>>> answers);

private:
    json config;
    json requestsJSON;

    void LoadConfig();
};
