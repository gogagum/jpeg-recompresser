#ifndef PROCESS_BACK_HPP
#define PROCESS_BACK_HPP

#include <array>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <cassert>

class ProcessBack {
public:
    ProcessBack(std::vector<int> els, int offset)
        : _els(els), _offset(offset),
          _esc((*std::max_element(els.begin(), els.end())) / 2) {}

    std::vector<int> process() {
        auto iter = _els.cbegin();
        while (iter != _els.cend()) {
            _process64(iter);
        }
        return _ret;
    }

private:

    void _process64(std::vector<int>::const_iterator& iter) {
        std::size_t written = 0;

        if (*iter != 2 * _esc) {
            for (; written < 64 && *iter < _esc; ++written, ++iter) {
                _ret.push_back(*iter + _offset);
            }
            _ret.push_back(*iter + _offset - _esc);
            ++written;
        }
        ++iter;

        for (; written < 64; ++written) {
            _ret.push_back(0);
        }
    }

private:
    std::vector<int> _els;
    std::vector<int> _ret;
    int _esc;
    int _offset;
};

#endif // PROCESS_BACK_HPP
