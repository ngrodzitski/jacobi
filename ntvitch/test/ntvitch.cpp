#include <jacobi/ntvitch/ntvitch.hpp>
#include <jacobi/ntvitch/ntvitch_fmt.hpp>

#include <gtest/gtest.h>

namespace jacobi::ntvitch
{

namespace /* anonymous */
{

// NOLINTNEXTLINE
TEST( JacobiNtvitch, SwapBytes )
{
    {
        const std::uint64_t x = 0x0123456789abcdef;
        const std::uint64_t y = JACOBI_NTVITCH_SWAP64( x );
        EXPECT_EQ( y, 0xefcdab8967452301ULL );
    }
    {
        const std::uint32_t x = 0x01234567U;
        const std::uint32_t y = JACOBI_NTVITCH_SWAP32( x );
        EXPECT_EQ( y, 0x67452301U );
    }
    {
        const std::uint16_t x = 0xAB12;
        const std::uint16_t y = JACOBI_NTVITCH_SWAP16( x );
        EXPECT_EQ( y, 0x12AB );
    }
}

}  // anonymous namespace

}  // namespace jacobi::ntvitch
