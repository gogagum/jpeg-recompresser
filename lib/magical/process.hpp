#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <vector>
#include <array>
#include <cassert>
#include <algorithm>

class Process {
public:
    Process(const std::vector<int> els) : _els(std::move(els)) {
        auto [minIter, maxIter] = std::minmax_element(_els.begin(), _els.end());
        _minEl = *minIter;
        _maxEl = *maxIter;
        assert(_els.size() % 64 == 0);
    }

    struct Ret {
        int offset;
        std::vector<int> nums;
    };

    Ret process() {
        auto iter = _els.cbegin();
        while (iter != _els.cend()) {
            _process64(iter);
        }
        return { _minEl, _ret };
    }

    int esc() const {
        return _maxEl - _minEl + 1;
    }

private:
    void _process64(std::vector<int>::const_iterator& input) {
        std::size_t numNonZero = 64;

        while (numNonZero > 0 && input[numNonZero - 1] == 0) {
            --numNonZero;
        }

        if (numNonZero == 0) {
            _ret.push_back(2 * esc());
        } else {
            for (std::size_t i = 0; i + 1 < numNonZero; ++i) {
                _ret.push_back(input[i] - _minEl);
            }
            _ret.push_back(input[numNonZero-1] - _minEl + esc());
        }

        input += 64;
    }

private:
    std::vector<int> _els;
    std::vector<int> _ret;
    int _minEl;
    int _maxEl;
};

#endif // PROCESS_HPP
