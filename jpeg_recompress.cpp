#include <algorithm>
#include <bits/ranges_algo.h>
#include <bits/ranges_algobase.h>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <fstream>
#include <ranges>

#include <ael/arithmetic_coder.hpp>
#include <ael/dictionary/adaptive_d_dictionary.hpp>
#include <ael/dictionary/ppmd_dictionary.hpp>
#include <ael/byte_data_constructor.hpp>

#include <applib/file_opener.hpp>
#include <applib/opt_ostream.hpp>

#include <boost/range/adaptor/transformed.hpp>

#include "lib/file_io.hpp"
#include "lib/magical/process.hpp"
#include "lib/nj/nanojpeg.hpp"
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

        std::cout << blocks.size() << std::endl;

        auto width = std::uint32_t(nj.getWidth());
        auto height = std::uint32_t(nj.getHeight());
        auto nComp = std::uint8_t(nj.ncomp);

        auto [dc, ac] = DCAC::separate(blocks);

        auto [acProcessed, acLengthes, acOffset, acRng] = AcTransform::process(ac, 63);

        auto dcOffset = *std::ranges::min_element(dc);
        auto dcMax = *std::ranges::max_element(dc);
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
        auto dcBitsSizePos = outData.saveSpaceForT<std::int32_t>();
        auto acSizePos = outData.saveSpaceForT<std::uint32_t>();
        auto acBitsSizePos = outData.saveSpaceForT<std::int32_t>();
        auto acLengthesSizePos = outData.saveSpaceForT<std::uint32_t>();
        auto acLengthesBitSizePos = outData.saveSpaceForT<std::uint32_t>();

        auto dcDict = ael::dict::PPMDDictionary(dcMax - dcOffset + 1, 1);
        auto dcCoder = ael::ArithmeticCoder();
        
        auto [dcCnt, dcBitsCnt] = dcCoder.encode(dcTransformed, outData, dcDict,
                                              optout::OptOstreamRef{std::cout});

        auto acDict = ael::dict::PPMDDictionary(acRng, 1);
        auto acLengthesDict = ael::dict::PPMDDictionary(64, 1);
        auto acCoder = ael::ArithmeticCoder();
        
        auto [acCnt, acBitsCnt] = 
            acCoder.encode(
                acProcessed, outData, acDict, optout::OptOstreamRef(std::cout));

        auto [acLengthesCnt, acLengthesBitsCnt] =
            acCoder.encode(
                acLengthes, outData, acLengthesDict, optout::OptOstreamRef(std::cout));

        outData.putTToPosition<std::uint32_t>(dcBitsCnt, dcBitsSizePos);
        outData.putTToPosition<std::uint32_t>(acBitsCnt, acBitsSizePos);

        outData.putTToPosition<std::uint32_t>(acCnt, acSizePos);
        outData.putTToPosition<std::uint32_t>(dcCnt, dcSizePos);

        outData.putTToPosition<std::uint32_t>(acLengthesCnt, acLengthesSizePos);
        outData.putTToPosition<std::uint32_t>(acLengthesBitsCnt, acLengthesBitSizePos);

        outCompressed.write(outData.data<const char>(), outData.size());

        std::ofstream acsOs("acs.txt", std::ios::out | std::ios::trunc);
        for (auto acI : acProcessed) {
            acsOs << acI << std::endl;
        }

        std::ofstream acsLengthesOs("acs_lengths.txt", std::ios::out | std::ios::trunc);
        for (auto acLength : acLengthes) {
            acsLengthesOs << acLength << std::endl;
        }

        std::ofstream dcsOs("dcs.txt", std::ios::out | std::ios::trunc);
        for (auto dc : dcTransformed) {
            dcsOs << dc << std::endl;
        }

        std::cout << acRng << std::endl;
        std::cout << acBitsCnt << " " << acLengthesBitsCnt << " " << dcBitsCnt << std::endl;

    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
