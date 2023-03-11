#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include <array>
#include <cassert>
#include <algorithm>
#include <boost/container/static_vector.hpp>

////////////////////////////////////////////////////////////////////////////////
/// \brief The Process class
///
class DCACTransform2 {
public:

    struct Ret {
        std::vector<std::uint32_t> dc;
        std::int32_t dcOffset;
        std::uint32_t dcRng;
        std::vector<std::uint32_t> acProcessed;
        std::int32_t acOffset;
        std::uint32_t acRng;
        std::vector<std::uint32_t> acLengthes;
        std::uint32_t acLengthesRng;
    };

    static Ret process(const std::vector<std::int32_t>& acs);
    
    static std::vector<std::int32_t> processBack(
        const std::vector<std::uint32_t>& dcMoved,
        std::int32_t dcOffset,
        const std::vector<std::uint32_t>& acProcessed,
        std::int32_t acOffset,
        const std::vector<std::uint8_t> scLengthes
    );
    
private:

    struct _ACDCSplitRes {
        std::vector<std::int32_t> dc;
        std::vector<std::int32_t> ac;
    };

    struct _RangeMoveRes {
        std::vector<std::uint32_t> coeffs;
        std::int32_t offset;
        std::uint32_t rng;
    };

    struct _ACZipResult {
        std::vector<std::int32_t> acZipped;
        std::vector<std::uint32_t> lengthes;
        std::uint32_t lengthesRng;
    };

    constexpr const static std::array<char, 63> _zigZag = {
    //   0  1
         0, 7,
    //   2   3   4
        15,  8,  1,
    //   5   6   7   8
         2,  9, 16, 23,
    //   9  10  11  12  13 
        31, 24, 17, 10,  3,
    //  14  15  16  17  18  19
         4, 11, 18, 25, 32, 39,
    //  20  21  22  23  24  25  26
        47, 40, 33, 26, 19, 12,  5,
    //  27  28  29  30  31  32  33  34
         6, 13, 20, 27, 34, 41, 48, 55,
    //  35  36  37  38  39  40  41
        56, 49, 42, 35, 28, 21, 14,
    //  42  43  44  45  46  47
        22, 29, 36, 43, 50, 57,
    //  48  49  50  51  52
        58, 51, 44, 37, 30,
    //  53  54  55  56
        38, 45, 52, 59,
    //  57  58  59
        60, 53, 46,
    //  60  61  
        54, 61,
    //  62
        62
    };

    constexpr const static std::array<char, 63> _zigZagBack = {
              0,  4,  5, 13, 14, 26, 27,
          1,  3,  6, 12, 15, 25, 28, 41,
          2,  7, 11, 16, 24, 29, 40, 42,   
          8, 10, 17, 23, 30, 39, 43, 52,
         14, 18, 22, 31, 38, 44, 51, 53, 
         19, 21, 32, 37, 45, 50, 54, 59,
         20, 33, 36, 46, 49, 55, 58, 60,
         34, 35, 47, 48, 56, 57, 61, 62
    };

    static _ACDCSplitRes _splitACDC(const std::vector<std::int32_t>& blocks);
    
    static _RangeMoveRes _rangeMove(const std::vector<std::int32_t>& numbers);
    
    static std::vector<std::int32_t> _acZigZagReorder(
        const std::vector<std::int32_t>& coeffs);

    static _ACZipResult _acZip(
        const std::vector<std::uint32_t>& coeffs,
        std::uint32_t maxOrd,
        std::uint32_t movedZero);
    
    static std::vector<std::int32_t> _joinACDC(
        const std::vector<std::int32_t>& dc,
        const std::vector<std::int32_t>& ac);

    static std::vector<int32_t> _rangeMoveBack(
        const std::vector<std::uint32_t>& numbersMoved,
        std::int32_t offset);

    static std::vector<std::int32_t> _acZigZagReorderBack(
        const std::vector<std::int32_t>& coeffs);

    static std::vector<std::int32_t> _acTailsUnzip(
        const std::vector<std::int32_t>& acCut,
        const std::vector<std::uint8_t>& acLengthes);

    


};

#endif // PROCESS_HPP
