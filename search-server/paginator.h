#pragma once
#include <algorithm>
#include <vector>
#include <cassert>
#include <iostream>
#include "document.h"

using namespace std::string_literals;

template <typename T_Iterator>
class IteratorRange {
public:
    explicit IteratorRange(T_Iterator begin, T_Iterator end)
        : begin_(begin), end_(end), size_(distance(begin, end)) {}

    T_Iterator begin() const {
        return begin_;
    }
    T_Iterator end() const {
        return end_;
    }
    size_t size() const {
        return size_;
    }
private:
    T_Iterator begin_;
    T_Iterator end_;
    size_t size_;
};

template <typename T_Iterator>
class Paginator {
public:
    Paginator(const T_Iterator& result_begin, const T_Iterator& result_end, size_t size_of_sheet) {
        auto full_size = distance(result_begin, result_end);
        T_Iterator temp_iter = result_begin; // make a temperary iterator
        for (auto i = 0; i < full_size / size_of_sheet; ++i) {
            sheets.push_back(IteratorRange<T_Iterator>(temp_iter, temp_iter + size_of_sheet));
            temp_iter = temp_iter + size_of_sheet;
        }
        if (temp_iter != result_end) {
            sheets.push_back(IteratorRange<T_Iterator>(temp_iter, result_end));
        }
    }
    auto begin() const {
        return sheets.begin();
    }
    auto end() const {
        return sheets.end();
    }
    size_t size() {
        return sheets.size();
    }
private:
    std::vector<IteratorRange<T_Iterator>> sheets;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}