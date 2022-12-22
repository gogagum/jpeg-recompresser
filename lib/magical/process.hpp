#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <vector>
#include <array>
#include <cassert>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
/// \brief The Process class
///
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

    void _process64(std::vector<int>::const_iterator& input);

private:
    std::vector<int> _els;
    std::vector<int> _ret;
    int _minEl;
    int _maxEl;
};

#endif // PROCESS_HPP
