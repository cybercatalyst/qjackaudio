/*
    Copyright (C) 2003 Fons Adriaensen <fons.adriaensen@skynet.be>
    Copyright (C) 2011 Jacob Dawid <jacob.dawid@cybercatalyst.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef PRBS_GENERATOR_H
#define	PRBS_GENERATOR_H

#include <assert.h>
#include <sys/types.h>
#include <stdint.h>


//---------------------------------------------------------------------------
//
// Class Synopsis: class Prbs_gen
//
// Description:
//   Pseudo random binary sequence generator using polynomial
//   division in GF (2).
//
// There are two ways to built such a generator. Both use some
// form of shift register.
//
// 1. The first type feeds back the parity (XOR) of the taps corresponding to
//    the non-zero elements of the polynomial into the input of the register.
//    This is the most efficient way to do it in hardware. 
//
// 2. In the seond form, when the bit shifted out is 1, the contents of the
//    register are XORed with a bit pattern representing the polynomial.
//    This is the best way to do it in software.
//
// Mutatis mutandis the two forms are equivalent. Any sequence that can be
// generated by one of the realisations can also be produced by the other.
// This software obviously uses the second form. It can use any polynomial
// up to (and including) a degree of 32.
//  
//
//  set_poly (p)
//
//    Defines the polynomial to be used. The value of p is found from the
//    sequence of coefficients (0 or 1) of the polynomial starting with the  
//    constant term, and dropping the highest one
//
//                                   0 1 2 3 4 5 6 7
//    Example: P = x^7 + x^6 + 1 --> 1 0 0 0 0 0 1 1 --> 1000001 --> 0x41
//
//    To emulate the first form described above, start with the highest
//    exponent and drop the constant term.
//
//                                   7 6 5 4 3 2 1 0
//    Example: P = x^7 + x^6 + 1 --> 1 1 0 0 0 0 0 1 --> 1100000 --> 0x60
//
//    Also sets the state to all ones.
//
//   
//  set_state (x)
//   
//    Sets the initial state to x.
// 
//
//  step ()
//
//     Returns the next pseudo random bit.
//
//
//  sync_forw (x)
// 
//    This sets the generator in a state as if the last N (= degree) bits 
//    were those defined by x (the LSB of x represents the oldest bit).
//    This can be used to synchronise a BER counter to a received bit stream,
//    or to set the initial state when emulating a generator of the first form
//    when the output is taken from the feedback.
//
//
//  sync_back (x)
//
//    This sets the generator in a state so that the first N (= degree) output
//    bits will be those defined by x (the LSB of x will be the first output bit).
//    This can be used to set the initial state when emulating a generator of
//    the first form when the output is taken from the shifted out bit.
//
//
//---------------------------------------------------------------------------==


class PRBSGenerator
{
public:

    enum
    {
        // Some polynomials for maximum length seqeunces.

        G7  = 0x00000041,
        G8  = 0x0000008E,
        G15 = 0x00004001,
        G16 = 0x00008016,
        G23 = 0x00400010,
        G24 = 0x0080000D,
        G31 = 0x40000004,
        G32 = 0x80000057
    };

    PRBSGenerator (void);

    void setPoly (uint32_t poly);
    void setStat (uint32_t stat);
    void sync_forw (uint32_t bits);
    void sync_back (uint32_t bits);
    int  step (void);
    void crc_in (int b);
    int  crc_out (void);

    uint32_t stat (void) const;
    uint32_t poly (void) const;
    uint32_t mask (void) const;
    uint32_t hbit (void) const;
    int degr (void) const;

    ~PRBSGenerator (void);

private:

    uint32_t _stat;
    uint32_t _poly;
    uint32_t _mask;
    uint32_t _hbit;
    int _degr;

};


inline PRBSGenerator::PRBSGenerator (void)
    : _stat (0), _poly (0), _mask (0), _degr (0)
{
}


inline PRBSGenerator::~PRBSGenerator (void)
{
}


inline void PRBSGenerator::setPoly (uint32_t poly)
{
    assert (poly != 0);

    _poly = poly;
    _mask = 0;
    _degr = 0;

    while (_mask < _poly)
    {
        _mask = (_mask << 1) | 1;
        _degr += 1;
    }
    _stat = _mask;
    _hbit = (_mask >> 1) + 1;
}


inline void PRBSGenerator::setStat (uint32_t stat)
{
    assert (_poly != 0);

    _stat = stat & _mask;

    assert (_stat != 0);
}


inline int PRBSGenerator::step (void)
{
    int bit;

    assert (_poly != 0);

    bit = _stat & 1;
    _stat >>= 1;
    if (bit) _stat ^= _poly;

    return bit;
}


inline void PRBSGenerator::sync_forw (uint32_t bits)
{
    assert (_poly != 0);

    for (int i = 0; i < _degr; i++)
    {
        _stat >>= 1;
        if (bits & 1) _stat ^= _poly;
        bits >>= 1;
    }
}


inline void PRBSGenerator::sync_back (uint32_t bits)
{
    assert (_poly != 0);

    _stat = 0;
    for (int h = _hbit; h; h >>= 1)
    {
        if (bits & h) _stat ^= _poly;
        _stat <<= 1;
    }
    _stat ^= bits;
    _stat &= _mask;
}


inline void PRBSGenerator::crc_in (int b)
{
    int bit;

    assert (_poly != 0);

    bit = (_stat & 1) ^ b;
    _stat >>= 1;
    if (bit) _stat ^= _poly;
}


inline int PRBSGenerator::crc_out (void)
{
    int bit;

    assert (_poly != 0);

    bit = (_stat & 1);
    _stat >>= 1;
    return bit;
}


inline uint32_t PRBSGenerator::stat (void) const { return _stat; }

inline uint32_t PRBSGenerator::poly (void) const { return _poly; }

inline uint32_t PRBSGenerator::mask (void) const { return _mask; }

inline uint32_t PRBSGenerator::hbit (void) const { return _hbit; }

inline int PRBSGenerator::degr (void) const { return _degr; }


#endif // PRBS_GENERATOR_H
