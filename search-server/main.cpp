#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>


using namespace std;

constexpr double EPSILON = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const auto& stop_word : stop_words_) {
            if (IsValidWord(stop_word) == false) {
                throw invalid_argument("Invalide characters in stop words"s);
            }
        }
    }
    explicit SearchServer(const string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {
    }

    bool AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("Negative ID in document"s);
        }
        if (documents_.count(document_id)) {
            throw invalid_argument("ID is already in added documents"s);
        }
        if (IsValidWord(document) == false) {
            throw invalid_argument("Invalid characters in document"s);
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        document_ids.push_back(document_id);
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        return true;
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query,
        DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }});
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(/*const*/ int index) const {
        return document_ids.at(index);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        auto result = tuple{ matched_words, documents_.at(document_id).status };
        return result;
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    vector<int> document_ids;
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        if (text.empty() == true) {
            throw invalid_argument("Search is empty"s);
        }
        if (text.back() == '-') {
            throw invalid_argument("Empty query after -"s);
        }

        bool is_minus = false;
        QueryWord result;

        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        if (IsValidWord(text) == false) {
            throw invalid_argument("Errors in query words"s);
        }
        if (text[0] == '-') {
            throw invalid_argument("Two or more - signs"s);
        }
        result = { text, is_minus, IsStopWord(text) };
        return result;
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }

    static bool IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' '; });
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    SearchServer search_server("a and at in on wi\x17th"s);
    search_server.AddDocument(-1, "russian guys and golden dragon"s, DocumentStatus::ACTUAL, { -7, 1, 7, -1 });
    search_server.AddDocument(-1, "russian guys and golden dragon"s, DocumentStatus::ACTUAL, { -7, 1, 7, -1 });
    search_server.AddDocument(1, "russian guys and golden dragon"s, DocumentStatus::ACTUAL, { 3, 5 });
    search_server.AddDocument(2, "a dragon eats meat with a small bee"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "a dragon eat\x17s meat with a small snake"s, DocumentStatus::ACTUAL, { -1, -2, -3 });
    search_server.AddDocument(4, "dragon and snake are good guys"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    search_server.FindTopDocuments("russian -guys"s);
    search_server.FindTopDocuments("--russian guys"s);
    search_server.FindTopDocuments("guys -"s);
    search_server.FindTopDocuments("-"s);
    search_server.MatchDocument("good guys"s, 4);
    search_server.MatchDocument("russian --guys"s, 1);
    search_server.MatchDocument("-golden dragon"s, 1);
    search_server.MatchDocument("dragon snake -"s, 4);
    search_server.MatchDocument("dragon - snake"s, 14);
}
