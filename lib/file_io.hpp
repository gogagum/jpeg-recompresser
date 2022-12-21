#ifndef FILE_IO_HPP
#define FILE_IO_HPP

#include <fstream>
#include <vector>

namespace jrec::io {

std::ifstream openInputBinFile(std::string& filename);

std::ofstream openOutPutBinFile(std::string& filename);

std::vector<char> readFileBuff(std::ifstream& filename);

template <class T>
T readT(std::ifstream& stream) {
    T ret;
    stream.read(reinterpret_cast<char*>(&ret), sizeof(T));
    return ret;
}

std::uint16_t deserializeNumBits(std::ifstream& stream);

template <class T>
void writeT(std::ofstream& stream, const T& toWrite) {
    stream.write(reinterpret_cast<const char*>(&toWrite), sizeof(T));
}



}  // namespace jrec::io

#endif // FILE_IO_HPP
