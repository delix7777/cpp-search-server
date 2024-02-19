#include "request_queue.h"
#include "document.h"


RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server)
    , no_results_requests_(0)
    , current_time_(0) {
}
// сделаем "обертки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    const auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
    AddRequest(result.size());
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    const auto result = search_server_.FindTopDocuments(raw_query, status);
    AddRequest((int)result.size());
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    const auto result = search_server_.FindTopDocuments(raw_query);
    AddRequest((int)result.size());
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return RequestQueue::no_results_requests_;
}