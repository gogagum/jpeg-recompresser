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
#include <stdexcept>
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

#include "lib/file_io.hpp"
#include "lib/transform/dc_ac_transform.hpp"
#include "lib/nj/nanojpeg.hpp"

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

        const auto headerSize = njDecode(channels, buff.data(), buff.size());

        //std::cout << "Header size: " << headerSize << std::endl;

        //auto headerCopy = std::vector<char>();
        //std::copy(buff.begin(), buff.begin() + headerSize,
        //          std::back_inserter(headerCopy));

        assert(channels[1].size() * 4 == channels[0].size()
               && "Not 4-2-0.");
        assert(channels[2].size() * 4 == channels[0].size()
               && "Not 4-2-0.");

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

        if (nj.ncomp > 3) {
            throw std::invalid_argument("Unsupported channels count.");
        }

        bc::static_vector<std::size_t, 3> dcOffsetPos(nj.ncomp);
        bc::static_vector<std::size_t, 3> dcRngPos(nj.ncomp);
        bc::static_vector<std::size_t, 3> acOffsetPos(nj.ncomp);
        bc::static_vector<std::size_t, 3> acRngPos(nj.ncomp);
        bc::static_vector<std::size_t, 3> acLengthRngPos(nj.ncomp);

        bc::static_vector<std::size_t, 3> blocksCountPos(nj.ncomp);
        bc::static_vector<std::size_t, 3> dcBitsCountPos(nj.ncomp);
        bc::static_vector<std::size_t, 3> acCountPos(nj.ncomp);
        bc::static_vector<std::size_t, 3> acBitsCountPos(nj.ncomp);
        bc::static_vector<std::size_t, 3> acLengthesBitsCountPos(nj.ncomp);

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
            auto processed = DCACTransform::process(channels[i]);

            std::ofstream chStream("encoded_coeffs_" + std::to_string(i) , std::ios::trunc);
            for (auto coeff : channels[i]) {
                chStream << coeff << std::endl;
            }

            outData.putTToPosition<std::int32_t>(processed.dcOffset,
                                                 dcOffsetPos[i]);
            outData.putTToPosition<std::uint32_t>(processed.dcRng, dcRngPos[i]);
            outData.putTToPosition<std::int32_t>(processed.acOffset,
                                                 acOffsetPos[i]);
            outData.putTToPosition<std::uint32_t>(processed.acRng, acRngPos[i]);
            outData.putTToPosition<std::uint8_t>(processed.acLengthesRng,
                                                 acLengthRngPos[i]);

            assert(processed.acLengthes.size() == processed.dc.size()
                   && "AC lengthes count is not equal with DCs count.");
            outData.putTToPosition<std::uint32_t>(processed.dc.size(),
                                                  blocksCountPos[i]);
            outData.putTToPosition<std::uint32_t>(processed.acProcessed.size(),
                                                  acCountPos[i]);

            // Encode dc.
            auto dcDict = ael::dict::PPMDDictionary(processed.dcRng, 1);
            auto [dcCount, dcBitsCount] =
                ael::ArithmeticCoder::encode(
                    processed.dc, outData, dcDict,
                    logStream
                );
            assert(dcCount == processed.dc.size()
                   && "Encoded number of dcs is not correct.");
            outData.putTToPosition<std::uint32_t>(dcBitsCount,
                                                  dcBitsCountPos[i]);

            // Encode ac.
            auto acDict = ael::dict::PPMDDictionary(processed.acRng, 1);
            auto [acCount, acBitsCount] =
                ael::ArithmeticCoder::encode(
                    processed.acProcessed, outData, acDict,
                    logStream
                );
            assert(acCount == processed.acProcessed.size()
                   && "Encoded number of acs is not correct.");
            outData.putTToPosition<std::uint32_t>(acBitsCount,
                                                  acBitsCountPos[i]);

            // Encode ac lengthes.
            auto acLengthesDict =
                ael::dict::PPMDDictionary(processed.acLengthesRng, 1);
            auto [acLengthesCount, acLengthesBitsCount] =
                ael::ArithmeticCoder::encode(
                    processed.acLengthes, outData, acLengthesDict,
                    logStream
                );
            assert(acLengthesCount == processed.acLengthes.size()
                   && "Encoded number of lengthes is not correct.");
            outData.putTToPosition<std::uint32_t>(acLengthesBitsCount,
                                                  acLengthesBitsCountPos[i]);

            logStream << "AC range: " << processed.acRng << std::endl;
            logStream << "DC range: " << processed.dcRng << std::endl;
            logStream << "AC bits: " << acBitsCount << std::endl;
            logStream << "AC len bits: " << acLengthesBitsCount << std::endl;
            logStream << "DC bits length: " << dcBitsCount << std::endl;
        }

        //outCompressed.write(headerCopy.data(), headerCopy.size());
        outCompressed.write(outData.data<const char>(), outData.size());
    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
