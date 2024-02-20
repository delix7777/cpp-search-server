#include <numeric>
#include "search_server.h"

bool SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Negative ID in document " + document_id);
    }
    if (documents_.count(document_id)) {
        throw std::invalid_argument(document_id + " ID is already in added documents");
    }

    const std::vector<std::string> words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / (int)words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }

    document_ids.push_back(document_id);
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    return true;
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status]( int document_id, DocumentStatus document_status, int rating) {
        return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return (int)documents_.size();
}

int SearchServer::GetDocumentId(int index) const {
    return document_ids.at((long long unsigned int)index);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
                                                    int document_id) const {
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        if (c >= '\0' && c < ' ') {
            throw std::invalid_argument("Invalid characters in document: " + c);
        }
        return false; });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Invalid characters in word: " + word);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    if (text.empty()) {
        throw std::invalid_argument("Search query is empty");
    }
    if (text.back() == '-') {
        throw std::invalid_argument("Empty query after -");
    }

    bool is_minus = false;
    QueryWord result;

    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (!IsValidWord(text)) {
        throw std::invalid_argument("Errors in query words");
    }
    if (text[0] == '-') {
        throw std::invalid_argument("Two or more - signs");
    }
    result = { text, is_minus, IsStopWord(text) };
    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}