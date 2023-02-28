#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include <array>
#include <cassert>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
/// \brief The Process class
///
class AcTransform {
public:

    struct Ret {
        std::vector<std::uint32_t> processed;
        std::vector<std::uint32_t> lengthes;
        std::int32_t offset;
        std::uint32_t rng;
    };

    static Ret process(const std::vector<int>& acs, std::size_t blockSize = 63);
    static std::vector<int> processBack(const std::vector<std::int32_t>& acsProcessed,
                                        const std::vector<std::int32_t>& lengths,
                                        std::int32_t offset); 
};

#endif // PROCESS_HPP
