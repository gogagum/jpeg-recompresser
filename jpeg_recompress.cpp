#include <iostream>
#include <string>
#include <fstream>

#include "lib/file_io.hpp"
#include "lib/nj/nanojpeg.h"

////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <input-jpeg> <out-binary-file> <quality>" << std::endl;
        return 0;
    }

    std::string inFileName = argv[1];
    std::string outFileName = argv[2];
    int imageQuality = std::stoi(argv[3]);

    try {
        auto inJpeg = jrec::io::openInputBinFile(inFileName);
        auto outCompressed = jrec::io::openOutPutBinFile(outFileName);

        auto buff = jrec::io::readFileBuff(inJpeg);

        std::vector<int> blocks;

        {
            auto blocksInserter = std::back_inserter(blocks);
            if (njDecode(blocksInserter, buff.data(), buff.size())) {
                throw std::runtime_error("Error decoding the input file.");
            }
        }




    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
