#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <vector>
#include <array>
#include <algorithm>

class Process {
public:

    constexpr static auto numErased = std::array{ 64, 63, 62, 60, 56, 48 };
    constexpr static std::size_t numEsc = numErased.size();

public:
    Process(const std::vector<int> els) : _els(std::move(els)) {
        auto [minIter, maxIter] = std::minmax_element(_els.begin(), _els.end());
        _minEl = *minIter;
        _maxEl = *maxIter;
    }

    struct Res {
        int offset;
        int escStart;
        std::vector<int> elements;
    };

    Res process() {
        int retOffset = _minEl;

        for (std::size_t i = 0; i < numEsc; ++i) {
            _process(i);
        }

        for (auto& el: _els) {
            el -= _minEl;
        }

        _minEl = 0;
        _maxEl -= retOffset;

        return { retOffset, _maxEl + 1, _els };
    }


private:

    void _process(std::size_t iter) {
        std::vector<int> tmp;
        auto currElsIter = _els.begin();
        while (currElsIter != _els.end()) {
            if (_els.end() < currElsIter + numErased[iter]) {
                for (; currElsIter < _els.end(); ++currElsIter) {
                    tmp.push_back(*currElsIter);
                }
            } else if (checkZeros(currElsIter, iter)) {
                tmp.push_back(_maxEl + iter + 1);
                currElsIter += numErased[iter];
            } else {
                tmp.push_back(*currElsIter);
                ++currElsIter;
            }
        }
        _els = tmp;
    }

    bool checkZeros(std::vector<int>::iterator it, std::size_t iter) {
        for (auto it_ = it; it_ < it + numErased[iter]; ++it_) {
            if (*it_ != 0) {
                return false;
            }
        }
        return true;
    }

private:
    std::vector<int> _els;
    int _minEl;
    int _maxEl;
};

#endif // PROCESS_HPP
