#pragma once
#include <iostream>
#include "paginator.h"

struct Document {
    Document() = default;

    Document(int idd, double relevancee, int ratingg)
        : id(idd)
        , relevance(relevancee)
        , rating(ratingg) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

std::ostream& operator<<(std::ostream& out, const Document& document);

template <typename To_Out>
std::ostream& operator<<(std::ostream& out, const IteratorRange<To_Out>& sheet) {
    auto i = sheet.begin();
    while (i != sheet.end()) {
        out << *i;
        ++i;
    }
    return out;
}

void PrintDocument(const Document& document);