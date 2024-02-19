#pragma once
#include <iostream>

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