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
class AcTransform {
public:

    struct Ret {
        std::vector<std::int32_t> processed;
        std::vector<std::int32_t> lengthes;
        std::int32_t offset;
        std::int32_t rng;
    };

    struct MateRet {
        std::vector<std::uint32_t> mated;
        boost::container::static_vector<std::uint32_t, 32> tail;
    };

    static Ret process(const std::vector<int>& acs, std::size_t blockSize = 63);
    
    static std::vector<std::int32_t> processBack(const std::vector<std::int32_t>& acsProcessed,
                                        const std::vector<std::int32_t>& lengths,
                                        std::int32_t offset); 


    static MateRet mate(std::vector<std::int32_t>& seq,
                        std::size_t mateCount,
                        std::int32_t maxVal);
};

#endif // PROCESS_HPP
