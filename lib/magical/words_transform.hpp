#ifndef WORDS_TRANSFORM_HPP
#define WORDS_TRANSFORM_HPP

#include <cstdint>
#include <vector>

class WordsTransform {
public:
    struct Ret {
        std::vector<std::uint32_t> transforemed;
        std::vector<std::uint32_t> words;
    };
public:
    static Ret process(const std::vector<std::uint32_t>& words);
    static std::vector<std::uint32_t> processBack(
        const std::vector<std::uint32_t>& words,
        const std::vector<std::uint32_t>& wordsMapping);
};

#endif  // WORDS_TRANSFORM_HPP