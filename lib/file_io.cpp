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
std::vector<char> readWholeFile(std::ifstream& file) {
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

//----------------------------------------------------------------------------//
std::vector<std::byte> readFileBuff(std::ifstream& file, std::size_t n) {
    std::vector<std::byte> ret(n);
    file.read(reinterpret_cast<char*>(ret.data()), n);

    return ret;
}

//----------------------------------------------------------------------------//
std::uint16_t deserializeNumBits(std::ifstream& stream)  {
    std::uint16_t r1 = readT<unsigned char>(stream);
    std::uint16_t r2 = readT<unsigned char>(stream);

    return (r1 << 8) + r2;
};

}  // namespace jrec::io
