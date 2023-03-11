#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>
#include <fstream>

#include <boost/container/static_vector.hpp>

#include <ael/arithmetic_decoder.hpp>
#include <ael/data_parser.hpp>
#include <ael/dictionary/ppmd_dictionary.hpp>

#include "applib/file_opener.hpp"
#include "applib/opt_ostream.hpp"
#include "lib/jo/jo_write_jpeg.hpp"
#include "lib/transform/dc_ac_transform.hpp"

namespace bc = boost::container;

////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << std::string(argv[0])
                  << " <input-jpeg> <out-binary-file>" << std::endl;
        return 0;
    }

    std::string inFileName = argv[1];
    std::string outFileName = argv[2];

    try {
        auto logStream = optout::OptOstreamRef{std::cout};

        auto fileOpener = FileOpener(inFileName, outFileName);
        auto inData = ael::DataParser(fileOpener.getInData());

        auto imageQuality = inData.takeT<std::uint32_t>();
        auto width = inData.takeT<std::uint32_t>();
        auto height = inData.takeT<std::uint32_t>();
        auto nComp = inData.takeT<std::uint8_t>();
        
        bc::static_vector<std::int32_t, 3> dcOffset(nComp);
        bc::static_vector<std::uint32_t, 3> dcRng(nComp);
        bc::static_vector<std::int32_t, 3> acOffset(nComp);
        bc::static_vector<std::uint32_t, 3> acRng(nComp);
        bc::static_vector<std::uint16_t, 3> acLengthRng(nComp);

        bc::static_vector<std::uint32_t, 3> blocksCount(nComp);
        bc::static_vector<std::uint32_t, 3> dcBitsCount(nComp);
        bc::static_vector<std::uint32_t, 3> acCount(nComp);
        bc::static_vector<std::uint32_t, 3> acBitsCount(nComp);
        bc::static_vector<std::uint32_t, 3> acLengthesBitsCount(nComp);

        for (std::size_t i = 0; i < nComp; ++i) {
            logStream << "Reading channel " << i << " info: " << std::endl;

            dcOffset[i] = inData.takeT<std::int32_t>();
            logStream << "DC offset: " << dcOffset[i] << std::endl;
            dcRng[i] = inData.takeT<std::uint32_t>();
            logStream << "DC rng: " << dcRng[i] << std::endl;
            acOffset[i] = inData.takeT<std::int32_t>();
            logStream << "AC offset: " << acOffset[i] << std::endl;
            acRng[i] = inData.takeT<std::uint32_t>();
            logStream << "AC rng: " << acRng[i] << std::endl;
            acLengthRng[i] = inData.takeT<std::uint8_t>();
            logStream << "AC length rng: "
                      << static_cast<std::size_t>(acLengthRng[i]) << std::endl;

            blocksCount[i] = inData.takeT<std::uint32_t>();
            dcBitsCount[i] = inData.takeT<std::uint32_t>();
            acCount[i] = inData.takeT<std::uint32_t>();
            acBitsCount[i] = inData.takeT<std::uint32_t>();
            acLengthesBitsCount[i] = inData.takeT<std::uint32_t>();
        }

        bc::static_vector<std::vector<std::int32_t>, 8> channels(nComp);

        for (std::size_t i = 0; i < nComp; ++i) {

            // Decode dc
            auto dcDict = ael::dict::PPMDDictionary(dcRng[i], 1);
            std::vector<std::uint32_t> dcMoved;
            ael::ArithmeticDecoder::decode(inData, dcDict,
                                           std::back_inserter(dcMoved),
                                           blocksCount[i],
                                           dcBitsCount[i],
                                           logStream);

            // Decode ac
            auto acDict = ael::dict::PPMDDictionary(acRng[i], 1);
            std::vector<std::uint32_t> acProcessed;
            ael::ArithmeticDecoder::decode(inData, acDict,
                                           std::back_inserter(acProcessed),
                                           acCount[i],
                                           acBitsCount[i],
                                           logStream);

            // Decode ac lengthes
            auto acLengthesDict = ael::dict::PPMDDictionary(acLengthRng[i], 1);
            std::vector<std::uint8_t> acLengthes;
            ael::ArithmeticDecoder::decode(inData, acLengthesDict,
                                           std::back_inserter(acLengthes),
                                           blocksCount[i],
                                           acLengthesBitsCount[i],
                                           logStream);

            channels[i] = DCACTransform::processBack(
                dcMoved, dcOffset[i], acProcessed, acOffset[i], acLengthes);
        }

        std::vector<std::int32_t> channelsJoined;

        for (std::size_t i = 0; i < channels[0].size() / 256; ++i) {
            std::copy(channels[0].begin() + i * 256,
                      channels[0].begin() + (i + 1) * 256,
                      std::back_inserter(channelsJoined));
            std::copy(channels[1].begin() + i * 64,
                      channels[1].begin() + (i + 1) * 64,
                      std::back_inserter(channelsJoined));
            std::copy(channels[2].begin() + i * 64,
                      channels[2].begin() + (i + 1) * 64,
                      std::back_inserter(channelsJoined));
        }

        auto iter = channelsJoined.begin();

        jo_write_jpg(iter, fileOpener.getOutFileStream(), width, height,
                     nComp, imageQuality);
    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
