#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <string>
#include <fstream>

#include <ael/arithmetic_decoder.hpp>
#include <ael/data_parser.hpp>
#include <ael/dictionary/adaptive_d_contextual_dictionary_improved.hpp>

#include "applib/file_opener.hpp"
#include "lib/jo/jo_write_jpeg.hpp"
#include "lib/magical/process_back.hpp"
#include "lib/magical/dc_ac.hpp"

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
        auto fileOpener = FileOpener(inFileName, outFileName);
        auto inData = ael::DataParser(fileOpener.getInData());

        auto imageQuality = inData.takeT<std::uint32_t>();
        auto width = inData.takeT<std::uint32_t>();
        auto height = inData.takeT<std::uint32_t>();
        auto nComp = inData.takeT<std::uint8_t>();
        auto acOffset = inData.takeT<int>();
        auto dcOffset = inData.takeT<int>();

        const auto dcSize = inData.takeT<std::uint32_t>();
        const auto dcBitsSize = inData.takeT<std::uint32_t>();
        const auto acSize = inData.takeT<std::uint32_t>();
        const auto acBitsSize = inData.takeT<std::uint32_t>();

        auto acDecoder = ael::ArithmeticDecoder();
        auto acDict = ael::dict::AdaptiveDContextualDictionaryImproved(7, 0, 6);

        auto dcDecoder = ael::ArithmeticDecoder();
        auto dcDict = ael::dict::AdaptiveDContextualDictionaryImproved(7, 0, 6);

        std::vector<int> dcProcessed;
        dcDecoder.decode(inData, dcDict, std::back_inserter(dcProcessed),
                         dcSize, dcBitsSize);

        std::vector<int> acProcessed;
        acDecoder.decode(inData, acDict, std::back_inserter(acProcessed),
                         acSize, acBitsSize);
        auto ac = ProcessBack::process(std::move(acProcessed), acOffset, 63);

        std::vector<int> dc;
        std::transform(dcProcessed.begin(), dcProcessed.end(),
                       std::back_inserter(dc),
                       [dcOffset](auto num) { 
                           return num + dcOffset;
                       });

        std::cout << dc.size() << " " << ac.size() << std::endl;
        std::cout << dcSize << " " << acSize << std::endl;

        auto acdc = DCAC::join(dc, ac);
        auto iter = acdc.begin();
        jo_write_jpg(iter, fileOpener.getOutFileStream(), width, height,
                     nComp, imageQuality);
    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
