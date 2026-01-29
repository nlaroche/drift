#pragma once

namespace ParameterIDs
{
    // DRIFT - Wandering Delay
    inline constexpr const char* time     = "time";     // 10 to 2000 ms
    inline constexpr const char* sync     = "sync";     // tempo sync on/off
    inline constexpr const char* division = "division"; // musical division (1/4, 1/8, etc.)
    inline constexpr const char* feedback = "feedback"; // 0 to 100%
    inline constexpr const char* duck     = "duck";     // 0 to 100% - ducking amount
    inline constexpr const char* taps     = "taps";     // 1 to 4 taps
    inline constexpr const char* spread   = "spread";   // 0 to 100% - stereo spread
    inline constexpr const char* mix      = "mix";      // 0 to 100%

    // Character controls (progressively applied per tap)
    inline constexpr const char* grit     = "grit";     // 0 to 100% - saturation in feedback
    inline constexpr const char* age      = "age";      // 0 to 100% - per-repeat HF rolloff
    inline constexpr const char* diffuse  = "diffuse";  // 0 to 100% - allpass diffusion
}
