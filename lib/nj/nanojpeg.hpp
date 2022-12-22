#ifndef _NANOJPEG_HPP
#define _NANOJPEG_HPP

#include <iterator>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstring>

#include "exceptions.hpp"

#ifndef NJ_CHROMA_FILTER
    #define NJ_CHROMA_FILTER 1
#endif

static const char njZZ[64] = { 0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18,
11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35,
42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45,
38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63 };

struct nj_vlc_code_t {
    unsigned char bits, code;
};

typedef struct _nj_cmp {
    int cid;
    int ssx, ssy;
    int width, height;
    int stride;
    int qtsel;
    int actabsel, dctabsel;
    int dcpred;
    std::vector<unsigned char> pixels;
} nj_component_t;

struct nj_context_t {
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    bool isColor() const { return (ncomp != 1); }
    unsigned char* getImage() { return (ncomp == 1) ? comp[0].pixels.data() : rgb.data(); }
    int getImageSize() const { return width * height * ncomp; }
    void byteAlign() { bufbits &= 0xF8; }
    void skipBits(int bits);
    int showBits(int bits);
    int getBits(int bits);
    void skip(int count);
    void decodeLength();
    void skipMarker();
    void decodeDRI();
    unsigned char clip(const int x);
    void colIDCT(const int* blk, unsigned char *out, int stride);
    unsigned short decode16(const unsigned char *pos);
    void upsampleH(nj_component_t* c);
    void upsampleV(nj_component_t* c);
    void upsample(nj_component_t* c);
    bool finished = false;

    const unsigned char *pos = nullptr;
    int size;
    int length;
    int width, height;
    int mbwidth, mbheight;
    int mbsizex, mbsizey;
    int ncomp;
    nj_component_t comp[3];
    int qtused, qtavail;
    unsigned char qtab[4][64];
    nj_vlc_code_t vlctab[4][65536];
    int buf, bufbits;
    int block[64];
    int rstinterval;
    std::vector<unsigned char> rgb;
};

static nj_context_t nj;

// njDecode: Decode a JPEG image.
// Decodes a memory dump of a JPEG file into internal buffers.
// Parameters:
//   jpeg = The pointer to the memory dump.
//   size = The size of the JPEG file.
template <std::output_iterator<int> OutIter>
void njDecode(OutIter& out, const void* jpeg, const int size);

constexpr static int W1 = 2841;
constexpr static int W2 = 2676;
constexpr static int W3 = 2408;
constexpr static int W5 = 1609;
constexpr static int W6 = 1108;
constexpr static int W7 = 565;

static inline void njRowIDCT(int* blk) {
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;
    if (!((x1 = blk[4] << 11)
        | (x2 = blk[6])
        | (x3 = blk[2])
        | (x4 = blk[1])
        | (x5 = blk[7])
        | (x6 = blk[5])
        | (x7 = blk[3])))
    {
        blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = blk[0] << 3;
        return;
    }
    x0 = (blk[0] << 11) + 128;
    x8 = W7 * (x4 + x5);
    x4 = x8 + (W1 - W7) * x4;
    x5 = x8 - (W1 + W7) * x5;
    x8 = W3 * (x6 + x7);
    x6 = x8 - (W3 - W5) * x6;
    x7 = x8 - (W3 + W5) * x7;
    x8 = x0 + x1;
    x0 -= x1;
    x1 = W6 * (x3 + x2);
    x2 = x1 - (W2 + W6) * x2;
    x3 = x1 + (W2 - W6) * x3;
    x1 = x4 + x6;
    x4 -= x6;
    x6 = x5 + x7;
    x5 -= x7;
    x7 = x8 + x3;
    x8 -= x3;
    x3 = x0 + x2;
    x0 -= x2;
    x2 = (181 * (x4 + x5) + 128) >> 8;
    x4 = (181 * (x4 - x5) + 128) >> 8;
    blk[0] = (x7 + x1) >> 8;
    blk[1] = (x3 + x2) >> 8;
    blk[2] = (x0 + x4) >> 8;
    blk[3] = (x8 + x6) >> 8;
    blk[4] = (x8 - x6) >> 8;
    blk[5] = (x0 - x4) >> 8;
    blk[6] = (x3 - x2) >> 8;
    blk[7] = (x7 - x1) >> 8;
}

static inline void njDecodeSOF(void) {
    int i, ssxmax = 0, ssymax = 0;
    nj_component_t* c;
    nj.decodeLength();
    if (nj.length < 9) {
        throw SyntaxErrorException();
    }
    if (nj.pos[0] != 8) {
        throw UnsupportedException();
    }
    nj.height = nj.decode16(nj.pos+1);
    nj.width = nj.decode16(nj.pos+3);
    if (!nj.width || !nj.height) {
        throw SyntaxErrorException();
    }
    nj.ncomp = nj.pos[5];
    nj.skip(6);
    switch (nj.ncomp) {
        case 1:
        case 3:
            break;
        default:
            throw UnsupportedException();
    }
    if (nj.length < (nj.ncomp * 3)) {
        throw SyntaxErrorException();
    }
    for (i = 0, c = nj.comp;  i < nj.ncomp;  ++i, ++c) {
        c->cid = nj.pos[0];
        if (!(c->ssx = nj.pos[1] >> 4)) throw SyntaxErrorException();
        if (c->ssx & (c->ssx - 1)) throw UnsupportedException();  // non-power of two
        if (!(c->ssy = nj.pos[1] & 15)) throw SyntaxErrorException();
        if (c->ssy & (c->ssy - 1)) throw UnsupportedException();  // non-power of two
        if ((c->qtsel = nj.pos[2]) & 0xFC) throw SyntaxErrorException();
        nj.skip(3);
        nj.qtused |= 1 << c->qtsel;
        if (c->ssx > ssxmax) ssxmax = c->ssx;
        if (c->ssy > ssymax) ssymax = c->ssy;
    }
    if (nj.ncomp == 1) {
        c = nj.comp;
        c->ssx = c->ssy = ssxmax = ssymax = 1;
    }
    nj.mbsizex = ssxmax << 3;
    nj.mbsizey = ssymax << 3;
    nj.mbwidth = (nj.width + nj.mbsizex - 1) / nj.mbsizex;
    nj.mbheight = (nj.height + nj.mbsizey - 1) / nj.mbsizey;
    for (i = 0, c = nj.comp;  i < nj.ncomp;  ++i, ++c) {
        c->width = (nj.width * c->ssx + ssxmax - 1) / ssxmax;
        c->height = (nj.height * c->ssy + ssymax - 1) / ssymax;
        c->stride = nj.mbwidth * c->ssx << 3;
        if (((c->width < 3) && (c->ssx != ssxmax)) || ((c->height < 3) && (c->ssy != ssymax))) {
            throw UnsupportedException();
        }
        c->pixels = std::vector<unsigned char>(c->stride * nj.mbheight * c->ssy << 3);
    }
    if (nj.ncomp == 3) {
        nj.rgb = std::vector<unsigned char>(nj.width * nj.height * nj.ncomp);
    }
    nj.skip(nj.length);
}

static inline void njDecodeDHT(void) {
    int codelen, currcnt, remain, spread, i, j;
    nj_vlc_code_t *vlc;
    static unsigned char counts[16];
    nj.decodeLength();
    while (nj.length >= 17) {
        i = nj.pos[0];
        if (i & 0xEC)  {
            throw SyntaxErrorException();
        }
        if (i & 0x02) {
            throw UnsupportedException();
        }
        i = (i | (i >> 3)) & 3;  // combined DC/AC + tableid value
        for (codelen = 1;  codelen <= 16;  ++codelen)
            counts[codelen - 1] = nj.pos[codelen];
        nj.skip(17);
        vlc = &nj.vlctab[i][0];
        remain = spread = 65536;
        for (codelen = 1;  codelen <= 16;  ++codelen) {
            spread >>= 1;
            currcnt = counts[codelen - 1];
            if (!currcnt) continue;
            if (nj.length < currcnt)  {
                throw SyntaxErrorException();
            }
            remain -= currcnt << (16 - codelen);
            if (remain < 0)  {
                throw SyntaxErrorException();
            }
            for (i = 0;  i < currcnt;  ++i) {
                unsigned char code = nj.pos[i];
                for (j = spread;  j;  --j) {
                    vlc->bits = (unsigned char) codelen;
                    vlc->code = code;
                    ++vlc;
                }
            }
            nj.skip(currcnt);
        }
        while (remain--) {
            vlc->bits = 0;
            ++vlc;
        }
    }
    if (nj.length)  {
        throw SyntaxErrorException();
    }
}

static inline void njDecodeDQT(void) {
    int i;
    unsigned char *t;
    nj.decodeLength();
    while (nj.length >= 65) {
        i = nj.pos[0];
        if (i & 0xFC)  {
            throw SyntaxErrorException();
        }
        nj.qtavail |= 1 << i;
        t = &nj.qtab[i][0];
        for (i = 0;  i < 64;  ++i)
            t[i] = nj.pos[i + 1];
        nj.skip(65);
    }
    if (nj.length)  {
        throw SyntaxErrorException();
    }
}

static int njGetVLC(nj_vlc_code_t* vlc, unsigned char* code) {
    int value = nj.showBits(16);
    int bits = vlc[value].bits;
    if (!bits)  {
        throw SyntaxErrorException();
    }
    nj.skipBits(bits);
    value = vlc[value].code;
    if (code) {
        *code = (unsigned char) value;
    }
    bits = value & 15;
    if (!bits) {
        return 0;
    }
    value = nj.getBits(bits);
    if (value < (1 << (bits - 1))) {
        value += ((-1) << bits) + 1;
    }
    return value;
}

template <std::output_iterator<int> OutIterT>
static inline void
njDecodeBlock(OutIterT& outBlocks, nj_component_t* c, unsigned char* out)
{
    unsigned char code = 0;
    int coef = 0;
    std::memset(nj.block, 0, sizeof(nj.block));
    c->dcpred += njGetVLC(&nj.vlctab[c->dctabsel][0], NULL);

    nj.block[0] = c->dcpred;

    do {
        int value = njGetVLC(&nj.vlctab[c->actabsel][0], &code);

        if (!code) break;  // EOB
        if (!(code & 0x0F) && (code != 0xF0))  {
            throw SyntaxErrorException();
        }
        coef += (code >> 4) + 1;

        if (coef > 63)  {
            throw SyntaxErrorException();
        }

        nj.block[(int) njZZ[coef]] = value;
    } while (coef < 63);

    for(unsigned int i = 0; i < 64; ++i, ++outBlocks) {
        *outBlocks = nj.block[i];
    }

    for (coef = 0; coef < 64; coef += 8) {
        njRowIDCT(&nj.block[coef]);
    }

    for (coef = 0; coef < 8; ++coef) {
        nj.colIDCT(&nj.block[coef], &out[coef], c->stride);
    }
}

template <std::output_iterator<int> OutIterT>
static inline void njDecodeScan(OutIterT& outBlocksIter) {
    int i, mbx, mby, sbx, sby;
    int rstcount = nj.rstinterval, nextrst = 0;
    nj_component_t* c;
    nj.decodeLength();
    if (nj.length < (4 + 2 * nj.ncomp))  {
        throw SyntaxErrorException();
    }
    if (nj.pos[0] != nj.ncomp) {
        throw UnsupportedException();
    }
    nj.skip(1);
    for (i = 0, c = nj.comp;  i < nj.ncomp;  ++i, ++c) {
        if (nj.pos[0] != c->cid
                || nj.pos[1] & 0xEE) {
            throw SyntaxErrorException();
        }
        c->dctabsel = nj.pos[1] >> 4;
        c->actabsel = (nj.pos[1] & 1) | 2;
        nj.skip(2);
    }
    if (nj.pos[0] || (nj.pos[1] != 63) || nj.pos[2]) {
        throw UnsupportedException();
    }
    nj.skip(nj.length);
    for (mbx = mby = 0;;) {
        for (i = 0, c = nj.comp;  i < nj.ncomp;  ++i, ++c)
            for (sby = 0;  sby < c->ssy;  ++sby)
                for (sbx = 0;  sbx < c->ssx;  ++sbx) {
                    auto* out = &c->pixels[((mby * c->ssy + sby) * c->stride + mbx * c->ssx + sbx) << 3];
                    njDecodeBlock(outBlocksIter,c, out);
                }
        if (++mbx >= nj.mbwidth) {
            mbx = 0;
            if (++mby >= nj.mbheight) {
                break;
            }
        }
        if (nj.rstinterval && !(--rstcount)) {
            nj.byteAlign();
            i = nj.getBits(16);
            if (((i & 0xFFF8) != 0xFFD0) || ((i & 7) != nextrst))  {
                throw SyntaxErrorException();
            }
            nextrst = (nextrst + 1) & 7;
            rstcount = nj.rstinterval;
            for (i = 0;  i < 3;  ++i) {
                nj.comp[i].dcpred = 0;
            }
        }
    }
    nj.finished  = true;
}

static inline void njConvert(void) {
    int i;
    nj_component_t* c;
    for (i = 0, c = nj.comp;  i < nj.ncomp;  ++i, ++c) {
        #if NJ_CHROMA_FILTER
            while ((c->width < nj.width) || (c->height < nj.height)) {
                if (c->width < nj.width) nj.upsampleH(c);
                if (c->height < nj.height) nj.upsampleV(c);
            }
        #else
            if ((c->width < nj.width) || (c->height < nj.height))
                nj.upsample(c);
        #endif
        assert(!((c->width < nj.width) || (c->height < nj.height)));
    }
    if (nj.ncomp == 3) {
        // convert to RGB
        int x, yy;
        unsigned char *prgb = nj.rgb.data();
        const unsigned char *py  = nj.comp[0].pixels.data();
        const unsigned char *pcb = nj.comp[1].pixels.data();
        const unsigned char *pcr = nj.comp[2].pixels.data();
        for (yy = nj.height;  yy;  --yy) {
            for (x = 0;  x < nj.width;  ++x) {
                int y = py[x] << 8;
                int cb = pcb[x] - 128;
                int cr = pcr[x] - 128;
                *prgb++ = nj.clip((y            + 359 * cr + 128) >> 8);
                *prgb++ = nj.clip((y -  88 * cb - 183 * cr + 128) >> 8);
                *prgb++ = nj.clip((y + 454 * cb            + 128) >> 8);
            }
            py += nj.comp[0].stride;
            pcb += nj.comp[1].stride;
            pcr += nj.comp[2].stride;
        }
    } else if (nj.comp[0].width != nj.comp[0].stride) {
        // grayscale -> only remove stride
        unsigned char *pin = &nj.comp[0].pixels[nj.comp[0].stride];
        unsigned char *pout = &nj.comp[0].pixels[nj.comp[0].width];
        int y;
        for (y = nj.comp[0].height - 1;  y;  --y) {
            std::memcpy(pout, pin, nj.comp[0].width);
            pin += nj.comp[0].stride;
            pout += nj.comp[0].width;
        }
        nj.comp[0].stride = nj.comp[0].width;
    }
}

template <std::output_iterator<int> OutIterT>
void njDecode(OutIterT& out, const void* jpeg, const int size) {
    nj.pos = static_cast<const unsigned char*>(jpeg);
    nj.size = size & 0x7FFFFFFF;
    if (nj.size < 2
            || ((nj.pos[0] ^ 0xFF) | (nj.pos[1] ^ 0xD8))) {
        throw NotJpegException();
    }
    nj.skip(2);
    while (!nj.finished) {
        if ((nj.size < 2) || (nj.pos[0] != 0xFF))  {
            throw SyntaxErrorException();
        }
        nj.skip(2);
        switch (nj.pos[-1]) {
            case 0xC0: njDecodeSOF(); break;
            case 0xC4: njDecodeDHT(); break;
            case 0xDB: njDecodeDQT(); break;
            case 0xDD: nj.decodeDRI(); break;
            case 0xDA: njDecodeScan(out); break;
            case 0xFE: nj.skipMarker(); break;
            default:
                if ((nj.pos[-1] & 0xF0) == 0xE0) {
                    nj.skipMarker();
                } else {
                    throw UnsupportedException();
                }
        }
    }
    if (!nj.finished) {
        throw std::runtime_error("Tot finished.");
    }
    njConvert();
}

static void njDecodeHeader(const void* jpegHeader, const int size) {
    nj.pos = static_cast<const unsigned char*>(jpegHeader);
    nj.size = size & 0x7FFFFFFF;
    if (nj.size < 2
            || ((nj.pos[0] ^ 0xFF) | (nj.pos[1] ^ 0xD8))) {
        throw NotJpegException();
    }
    nj.skip(2);
    while (!nj.finished) {
        if (nj.size == 0) { break; }
        nj.skip(2);
        switch (nj.pos[-1]) {
            case 0xC0: njDecodeSOF(); break;
            case 0xC4: njDecodeDHT(); break;
            case 0xDB: njDecodeDQT(); break;
            case 0xDD: nj.decodeDRI(); break;
            case 0xFE: nj.skipMarker(); break;
            default:
                if ((nj.pos[-1] & 0xF0) == 0xE0) {
                    nj.skipMarker();
                } else {
                    throw UnsupportedException();
                }
        }
    }
    nj.finished = true;
}

#endif//_NANOJPEG_HPP


