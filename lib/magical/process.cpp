#include "process.hpp"
#include <algorithm>
#include <bits/ranges_algo.h>
#include <cstddef>
#include <cstdint>
#include <iterator>

#include <boost/range/adaptor/transformed.hpp>

////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
auto AcTransform::process(const std::vector<int>& acs, std::size_t blockSize) -> Ret {
    assert(acs.size() % blockSize == 0);
    auto ret = Ret();
    auto [minIter, maxIter] = std::ranges::minmax_element(acs);
    ret.offset = *minIter;
    ret.rng = *maxIter - *minIter + 1;
    for (auto inIter = acs.begin(); inIter < acs.end(); inIter += blockSize) {
        auto endIter = inIter + blockSize;
        while (inIter + 1 < endIter && endIter[-1] == 0) {
            --endIter;
        }
        ret.lengthes.push_back(endIter - inIter);
        std::transform(inIter, endIter, std::back_inserter(ret.processed),
                       [&ret](auto num) { return num - ret.offset; });
    }
    return ret;
}

//----------------------------------------------------------------------------//
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
