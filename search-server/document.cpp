#include <iostream>
#include <string>
#include "document.h"

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ document_id = " << document.id
        << ", relevance = " << document.relevance
        << ", rating = " << document.rating << " }";
    return out;
}

void PrintDocument(const Document& document) {
    std::cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << std::endl;
}