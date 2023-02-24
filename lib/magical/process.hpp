#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <vector>
#include <array>
#include <cassert>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
/// \brief The Process class
///
template <std::size_t blockSize>
class Process {
public:

    struct Ret {
        int offset;
        std::vector<int> nums;
    };

public:

    /**
     * @brief process
     * @param els
     * @return
     */
    static Ret process(std::vector<int>&& els);

private:

    Process(std::vector<int>&& els);

    Ret _process();

    int esc() const;

    void _processBlock(std::vector<int>::const_iterator& input);

private:
    std::vector<int> _els;
    std::vector<int> _ret;
    int _minEl;
    int _maxEl;
};


////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
template <std::size_t blockSize>
auto Process<blockSize>::process(std::vector<int>&& els) -> Ret {
    auto processor = Process(std::move(els));
    return processor._process();
}

//----------------------------------------------------------------------------//
template <std::size_t blockSize>
Process<blockSize>::Process(std::vector<int>&& els) : _els(std::move(els)) {
    auto [minIter, maxIter] = std::minmax_element(_els.begin(), _els.end());
    _minEl = *minIter;
    _maxEl = *maxIter;
    assert(_els.size() % blockSize == 0);
}

//----------------------------------------------------------------------------//
template <std::size_t blockSize>
auto Process<blockSize>::_process() -> Ret {
    auto iter = _els.cbegin();
    while (iter != _els.cend()) {
        _processBlock(iter);
    }
    return { _minEl, _ret };
}

//----------------------------------------------------------------------------//
template <std::size_t blockSize>
int Process<blockSize>::esc() const {
    return _maxEl - _minEl + 1;
}

//----------------------------------------------------------------------------//
template <std::size_t blockSize>
void
Process<blockSize>::_processBlock(std::vector<int>::const_iterator& input) {
    std::size_t numNonZero = blockSize;

    for (; numNonZero > 0 && input[numNonZero - 1] == 0; --numNonZero) {}

    if (numNonZero == 0) {
        _ret.push_back(2 * esc());
    } else {
        for (std::size_t i = 0; i + 1 < numNonZero; ++i) {
            _ret.push_back(input[i] - _minEl);
        }
        _ret.push_back(input[numNonZero-1] - _minEl + esc());
    }

    input += 63;
}



#endif // PROCESS_HPP
