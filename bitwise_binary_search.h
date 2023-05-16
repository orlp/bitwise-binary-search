/*
    See https://orlp.net/blog/bitwise-binary-search.

    Copyright (c) 2023 Orson Peters

    This software is provided 'as-is', without any express or implied warranty. In
    no event will the authors be held liable for any damages arising from the use of
    this software.

    Permission is granted to anyone to use this software for any purpose, including
    commercial applications, and to alter it and redistribute it freely, subject to
    the following restrictions:

    1. The origin of this software must not be misrepresented; you must not claim
        that you wrote the original software. If you use this software in a product,
        an acknowledgment in the product documentation would be appreciated but is
        not required.

    2. Altered source versions must be plainly marked as such, and must not be
        misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include <cstddef>
#include <bit>


// More efficient shim for std::bit_floor.
inline size_t std_bit_floor(size_t n) {
    if (n == 0) return 0;
    return size_t(1) << (std::bit_width(n) - 1);
}


template<typename It, typename T, typename Cmp>
It lower_bound_pad(It begin, It end, const T& value, Cmp comp) {
    size_t n = end - begin;
    size_t b = -1;
    for (size_t bit = std_bit_floor(n); bit != 0; bit >>= 1) {
        if (b + bit < n && comp(begin[b + bit], value)) b += bit;
    }
    return begin + (b + 1);
}


template<typename It, typename T, typename Cmp>
It lower_bound_overlap(It begin, It end, const T& value, Cmp comp) {
    size_t n = end - begin;
    if (n == 0) return begin;

    size_t two_k = std_bit_floor(n);
    if (comp(begin[n / 2], value)) begin = end - (two_k - 1);
    
    size_t b = -1;
    for (size_t bit = two_k >> 1; bit != 0; bit >>= 1) {
        if (comp(begin[b + bit], value)) b += bit;
    }
    return begin + (b + 1);
}


template<typename It, typename T, typename Cmp>
It lower_bound_opt(It begin, It end, const T& value, Cmp comp) {
    size_t n = end - begin;
    if (n == 0) return begin;

    size_t two_r = std_bit_floor(n);
    size_t two_l = two_r - ((two_r >> 1) & ~n);
    bool use_r = comp(begin[two_l - 1], value);
    size_t two_k = use_r ? two_r : two_l;
    begin = use_r ? end - (two_r - 1) : begin;

    size_t b = -1;
    for (size_t bit = two_k >> 1; bit != 0; bit >>= 1) {
        if (comp(begin[b + bit], value)) b += bit;
    }
    return begin + (b + 1);
}


