// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

// clang-format off
#pragma once

#define JACOBI_VERSION_CODE( major, minor, patch ) \
    ( ( ( major ) << 16UL ) + ( ( minor ) << 8UL ) + ( ( patch ) << 0UL ))

#define JACOBI_VERSION_MAJOR 0ull
#define JACOBI_VERSION_MINOR 1ull
#define JACOBI_VERSION_PATCH 0ull

// Consider to insert real revision here on CI:
#define JACOBI_VCS_REVISION "n/a"

#define JACOBI_VERSION \
    JACOBI_VERSION_CODE( JACOBI_VERSION_MAJOR, JACOBI_VERSION_MINOR, JACOBI_VERSION_PATCH )

