#include <algorithm>
#include <bits/ranges_algo.h>
#include <bits/ranges_algobase.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <fstream>
#include <ranges>

#include <ael/arithmetic_coder.hpp>
#include <ael/dictionary/adaptive_d_contextual_dictionary_improved.hpp>
#include <ael/byte_data_constructor.hpp>

#include <applib/file_opener.hpp>
#include <applib/opt_ostream.hpp>

#include <boost/range/adaptor/transformed.hpp>

#include "lib/file_io.hpp"
#include "lib/nj/nanojpeg.hpp"
#include "lib/magical/process.hpp"
#include "lib/magical/dc_ac.hpp"

////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: " << std::string(argv[0])
                  << " <input-jpeg> <out-binary-file> <quality>" << std::endl;
        return 0;
    }

    const auto inFileName = std::string(argv[1]);
    const auto outFileName = std::string(argv[2]);
    const auto imageQuality = std::uint32_t(std::stoul(argv[3]));

    try {
        auto inJpeg = jrec::io::openInputBinFile(inFileName);
        auto outCompressed = jrec::io::openOutPutBinFile(outFileName);

        auto buff = jrec::io::readWholeFile(inJpeg);
        auto outData = ael::ByteDataConstructor();

        std::vector<int> blocks;

        auto blocksInserter = std::back_inserter(blocks);
        njDecode(blocksInserter, buff.data(), buff.size());

        auto width = std::uint32_t(nj.getWidth());
        auto height = std::uint32_t(nj.getHeight());
        auto nComp = std::uint8_t(nj.ncomp);

        auto [dc, ac] = DCAC::separate(blocks);

        auto acOffset = *std::ranges::min_element(ac);
        auto acTransformed = boost::adaptors::transform(
            ac, [acOffset](auto num) { return num - acOffset; } 
        );

        auto dcOffset = *std::ranges::min_element(dc);
        auto dcTransformed = boost::adaptors::transform(
            dc, [dcOffset](auto num) { return num - dcOffset; }
        );

        outData.putT(imageQuality);
        outData.putT(width);
        outData.putT(height);
        outData.putT(nComp);
        outData.putT(acOffset);
        outData.putT(dcOffset);

        auto dcSizePos = outData.saveSpaceForT<std::uint32_t>();
        auto dcBitsInfoPos = outData.saveSpaceForT<std::int32_t>();
        auto acSizePos = outData.saveSpaceForT<std::uint32_t>();
        auto acBitsInfoPos = outData.saveSpaceForT<std::int32_t>();

        auto dcDict = ael::dict::AdaptiveDContextualDictionaryImproved(7, 0, 6);
        auto dcCoder = ael::ArithmeticCoder();
        
        auto [dcCnt, dcBitsCnt] = dcCoder.encode(dcTransformed, outData, dcDict,
                                              optout::OptOstreamRef{std::cout});

        auto acDict = ael::dict::AdaptiveDContextualDictionaryImproved(7, 0, 6);
        auto acCoder = ael::ArithmeticCoder();
        
        auto [acCnt, acBitsCnt] = acCoder.encode(acTransformed, outData, acDict,
                                              optout::OptOstreamRef(std::cout));

        outData.putTToPosition<std::uint32_t>(dcBitsCnt, dcBitsInfoPos);
        outData.putTToPosition<std::uint32_t>(acBitsCnt, acBitsInfoPos);

        outData.putTToPosition<std::uint32_t>(acCnt, acSizePos);
        outData.putTToPosition<std::uint32_t>(dcCnt, dcSizePos);

        outCompressed.write(outData.data<const char>(), outData.size());
    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
