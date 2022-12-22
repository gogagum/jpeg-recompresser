#include "process_back.hpp"

////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
std::vector<int> ProcessBack::process(std::vector<int>&& els, int offset) {
    auto processor = ProcessBack(std::move(els), offset, 64);
    return processor._process();
}

//----------------------------------------------------------------------------//
ProcessBack::ProcessBack(std::vector<int>&& els, int offset, int blockSize)
    : _els(std::move(els)),
      _blockSize(blockSize),
      _esc((*std::max_element(_els.begin(), _els.end())) / 2),
      _offset(offset) {}

//----------------------------------------------------------------------------//
std::vector<int> ProcessBack::_process() {
    for (auto iter = _els.cbegin(); iter != _els.cend(); _process64(iter)) {}
    return _ret;
}

//----------------------------------------------------------------------------//
void ProcessBack::_process64(std::vector<int>::const_iterator& iter) {
    std::size_t written = 0;
    if (*iter != 2 * _esc) {
        for (; written < 64 && *iter < _esc; ++written, ++iter) {
            _ret.push_back(*iter + _offset);
        }
        _ret.push_back(*iter + _offset - _esc);
        ++written;
    }
    ++iter;
    _ret.insert(_ret.end(), 64 - written, 0);
}
