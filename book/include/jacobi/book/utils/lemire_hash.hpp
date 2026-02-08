// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file
 *
 * Provides routines to work with hash map that is used by jacobi
 * components to store in unordered mapping.
 */
#pragma once

#include <cstring>
#include <cstdint>

namespace jacobi::book::utils
{

//
// lemire_64bit_hash_t
//

/**
 * @brief Efficient implementation of hash function:
 *
 * @see
 * https://lemire.me/blog/2018/08/15/fast-strongly-universal-64-bit-hashing-everywhere/
 */
struct lemire_64bit_hash_t
{
    static std::uint64_t hash32_1( std::uint64_t x ) noexcept
    {
        constexpr std::uint64_t a = 0x65d200ce55b19ad8ull;
        constexpr std::uint64_t b = 0x4f2162926e40c299ull;
        constexpr std::uint64_t c = 0x162dd799029970f8ull;

        const auto low  = static_cast< std::uint32_t >( x );
        const auto high = static_cast< std::uint32_t >( x >> 32 );
        return ( a * low + b * high + c ) >> 32;
    }

    static std::uint64_t hash32_2( std::uint64_t x ) noexcept
    {
        constexpr std::uint64_t a = 0x68b665e6872bd1f4ull;
        constexpr std::uint64_t b = 0xb6cfcf9d79b51db2ull;
        constexpr std::uint64_t c = 0x7a2b92ae912898c2ull;

        const auto low  = static_cast< std::uint32_t >( x );
        const auto high = static_cast< std::uint32_t >( x >> 32 );
        return ( a * low + b * high + c ) >> 32;
    }

    auto operator()( std::uint64_t x ) const noexcept
    {
        return hash32_1( x ) | ( hash32_2( x ) << 32 );
    }

    auto operator()( std::int64_t x ) const noexcept
    {
        std::uint64_t xx;
        std::memcpy( &xx, &x, sizeof( xx ) );
        return hash32_1( xx ) | ( hash32_2( xx ) << 32 );
    }
};

}  // namespace jacobi::book::utils
