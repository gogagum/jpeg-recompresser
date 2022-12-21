#ifndef PROCESS_BACK_HPP
#define PROCESS_BACK_HPP

#include <array>
#include <algorithm>
#include <vector>
#include <cstdint>

class ProcessBack {
public:

    constexpr static auto numErased = std::array{ 64, 63, 62, 60, 56, 48 };
    constexpr static std::size_t numEsc = numErased.size();

public:
    ProcessBack(std::vector<int> els, int escStart, int offset) : _els(els), _escStart(escStart), _offset(offset) {

    }

    std::vector<int> process() {
        std::vector<int> ret;
        for (auto el : _els) {
            if (el >= _escStart) {
                std::size_t escIndex = el - _escStart;
                _processEsc(ret, escIndex);
            } else {
                ret.push_back(el + _offset);
            }

        }
        return ret;
    }

private:

    void _processEsc(std::vector<int>& ret, std::size_t idx) {
        for (std::size_t i = 0; i < numErased[idx]; ++i) {
            ret.push_back(0);
        }
    }


private:
    std::vector<int> _els;
    int _escStart;
    int _offset;
};

#endif // PROCESS_BACK_HPP
