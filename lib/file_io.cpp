#include <iterator>

#include "file_io.hpp"

namespace jrec::io {

//----------------------------------------------------------------------------//
std::ifstream openInputBinFile(std::string& filename) {
    auto ret = std::ifstream(filename, std::ios::in | std::ios::binary);
    if (!ret.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    return ret;
}

//----------------------------------------------------------------------------//
std::ofstream openOutPutBinFile(std::string& filename){
    auto ret =
        std::ofstream(filename,
                      std::ios::out | std::ios::binary | std::ios::trunc);
    if (!ret.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    return ret;
}

//----------------------------------------------------------------------------//
std::vector<char> readFileBuff(std::ifstream& file) {
    // Stop eating new lines in binary mode!!!
    file.unsetf(std::ios::skipws);

    // get its size:
    std::streampos fileSize;

    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // reserve capacity
    std::vector<char> buff;
    buff.reserve(fileSize);

    // read the data:
    buff.insert(buff.begin(),
               std::istream_iterator<char>(file),
               std::istream_iterator<char>());
    return buff;
}

}  // namespace jrec::io
