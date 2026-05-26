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

#include "unit_osc.h" // Base definitions for oscillator units

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),               // Header size expected by the runtime
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc, // Platform + module type
    .api = UNIT_API_VERSION,                            // Runtime API compatibility tag
    .dev_id = 0x4D616A69U,                              // Developer ID
    .unit_id = 0x3U,                                    // Unit ID within this developer ID
    .version = 0x00010010U,                             // Version: major.minor.patch
    .name = "Lirah-1",                                 // Display name on the hardware
    .num_params = 10,                                   // Number of active parameters (max 10 on NTS-1 mkII osc)
    .params = {
        // Format:
        // min, max, center (unused), default, type, frac. bits, frac. mode, <reserved>, name

        // See common/runtime.h for type enum and unit_param_t structure

        // Parameters 0 and 1 map to hardware knobs A and B.

        // Index 0 — SHAPE knob (A): FM depth.
        // Raw range is 0-1023. DSP scales this to FM depth.
        // Higher values add brighter and more inharmonic FM tone.
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"FM DEPTH"}},

        // Index 1 — ALT knob (B): Hyper LFO depth.
        // Controls how strong the Hyper LFO pitch jump feels.
        // 0 = no extra jump, 1023 = strongest jump.
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"HYPER LFO"}},

        // Parameters 2-9 are in the edit menu.

        // Hyper LFO lane 1 rate: 0-100 maps to 0-10 Hz.
        {0, 100, 0, 10, k_unit_param_type_none, 0, 0, 0, {"LFO1 RATE"}},

        // Hyper LFO lane 2 rate: 0-100 maps to 0-10 Hz.
        // This combines with lane 1 for the AND-gated Hyper behavior.
        {0, 100, 0, 20, k_unit_param_type_none, 0, 0, 0, {"LFO2 RATE"}},

        // Fold amount before the single-stage folder.
        {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"FOLD"}},

        // Relative pitch of the FM modulator (0 to +1 octave span).
        {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"FM TUNE"}},

        // Relative pitch offset of the carrier (0 to +1 octave span).
        {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"PITCH"}},

        // Feedback depth in the FM/carrier path.
        {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"FEEDBACK"}},

        // LFO 3 destination selector.
        // 0=OFF, 1=FMDEP, 2=HDEP, 3=HR1, 4=HR2, 5=FOLD, 6=FMTUN, 7=OTUN, 8=FDBK.
        {0, 8, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"LFO TARGET"}},

        // LFO 3 rate: piecewise map for finer slow control.
        // 0-50 -> 0-1 Hz, 50-100 -> 1-25 Hz.
        {0, 100, 0, 10, k_unit_param_type_none, 0, 0, 0, {"LFO RATE"}}},
};
