#include "process.hpp"
#include <algorithm>
#include <bits/ranges_algo.h>
#include <cstddef>
#include <cstdint>
#include <iterator>

#include <boost/range/adaptor/transformed.hpp>

////////////////////////////////////////////////////////////////////////////////
auto AcTransform::process(const std::vector<std::int32_t>& acs, std::size_t blockSize) -> Ret {
    assert(acs.size() % blockSize == 0);
    auto ret = Ret();
    for (auto inIter = acs.begin(); inIter < acs.end(); inIter += blockSize) {
        auto endIter = inIter + blockSize;
        while (inIter < endIter && endIter[-1] == 0) {
            --endIter;
        }
        ret.lengthes.push_back(endIter - inIter);
        std::copy(inIter, endIter, std::back_inserter(ret.processed));
    }
    auto [minIter, maxIter] = std::ranges::minmax_element(ret.processed);
    ret.offset = *minIter;
    ret.rng = *maxIter - *minIter + 1;
    for (auto& processedI: ret.processed) {
        processedI -= ret.offset;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
auto AcTransform::mate(std::vector<std::int32_t> &seq,
                       std::size_t mateCount,
                       std::int32_t maxVal) -> MateRet {
    MateRet ret;
    auto iter = seq.begin();
    while (iter + mateCount < seq.end()) {
        ret.mated.push_back(0);
        for (auto groupIter = iter; groupIter < seq.end(); ++groupIter) {
            *ret.mated.rbegin() *= maxVal;
            *ret.mated.rbegin() += *groupIter;
        }
        iter += mateCount;
    } 
    std::copy(iter, seq.end(), std::back_inserter(ret.tail));
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<int> AcTransform::processBack(const std::vector<std::int32_t> &acsProcessed,
                                          const std::vector<std::int32_t> &lengths,
                                          std::int32_t offset) {
    std::vector<int> ret;
    auto acsDeoffset = boost::adaptors::transform(
            acsProcessed, [offset](auto num) { return num + offset; }
        );
    auto acIter = acsDeoffset.begin();
    for (auto length: lengths) {
        std::copy(acIter, acIter + length, std::back_inserter(ret));
        acIter += length;
        std::fill_n(std::back_inserter(ret), 63 - length, 0);
    }
    return ret;                        
}
