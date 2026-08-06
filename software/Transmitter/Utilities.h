#pragma once

// ----------------------------------------------------------------------------------------

// Degrees to radians
constexpr float deg2rad = 3.14159265358979323846f/180.0f;

// An Arduino style map function supporting float
// If fromLow == fromHigh (degenerate/zero-width input range), returns toLow rather than
// dividing by zero - callers passing a fixed input range should ensure it's non-degenerate.
inline float mapf(float x, float fromLow, float fromHigh, float toLow, float toHigh)
{
  if (fromHigh == fromLow)
    return toLow;

  return (x - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}