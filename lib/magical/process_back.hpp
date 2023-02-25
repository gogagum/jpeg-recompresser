#ifndef PROCESS_BACK_HPP
#define PROCESS_BACK_HPP

#include <array>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <cassert>

////////////////////////////////////////////////////////////////////////////////
/// \brief The ProcessBack class
///
class ProcessBack {
public:

    static std::vector<int> process(std::vector<int>&& els,
                                    int offset,
                                    std::size_t blockSize);

private:

    ProcessBack(std::vector<int>&& els, int offset, int blockSize);

    std::vector<int> _process();

    void _process63(std::vector<int>::const_iterator& iter);

private:
    std::vector<int> _els;
    std::vector<int> _ret;
    int _blockSize;
    int _esc;
    int _offset;
};

#endif // PROCESS_BACK_HPP
