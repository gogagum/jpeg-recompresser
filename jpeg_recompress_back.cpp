#include <iostream>
#include <string>
#include <fstream>

#include "lib/file_io.hpp"

////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input-jpeg> <out-binary-file>" << std::endl;
        return 0;
    }

    std::string inFileName = argv[1];
    std::string outFileName = argv[2];

    try {
        auto inCompressed = jrec::io::openInputBinFile(inFileName);
        auto outJpeg = jrec::io::openOutPutBinFile(outFileName);

        int imageQuality;
        inCompressed.read(&imageQuality, sizeof(int));
        int width;
        outCompressed.read(&width, sizeof(int));
        int height;
        outCompressed.read(&height, sizeof(int));
        int ncomp;
        outCompressed.read(&ncomp, sizeof(int));



    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}