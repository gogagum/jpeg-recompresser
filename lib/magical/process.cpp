#include "process.hpp"




////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
auto Process::process(std::vector<int>&& els) -> Ret {
    auto processor = Process(std::move(els));
    return processor._process();
}

//----------------------------------------------------------------------------//
Process::Process(std::vector<int>&& els) : _els(std::move(els)) {
    auto [minIter, maxIter] = std::minmax_element(_els.begin(), _els.end());
    _minEl = *minIter;
    _maxEl = *maxIter;
    assert(_els.size() % 64 == 0);
}

//----------------------------------------------------------------------------//
auto Process::_process() -> Ret {
    auto iter = _els.cbegin();
    while (iter != _els.cend()) {
        _process64(iter);
    }
    return { _minEl, _ret };
}

//----------------------------------------------------------------------------//
int Process::esc() const {
    return _maxEl - _minEl + 1;
}

//----------------------------------------------------------------------------//
void Process::_process64(std::vector<int>::const_iterator& input) {
    std::size_t numNonZero = 64;

    for (; numNonZero > 0 && input[numNonZero - 1] == 0; --numNonZero) {}

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
