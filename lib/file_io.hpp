#ifndef FILE_IO_HPP
#define FILE_IO_HPP

#include <fstream>
#include <vector>

namespace jrec::io {

std::ifstream openInputBinFile(const std::string& filename);

std::ofstream openOutPutBinFile(const std::string& filename);

std::vector<char> readWholeFile(std::ifstream& file);

std::vector<std::byte> readFileBuff(std::ifstream& filename, std::size_t n);

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
