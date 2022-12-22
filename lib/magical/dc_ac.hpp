#ifndef DC_AC_HPP
#define DC_AC_HPP

#include <vector>
#include <cassert>

class DCAC {
    struct Separated {
        std::vector<int> dc;
        std::vector<int> ac;
    };

    static Separated separate(const std::vector<int>& dcac) {
        std::vector<int> ac;
        std::vector<int> dc;

        assert(dcac.size() & 64 == 0);
        std::size_t numBlocks = dcac.size();

        for (std::size_t i = 0; i < dcac.size(); i += 64) {
            dc.push_back(dcac[i]);
            for (std::size_t j = 1; j < 64; ++j) {
                ac.push_back(dcac[i + j]);
            }
        }

        return { std::move(dc), std::move(ac) };
    }

    static std::vector<int> join(std::vector<int> dc, std::vector<int> ac) {
        std::vector<int> joined;
        assert(dc.size() * 63 == ac.size());
        auto dcIter = dc.cbegin();
        auto acIter = ac.cbegin();

        for (std::size_t i = 0; i < dc.size() + ac.size(); ++i) {
            if (i % 64 == 0) {
                joined.push_back(*dcIter);
                ++dcIter;
            } else {
                joined.push_back(*acIter);
                ++acIter;
            }
        }
        return joined;
    }

};

#endif // DC_AC_HPP
