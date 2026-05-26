#pragma once
#include <Arduino.h>
#include <vector>

struct Category {
    String name;      // display name
    String filename;  // path on SD card, e.g. "/sdcard/categories/movies.txt"
};

class CategoryLoader {
public:
    bool begin();

    // Returns list of available categories (from /sdcard/categories/)
    const std::vector<Category>& categories() const { return categories_; }

    // Load and shuffle words for a given category. Returns false if file missing.
    bool loadWords(const Category& cat);

    // Returns next word from the loaded deck (cycles and reshuffles at end).
    String nextWord();

    int wordCount() const { return (int)words_.size(); }

private:
    void shuffle();

    std::vector<Category> categories_;
    std::vector<String>   words_;
    int                   wordIndex_ = 0;
};
