#ifndef FILE_IO_HPP
#define FILE_IO_HPP

#include <fstream>
#include <vector>

namespace jrec::io {

std::ifstream openInputBinFile(std::string& filename);

std::ofstream openOutPutBinFile(std::string& filename);

std::vector<char> readFileBuff(std::ifstream& filename);

}  // namespace jrec::io

#endif // FILE_IO_HPP
