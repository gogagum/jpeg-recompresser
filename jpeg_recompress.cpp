#include <algorithm>
#include <array>
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
#include <ael/numerical_coder.hpp>
#include <ael/dictionary/ppmd_dictionary.hpp>
#include <ael/byte_data_constructor.hpp>

#include <applib/file_opener.hpp>
#include <applib/opt_ostream.hpp>

#include <boost/range/adaptor/transformed.hpp>

#include "ael/numerical_coder.hpp"
#include "lib/file_io.hpp"
#include "lib/magical/process.hpp"
#include "lib/nj/nanojpeg.hpp"
#include "lib/magical/dc_ac.hpp"

namespace bc = boost::container;

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
        auto logStream = optout::OptOstreamRef{std::cout};

        auto inJpeg = jrec::io::openInputBinFile(inFileName);
        auto outCompressed = jrec::io::openOutPutBinFile(outFileName);

        auto buff = jrec::io::readWholeFile(inJpeg);
        auto outData = ael::ByteDataConstructor();

        using Channels = std::vector<std::vector<int>>;

        Channels channels; 

        njDecode(channels, buff.data(), buff.size());

        std::cout << "Channels count: " << channels.size() << std::endl;
        for (auto& channel: channels) {
            std::cout << "Channel size: " << channel.size() << std::endl; 
        }

        std::cout << nj.ncomp << std::endl;

        auto width = std::uint32_t(nj.getWidth());
        auto height = std::uint32_t(nj.getHeight());
        auto nComp = std::uint8_t(nj.ncomp);

        outData.putT(imageQuality);
        outData.putT(width);
        outData.putT(height);
        outData.putT(nComp);

        bc::static_vector<std::size_t, 8> dcOffsetPos(nj.ncomp);
        bc::static_vector<std::size_t, 8> dcRngPos(nj.ncomp);
        bc::static_vector<std::size_t, 8> acOffsetPos(nj.ncomp);
        bc::static_vector<std::size_t, 8> acRngPos(nj.ncomp);
        bc::static_vector<std::size_t, 8> acLengthRngPos(nj.ncomp);

        bc::static_vector<std::size_t, 8> blocksCountPos(nj.ncomp);
        bc::static_vector<std::size_t, 8> dcBitsCountPos(nj.ncomp);
        bc::static_vector<std::size_t, 8> acCountPos(nj.ncomp);
        bc::static_vector<std::size_t, 8> acBitsCountPos(nj.ncomp);
        bc::static_vector<std::size_t, 8> acLengthesBitsCountPos(nj.ncomp);

        for (std::size_t i = 0; i < nj.ncomp; ++i) {
            dcOffsetPos[i] = outData.saveSpaceForT<std::int32_t>();
            dcRngPos[i] = outData.saveSpaceForT<std::uint32_t>();
            acOffsetPos[i] = outData.saveSpaceForT<std::int32_t>();
            acRngPos[i] = outData.saveSpaceForT<std::uint32_t>();
            acLengthRngPos[i] = outData.saveSpaceForT<std::uint8_t>();

            blocksCountPos[i] = outData.saveSpaceForT<std::uint32_t>();
            dcBitsCountPos[i] = outData.saveSpaceForT<std::uint32_t>();
            acCountPos[i] = outData.saveSpaceForT<std::uint32_t>();
            acBitsCountPos[i] = outData.saveSpaceForT<std::uint32_t>();
            acLengthesBitsCountPos[i] = outData.saveSpaceForT<std::uint32_t>();
        }

        for (std::size_t i = 0; i < nj.ncomp; ++i) {

            std::ofstream channelOfs("channel" + std::to_string(i) + ".txt", std::ios::out | std::ios::trunc);
            for (auto coeff : channels[i]) {
                channelOfs << coeff << std::endl;
            }

            /**
             * dcMoved
             * dcOffset
             * dcRng
             * acMoved
             * acOffset
             * acRng
             * acLengthes
             * acLengthesRng
             */
            auto processed = ACDCTransform::process(channels[i]);

            outData.putTToPosition<std::int32_t>(processed.dcOffset, dcOffsetPos[i]);
            outData.putTToPosition<std::uint32_t>(processed.dcRng, dcRngPos[i]);
            outData.putTToPosition<std::int32_t>(processed.acOffset, acOffsetPos[i]);
            outData.putTToPosition<std::uint32_t>(processed.acRng, acRngPos[i]);
            outData.putTToPosition<std::uint8_t>(processed.acLengthesRng, acLengthRngPos[i]);

            assert(processed.acLengthes.size() == processed.dc.size()
                   && "AC lengthes count is not equal with DCs count.");
            outData.putTToPosition<std::uint32_t>(processed.dc.size(), blocksCountPos[i]);
            outData.putTToPosition<std::uint32_t>(processed.acProcessed.size(), acCountPos[i]);

            // Encode dc.
            auto dcDict = ael::dict::PPMDDictionary(processed.dcRng, 1);
            auto [dcCount, dcBitsCount] =
                ael::ArithmeticCoder::encode(
                    processed.dc, outData, dcDict,
                    logStream
                );
            assert(dcCount == processed.dc.size()
                   && "Encoded number of dcs is not correct.");
            outData.putTToPosition<std::uint32_t>(dcBitsCount, dcBitsCountPos[i]);

            // Encode ac.
            auto acDict = ael::dict::PPMDDictionary(processed.acRng, 1);
            auto [acCount, acBitsCount] =
                ael::ArithmeticCoder::encode(
                    processed.acProcessed, outData, acDict,
                    logStream
                );
            assert(acCount == processed.acProcessed.size()
                   && "Encoded number of acs is not correct.");
            outData.putTToPosition<std::uint32_t>(acBitsCount, acBitsCountPos[i]);

            // Encode ac lengthes.
            auto acLengthesDict = ael::dict::PPMDDictionary(processed.acLengthesRng, 1);
            auto [acLengthesCount, acLengthesBitsCount] =
                ael::ArithmeticCoder::encode(
                    processed.acLengthes, outData, acLengthesDict,
                    logStream
                );
            assert(acLengthesCount == processed.acLengthes.size()
                   && "Encoded number of lengthes is not correct.");
            outData.putTToPosition<std::uint32_t>(acLengthesBitsCount, acLengthesBitsCountPos[i]);

            std::ofstream acsOs("acs" + std::to_string(i) + ".txt", std::ios::out | std::ios::trunc);
            for (auto acI : processed.acProcessed) {
                acsOs << acI << std::endl;
            }

            std::ofstream acsLengthesOs("acs_lengths" + std::to_string(i) + ".txt", std::ios::out | std::ios::trunc);
            for (auto acLength : processed.acLengthes) {
                acsLengthesOs << static_cast<std::size_t>(acLength) << std::endl;
            }

            std::ofstream dcsOs("dcs" + std::to_string(i) + ".txt", std::ios::out | std::ios::trunc);
            for (auto dc : processed.dc) {
                dcsOs << dc << std::endl;
            }

            logStream << "AC range: " << processed.acRng << std::endl;
            logStream << "DC range: " << processed.dcRng << std::endl;
            logStream << "AC bits: " << acBitsCount << std::endl;
            logStream << "AC lengthes bits: " << acLengthesBitsCount << std::endl;
            logStream << "DC bits length: " << dcBitsCount << std::endl;
        }

        outCompressed.write(outData.data<const char>(), outData.size());

    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
