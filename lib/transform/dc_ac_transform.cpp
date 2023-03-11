#include "dc_ac_transform.hpp"
#include <algorithm>
#include <bits/ranges_algo.h>
#include <cstddef>
#include <cstdint>
#include <iterator>

#include <boost/range/adaptor/transformed.hpp>

////////////////////////////////////////////////////////////////////////////////
auto DCACTransform::process(const std::vector<std::int32_t>& coeffs) -> Ret {
    auto [dc, ac] = _splitACDC(coeffs);
    auto acReordered = _acZigZagReorder(ac);
    auto [acCut, acLengthes] = _acTailsZip(acReordered);
    auto [dcMoved, dcOffset, dcRng] = _rangeMove(dc);
    auto [acMoved, acOffset, acRng] = _rangeMove(acCut);
    std::uint8_t acLengthesRng = *std::ranges::max_element(acLengthes) + 1;
    return {
        dcMoved,
        dcOffset,
        dcRng,
        acMoved,
        acOffset,
        acRng,
        acLengthes,
        acLengthesRng
    };
}

////////////////////////////////////////////////////////////////////////////////
std::vector<std::int32_t> DCACTransform::processBack(
        const std::vector<std::uint32_t>& dcMoved,
        std::int32_t dcOffset,
        const std::vector<std::uint32_t>& acProcessed,
        std::int32_t acOffset,
        const std::vector<std::uint8_t> acLengthes) {
    auto dc = _rangeMoveBack(dcMoved, dcOffset);
    auto acCut = _rangeMoveBack(acProcessed, acOffset);
    auto acReordered = _acTailsUnzip(acCut, acLengthes);
    auto ac = _acZigZagReorderBack(acReordered);
    return _joinACDC(dc, ac);
}

////////////////////////////////////////////////////////////////////////////////
auto DCACTransform::_splitACDC(
        const std::vector<std::int32_t>& blocks) -> _ACDCSplitRes {
    _ACDCSplitRes ret;
    assert(blocks.size() % 64 == 0 && "Incorrect blocks size.");
    for (auto it = blocks.begin(); it < blocks.end(); it += 64) {
        ret.dc.push_back(*it);
        std::copy(it + 1, it + 64, std::back_inserter(ret.ac));
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
auto DCACTransform::_rangeMove(
        const std::vector<std::int32_t>& numbers) -> _RangeMoveRes {
    _RangeMoveRes ret;
    const auto [minIter, maxIter] = std::ranges::minmax_element(numbers);
    ret.offset = *minIter;
    ret.rng = *maxIter - *minIter + 1;
    std::transform(
        numbers.begin(), numbers.end(),
        std::back_inserter(ret.coeffs),
        [&ret](std::int32_t num) -> std::uint32_t {
            return num - ret.offset;
        });
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<std::int32_t>
DCACTransform::_acZigZagReorder(const std::vector<std::int32_t>& coeffs) {
    std::vector<std::int32_t> ret;
    for (auto iter = coeffs.begin(); iter < coeffs.end(); iter += 63) {
        for (std::size_t i = 0; i < 63; ++i) {
            ret.push_back(iter[_zigZag[i]]);
        }
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
auto DCACTransform::_acTailsZip(
        const std::vector<std::int32_t>& coeffs) -> _TailsZipResult {
    _TailsZipResult ret;
    for (auto iter = coeffs.begin(); iter < coeffs.end(); iter += 63) {
        auto endIter = iter + 63;
        while (endIter > iter && endIter[-1] == 0) {
            --endIter;
        }
        std::copy(iter, endIter, std::back_inserter(ret.acZipped));
        ret.lengthes.push_back(endIter - iter);
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<std::int32_t> DCACTransform::_joinACDC(
        const std::vector<std::int32_t>& dc,
        const std::vector<std::int32_t>& ac) {
    std::vector<std::int32_t> ret;
    assert(dc.size() * 63 == ac.size());
    for (auto [acIter, dcIter] = std::make_pair(ac.begin(), dc.begin());
         acIter < ac.end() && dcIter < dc.end();
         ++dcIter, acIter += 63) {
        ret.push_back(*dcIter);
        std::copy(acIter, acIter + 63, std::back_inserter(ret));
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<std::int32_t> DCACTransform::_rangeMoveBack(
        const std::vector<std::uint32_t>& numbersMoved,
        std::int32_t offset) {
    std::vector<std::int32_t> ret;        
    std::transform(numbersMoved.begin(), numbersMoved.end(),
                   std::back_inserter(ret),
                   [offset](std::uint32_t num) -> std::int32_t {
                       return static_cast<std::int32_t>(num) + offset; 
                   });
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<std::int32_t>
DCACTransform::_acZigZagReorderBack(const std::vector<std::int32_t>& coeffs) {
    std::vector<std::int32_t> ret;
    for (auto iter = coeffs.begin(); iter < coeffs.end(); iter += 63) {
        for (std::size_t i = 0; i < 63; ++i) {
            ret.push_back(iter[_zigZagBack[i]]);
        }
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<std::int32_t> DCACTransform::_acTailsUnzip(
        const std::vector<std::int32_t>& acCut,
        const std::vector<std::uint8_t>& acLengthes) {
    std::vector<std::int32_t> ret;
    for (auto [acCutIter, lengthesIter] = std::make_pair(acCut.begin(),
                                                         acLengthes.begin());
         lengthesIter < acLengthes.end();
         acCutIter += *lengthesIter,
         ++lengthesIter) {
        std::copy(acCutIter, acCutIter + *lengthesIter, std::back_inserter(ret));
        std::fill_n(std::back_inserter(ret), 63 - *lengthesIter, 0);
    }
    return ret;
}
