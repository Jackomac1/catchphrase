#include "CategoryLoader.h"
#include "board/BoardConfig.h"
#include <SD_MMC.h>
#include <algorithm>

static constexpr const char* kMountPoint  = "/sdcard";
static constexpr const char* kCategoryDir = "/sdcard/categories";

bool CategoryLoader::begin() {
    SD_MMC.setPins(BoardConfig::PIN_SD_CLK, BoardConfig::PIN_SD_CMD,
                   BoardConfig::PIN_SD_D0);

    // Try mounting at several frequencies
    const int freqs[] = {SDMMC_FREQ_DEFAULT, 10000, SDMMC_FREQ_PROBING};
    bool mounted = false;
    for (int f : freqs) {
        if (SD_MMC.begin(kMountPoint, /*mode1bit=*/true, /*format_if_failed=*/false, f, 5)) {
            mounted = true;
            break;
        }
        SD_MMC.end();
        delay(50);
    }
    if (!mounted) return false;

    // Scan category directory
    categories_.clear();
    File dir = SD_MMC.open(kCategoryDir);
    if (!dir || !dir.isDirectory()) return false;

    File entry = dir.openNextFile();
    while (entry) {
        String fname = entry.name();  // just filename, not full path
        if (!entry.isDirectory() && fname.endsWith(".txt")) {
            Category cat;
            // Strip extension for display name
            cat.name = fname.substring(0, fname.length() - 4);
            cat.name[0] = toupper(cat.name[0]);  // capitalize first letter
            cat.filename = String(kCategoryDir) + "/" + fname;
            categories_.push_back(cat);
        }
        entry = dir.openNextFile();
    }
    dir.close();
    return !categories_.empty();
}

bool CategoryLoader::loadWords(const Category& cat) {
    words_.clear();
    wordIndex_ = 0;

    File f = SD_MMC.open(cat.filename);
    if (!f) return false;

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            words_.push_back(line);
        }
    }
    f.close();

    if (words_.empty()) return false;
    shuffle();
    return true;
}

String CategoryLoader::nextWord() {
    if (words_.empty()) return "---";
    if (wordIndex_ >= (int)words_.size()) {
        shuffle();
        wordIndex_ = 0;
    }
    return words_[wordIndex_++];
}

void CategoryLoader::shuffle() {
    for (int i = (int)words_.size() - 1; i > 0; i--) {
        int j = random(i + 1);
        std::swap(words_[i], words_[j]);
    }
}
