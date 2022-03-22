// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2020 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#include <mc1/fast_math.h>

float fast_sin(float x) {
  // 1) Reduce periods of sin(x) to the range -PI/2 to PI/2.
  //
  //       x  -5.0   -4.0   -3.0   -2.0   -1.0    0.0    1.0    2.0    3.0    4.0    5.0
  //  period  -2     -1     -1     -1      0      0      0      1      1      1      2
  //      x'   1.28  -0.86   0.14   1.14  -1.00   0.00   1.00  -1.14  -0.14   0.86  -1.28
  //
  // Note: The offset 512 is there to push most negative x:es into the positive range when doing
  // the float-to-int conversion, so that rounding is correct. 512 is selected because the
  // floating-point constant 512.5 fits in a single MRISC32 ldi instruction, and because the
  // floating-point addition will not throw away more precision than we get with the Tailor series
  // approximation.
  // TODO(m): Use FTOIR instead of adding 0.5?
  const int period = ((int)(x * (1.0f / 3.141592654f) + 512.5f)) - 512;
  x -= 3.141592654f * (float)period;

  // 2) Use a Tailor series approximation in the range -PI/2 to PI/2.
  // See: https://en.wikipedia.org/wiki/Taylor_series#Approximation_error_and_convergence
  // sin(x) ≃ x - x³/3! + x⁵/5! - x⁷/7!
  // Note: 3! = 6, 5! = 120, 7! = 5040
  const float x2 = x * x;
  const float x3 = x2 * x;
  const float x5 = x3 * x2;
  const float x7 = x5 * x2;
  const float y = x - (1.0f/6.0f) * x3 + (1.0f/120.0f) * x5 - (1.0f/5040.0f) * x7;

  // 3) Negate the result if this is an odd period.
  const int sign_bit = (period & 1) << 31;
  return bitcast_int_to_float(bitcast_float_to_int(y) ^ sign_bit);
}

