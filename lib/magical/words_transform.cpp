#include <algorithm>
#include <cstddef>
#include <ranges>
#include <set>
#include <map>

#include "words_transform.hpp"

////////////////////////////////////////////////////////////////////////////////
auto WordsTransform::process(const std::vector<std::uint32_t>& words) -> Ret {
    Ret ret;
    auto wordsSet = std::set(words.begin(), words.end());
    ret.words = std::vector(wordsSet.rbegin(), wordsSet.rend());
    std::map<std::uint32_t, std::uint32_t> mapping;
    for (std::size_t i = 0; i < ret.words.size(); ++i) {
        mapping.emplace(ret.words[i], i);
    }
    std::transform(words.begin(), words.end(),
                   std::back_inserter(ret.transforemed),
                   [&mapping](std::uint32_t originalWord) {
                       return mapping.at(originalWord); 
                   });
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<std::uint32_t> WordsTransform::processBack(
        const std::vector<std::uint32_t> &words,
        const std::vector<std::uint32_t> &wordsMapping) {
    std::vector<std::uint32_t> ret;
    std::transform(words.begin(), words.end(), std::back_inserter(ret),
                   [&wordsMapping](std::uint32_t mappedWord) {
                       return wordsMapping[mappedWord]; 
                   });
    return ret;
}
