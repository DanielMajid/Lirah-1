/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 *  File: header.c
 *
 *  NTS-1 mkII oscillator unit header definition
 *
 */

#include "unit_osc.h"   // Note: Include base definitions for osc units

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),                  // Size of this header. Leave as is.
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,    // Target platform and module pair for this unit
    .api = UNIT_API_VERSION,                               // API version for which unit was built. See runtime.h
    .dev_id = 0x4D616A69U,                                        // Developer ID. See https://github.com/korginc/logue-sdk/blob/master/developer_ids.md
    .unit_id = 0x3U,                                       // ID for this unit. Scoped within the context of a given dev_id.
    .version = 0x00010000U,                                // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "Lirah-1",                                     // Name for this unit, will be displayed on device
    .num_params = 8,                                       // Number of valid parameter descriptors. (max. 10)
    .params = {
        // Format:
        // min, max, center (unused), default, type, frac. bits, frac. mode, <reserved>, name

        // See common/runtime.h for type enum and unit_param_t structure

        // ---- Fixed/direct UI parameters (shown on A/B knobs) ----------------

        // Index 0 — SHAPE knob (A): FM depth.
        // Raw 0-1023 value; multiplied by 10 in DSP to scale FM depth (0..10230).
        // Higher values = more timbral complexity and inharmonic FM sidebands.
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"FM DEPTH"}},

        // Index 1 — ALT knob (B): Hyper LFO depth.
        // Controls the pitch-jump amplitude when the AND-gated LFO gate opens.
        // 0 = no pitch effect; 1023 = maximum pitch jump (up to +3 semitone-multiples).
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"HYPER LFO"}},

        // ---- Edit menu parameters (indices 2-9) -----------------------------

        // Index 2 — LFO rate 1. Integer 0-100 → 0-10 Hz.
        // Rate of the first hyper LFO sine oscillator.
        {0, 100, 0, 10, k_unit_param_type_none, 0, 0, 0, {"LFO1 RATE"}},

        // Index 3 — LFO rate 2. Integer 0-100 → 0-10 Hz.
        // Rate of the second hyper LFO sine oscillator. Interact with LFO1 to gate.
        {0, 100, 0, 20, k_unit_param_type_none, 0, 0, 0, {"LFO2 RATE"}},

        // Index 4 — Wave fold depth. Integer 0-100 → 0-10 amplitude scalar.
        // Scales the carrier sine before the triangle wave folder; adds harmonics.
        {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"FOLD"}},

        // Index 5 — FM tune (modulator relative pitch). Integer 0-100 → 0 to +1 octave.
        // Shifts the modulator oscillator frequency above the carrier (linear ratio).
        {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"FM TUNE"}},

        // Index 6 — Oscillator tune (carrier pitch offset). Integer 0-100 → 0 to +1 octave.
        // Shifts the carrier oscillator frequency up, unquantized (linear ratio).
        {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"PITCH"}},

        // Index 7 — FM feedback. Integer 0-100 → 0-20 feedback scalar.
        // Feeds the previous carrier output back into the current sample's amplitude,
        // producing self-oscillating chaotic tones similar to the Lyra-8.
        {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"FEEDBACK"}},

        // Indices 8-9 — unused.
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}},
};
