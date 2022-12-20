#include <vector>
#include <cstring>
#include <cstdio>

#include "nanojpeg.h"

///////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION SECTION                                                    //
// you may stop reading here                                                 //
///////////////////////////////////////////////////////////////////////////////

unsigned char njClip(const int x) {
    return (x < 0) ? 0 : ((x > 0xFF) ? 0xFF : (unsigned char) x);
}

void njColIDCT(const int* blk, unsigned char *out, int stride)
{
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;
    if (!((x1 = blk[8*4] << 8)
        | (x2 = blk[8*6])
        | (x3 = blk[8*2])
        | (x4 = blk[8*1])
        | (x5 = blk[8*7])
        | (x6 = blk[8*5])
        | (x7 = blk[8*3]))) {
        x1 = njClip(((blk[0] + 32) >> 6) + 128);
        for (x0 = 8;  x0;  --x0) {
            *out = (unsigned char) x1;
            out += stride;
        }
        return;
    }
    x0 = (blk[0] << 8) + 8192;
    x8 = W7 * (x4 + x5) + 4;
    x4 = (x8 + (W1 - W7) * x4) >> 3;
    x5 = (x8 - (W1 + W7) * x5) >> 3;
    x8 = W3 * (x6 + x7) + 4;
    x6 = (x8 - (W3 - W5) * x6) >> 3;
    x7 = (x8 - (W3 + W5) * x7) >> 3;
    x8 = x0 + x1;
    x0 -= x1;
    x1 = W6 * (x3 + x2) + 4;
    x2 = (x1 - (W2 + W6) * x2) >> 3;
    x3 = (x1 + (W2 - W6) * x3) >> 3;
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
    *out = njClip(((x7 + x1) >> 14) + 128);  out += stride;
    *out = njClip(((x3 + x2) >> 14) + 128);  out += stride;
    *out = njClip(((x0 + x4) >> 14) + 128);  out += stride;
    *out = njClip(((x8 + x6) >> 14) + 128);  out += stride;
    *out = njClip(((x8 - x6) >> 14) + 128);  out += stride;
    *out = njClip(((x0 - x4) >> 14) + 128);  out += stride;
    *out = njClip(((x3 - x2) >> 14) + 128);  out += stride;
    *out = njClip(((x7 - x1) >> 14) + 128);
}

unsigned short njDecode16(const unsigned char *pos) {
    return (pos[0] << 8) | pos[1];
}

#define CF4A (-9)
#define CF4B (111)
#define CF4C (29)
#define CF4D (-3)
#define CF3A (28)
#define CF3B (109)
#define CF3C (-9)
#define CF3X (104)
#define CF3Y (27)
#define CF3Z (-3)
#define CF2A (139)
#define CF2B (-11)
#define CF(x) njClip(((x) + 64) >> 7)

//----------------------------------------------------------------------------//
void njUpsampleH(nj_component_t* c) {
    const int xmax = c->width - 3;
    unsigned char *lin, *lout;
    int x, y;
    auto out = std::vector<unsigned char>((c->width * c->height) << 1);
    lin = c->pixels.data();
    lout = out.data();
    for (y = c->height;  y;  --y) {
        lout[0] = CF(CF2A * lin[0] + CF2B * lin[1]);
        lout[1] = CF(CF3X * lin[0] + CF3Y * lin[1] + CF3Z * lin[2]);
        lout[2] = CF(CF3A * lin[0] + CF3B * lin[1] + CF3C * lin[2]);
        for (x = 0;  x < xmax;  ++x) {
            lout[(x << 1) + 3] = CF(CF4A * lin[x] + CF4B * lin[x + 1] + CF4C * lin[x + 2] + CF4D * lin[x + 3]);
            lout[(x << 1) + 4] = CF(CF4D * lin[x] + CF4C * lin[x + 1] + CF4B * lin[x + 2] + CF4A * lin[x + 3]);
        }
        lin += c->stride;
        lout += c->width << 1;
        lout[-3] = CF(CF3A * lin[-1] + CF3B * lin[-2] + CF3C * lin[-3]);
        lout[-2] = CF(CF3X * lin[-1] + CF3Y * lin[-2] + CF3Z * lin[-3]);
        lout[-1] = CF(CF2A * lin[-1] + CF2B * lin[-2]);
    }
    c->width <<= 1;
    c->stride = c->width;
    c->pixels = out;
}

//----------------------------------------------------------------------------//
void njUpsampleV(nj_component_t* c) {
    const int w = c->width, s1 = c->stride, s2 = s1 + s1;
    unsigned char *cin, *cout;
    int x, y;
    auto out = std::vector<unsigned char>((c->width * c->height) << 1);
    for (x = 0;  x < w;  ++x) {
        cin = &c->pixels[x];
        cout = &out[x];
        *cout = CF(CF2A * cin[0] + CF2B * cin[s1]);  cout += w;
        *cout = CF(CF3X * cin[0] + CF3Y * cin[s1] + CF3Z * cin[s2]);  cout += w;
        *cout = CF(CF3A * cin[0] + CF3B * cin[s1] + CF3C * cin[s2]);  cout += w;
        cin += s1;
        for (y = c->height - 3;  y;  --y) {
            *cout = CF(CF4A * cin[-s1] + CF4B * cin[0] + CF4C * cin[s1] + CF4D * cin[s2]);  cout += w;
            *cout = CF(CF4D * cin[-s1] + CF4C * cin[0] + CF4B * cin[s1] + CF4A * cin[s2]);  cout += w;
            cin += s1;
        }
        cin += s1;
        *cout = CF(CF3A * cin[0] + CF3B * cin[-s1] + CF3C * cin[-s2]);  cout += w;
        *cout = CF(CF3X * cin[0] + CF3Y * cin[-s1] + CF3Z * cin[-s2]);  cout += w;
        *cout = CF(CF2A * cin[0] + CF2B * cin[-s1]);
    }
    c->height <<= 1;
    c->stride = c->width;
    c->pixels = out;
}

//----------------------------------------------------------------------------//
void njUpsample(nj_component_t* c) {
    int x, y, xshift = 0, yshift = 0;

    unsigned char *lin, *lout;
    while (c->width < nj.width) { c->width <<= 1; ++xshift; }
    while (c->height < nj.height) { c->height <<= 1; ++yshift; }
    auto out = std::vector<unsigned char>(c->width * c->height);
    lout = out.data();
    for (y = 0;  y < c->height;  ++y) {
        for (x = 0;  x < c->width;  ++x)
            lout[x] = lin[x >> xshift];
        lout += c->width;
    }
    c->stride = c->width;
    c->pixels = out;
}

////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
void nj_context_t::skipBits(int bits) {
    if (bufbits < bits) {
        showBits(bits);
    }
    bufbits -= bits;
}

//----------------------------------------------------------------------------//
int nj_context_t::showBits(int bits) {
    unsigned char newbyte;
    if (!bits) return 0;
    while (bufbits < bits) {
        if (size <= 0) {
            buf = (buf << 8) | 0xFF;
            bufbits += 8;
            continue;
        }
        newbyte = *pos++;
        size--;
        bufbits += 8;
        buf = (buf << 8) | newbyte;
        if (newbyte == 0xFF) {
            if (size) {
                unsigned char marker = *pos++;
                size--;
                switch (marker) {
                    case 0x00:
                    case 0xFF:
                        break;
                    case 0xD9: size = 0; break;
                    default:
                        if ((marker & 0xF8) != 0xD0)
                            error = NJ_SYNTAX_ERROR;
                        else {
                            buf = (buf << 8) | marker;
                            bufbits += 8;
                        }
                }
            } else
                error = NJ_SYNTAX_ERROR;
        }
    }
    return (buf >> (bufbits - bits)) & ((1 << bits) - 1);
}

//----------------------------------------------------------------------------//
int nj_context_t::getBits(int bits) {
    int res = showBits(bits);
    skipBits(bits);
    return res;
}

//----------------------------------------------------------------------------//
void nj_context_t::skip(int count) {
    pos += count;
    size -= count;
    length -= count;
    if (size < 0) error = NJ_SYNTAX_ERROR;
}

//----------------------------------------------------------------------------//
void nj_context_t::decodeLength() {
    if (size < 2) njThrowInt(NJ_SYNTAX_ERROR);
    length = njDecode16(pos);
    if (length > size) njThrowInt(NJ_SYNTAX_ERROR);
    skip(2);
}

//----------------------------------------------------------------------------//
void nj_context_t::skipMarker() {
    decodeLength();
    skip(length);
}

//----------------------------------------------------------------------------//
void nj_context_t::decodeDRI() {
    decodeLength();
    njCheckErrorInt();
    if (length < 2) njThrowInt(NJ_SYNTAX_ERROR);
    rstinterval = njDecode16(pos);
    skip(length);
}
