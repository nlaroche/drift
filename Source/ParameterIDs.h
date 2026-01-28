#pragma once

namespace ParameterIDs
{
    // Grain Parameters
    inline constexpr const char* grainSize     = "grainSize";      // 10-500ms
    inline constexpr const char* grainDensity  = "grainDensity";   // 1-32 grains
    inline constexpr const char* grainSpread   = "grainSpread";    // 0-100% position randomization

    // Pitch Parameters
    inline constexpr const char* pitch         = "pitch";          // -24 to +24 semitones
    inline constexpr const char* pitchScatter  = "pitchScatter";   // 0-100% random pitch variation
    inline constexpr const char* shimmer       = "shimmer";        // 0-100% octave up feedback

    // Time Parameters
    inline constexpr const char* stretch       = "stretch";        // 0.25x - 4x time stretch
    inline constexpr const char* reverse       = "reverse";        // 0-100% reverse grain probability
    inline constexpr const char* freeze        = "freeze";         // Toggle infinite sustain

    // Texture Parameters
    inline constexpr const char* blur          = "blur";           // 0-100% spectral blur
    inline constexpr const char* warmth        = "warmth";         // 0-100% low-pass filtering
    inline constexpr const char* sparkle       = "sparkle";        // 0-100% high freq enhancement

    // Output Parameters
    inline constexpr const char* feedback      = "feedback";       // 0-100% grain feedback
    inline constexpr const char* mix           = "mix";            // 0-100% dry/wet
    inline constexpr const char* output        = "output";         // -24 to +12 dB
    inline constexpr const char* bypass        = "bypass";         // Toggle
}
