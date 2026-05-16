#pragma once
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
 *  File: osc.h
 *
 *  Lirah-1 — Lyra-8 style oscillator for NTS-1 mkII.
 *
 *  Ported from Lyre-1 (jamesdcheetham) originally written for the NTS-1 mki.
 *
 *  Signal path:
 *    - Two independent sine LFOs form a "hyper LFO": their output is
 *      AND-gated (positive half only) and used to pitch-modulate both
 *      the carrier and modulator oscillators.
 *    - A sine modulator oscillator provides FM input to the carrier.
 *    - The carrier sine is scaled by the wave-fold amount and fed back
 *      via a feedback path, then passed through a single-stage wave folder.
 *    - The hardware shape_lfo (knob A LFO) further amplitude-modulates
 *      the FM depth in real time.
 */

#include "processor.h"
#include "unit_osc.h"

class Osc : public Processor
{
public:
  // NTS-1 mkII oscillators do not support SDRAM allocation.
  uint32_t getBufferSize() const override final { return 0; }

  // ---- Parameter indices -------------------------------------------------------
  // Indices 0 and 1 correspond to the physical SHAPE (knob A) and ALT (knob B)
  // controls. Indices 2-7 are the edit-menu parameters shown on the device screen.
  enum
  {
    SHAPE     = 0U, // FM DEPTH   — knob A (0-1023)
    ALT       = 1U, // HYPER LFO  — knob B (0-1023)
    LFO_RATE1 = 2U, // LFO1 RATE  — edit menu (0-100 → 0-10 Hz)
    LFO_RATE2 = 3U, // LFO2 RATE  — edit menu (0-100 → 0-10 Hz)
    WAVE_FOLD = 4U, // FOLD       — edit menu (0-100 → 0-10x amplitude)
    MOD_TUNE  = 5U, // FM TUNE    — edit menu (0-100 → 0 to +1 octave)
    OSC_TUNE  = 6U, // PITCH      — edit menu (0-100 → 0 to +1 octave)
    FEEDBACK  = 7U, // FEEDBACK   — edit menu (0-100 → 0-2x)
    NUM_PARAMS
  };

  // ---- DSP parameter block ----------------------------------------------------
  // All values are stored pre-scaled so the audio loop does minimal work.
  // Note: defaults here must match init() and the header.c init fields.
  struct Params
  {
    float fmDepth;  // FM modulation depth (0 to ~2046); scaled for better playability
    float lfoDepth; // Hyper LFO amplitude (0..1); depth of the AND-gated pitch jump
    float lfoRate1; // LFO 1 frequency in Hz (0..10)
    float lfoRate2; // LFO 2 frequency in Hz (0..10)
    float waveFold; // Wave folder amplitude scalar (0..10); added to 1 before multiply
    int32_t modTune;// Modulator semitone offset (0-100 = 0 to +1 octave, linear approx)
    int32_t oscTune;// Carrier semitone offset   (0-100 = 0 to +1 octave, linear approx)
    float feedback; // FM feedback scalar (0..2); multiplies prev sample back into output

    void reset()
    {
      fmDepth  = 0.f;
      lfoDepth = 0.f;
      lfoRate1 = 1.f;  // 1 Hz default
      lfoRate2 = 2.f;  // 2 Hz default (offset from LFO1 so hyper gate fires)
      waveFold = 0.f;
      modTune  = 0;
      oscTune  = 0;
      feedback = 0.f;
    }

    Params() { reset(); }
  };

  // ---- Parameter setter -------------------------------------------------------
  void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case SHAPE:
      // FM depth: raw 0-1023 × 2 -> 0..2046 (linear response).
      params_.fmDepth = value * 2.f;
      break;

    case ALT:
      // Hyper LFO depth: 0-1023 → 0.0-1.0. Controls how far pitch jumps on the Hyper LFO gate.
      params_.lfoDepth = param_10bit_to_f32(value);
      break;

    case LFO_RATE1:
      // LFO 1 rate: raw 0-100 → 0-10 Hz. Matches Lyre-1: setF0(value/10, 1/fs).
      params_.lfoRate1 = value * 0.1f;
      break;

    case LFO_RATE2:
      // LFO 2 rate: same scaling as LFO 1.
      params_.lfoRate2 = value * 0.1f;
      break;

    case WAVE_FOLD:
      // Wave fold: raw 0-100 → 0..10. Matches Lyre-1: valf*10 where valf=value/100.
      params_.waveFold = value * 0.1f;
      break;

    case MOD_TUNE:
      // Modulator tuning: 0-100 integer, used as (1 + modTune/100) frequency multiplier.
      // Range: 1.0× (unison) to 2.0× (+1 octave). Matches Lyre-1 semitone/100 usage.
      params_.modTune = value;
      break;

    case OSC_TUNE:
      // Carrier tuning: same linear-semitone scheme as MOD_TUNE.
      params_.oscTune = value;
      break;

    case FEEDBACK:
      // FM feedback: raw 0-100 → 0..2. Coefficient reduced from Lyre-1's ×0.2 to ×0.02
      // so that the full knob range is musically usable without exponential blow-up.
      // At max (value=100) the feedback multiplier peaks at (1 + 1.0 × 2) = 3×, which
      // the prevSample_ clamp in process() keeps bounded rather than diverging.
      params_.feedback = value * 0.02f;
      break;

    default:
      break;
    }
  }

  // No string-type parameters in this unit.
  const char *getParameterStrValue(uint8_t, int32_t) const override final
  {
    return nullptr;
  }

  // ---- Life-cycle callbacks ---------------------------------------------------

  void init(float *) override final
  {
    params_.reset();

    // Oscillator phasors — all start at phase 0.
    carrierPhase_ = 0.f;
    modPhase_     = 0.f;
    lfo1Phase_    = 0.f;
    lfo2Phase_    = 0.f;

    // No previous sample output to feed back on first cycle.
    prevSample_   = 0.f;

    w0_  = 0.f;
    lfo_ = 0.f;
  }

  // ---- Audio parameter setters (called from unit.cc before process()) ---------

  // Normalized pitch (w = f / samplerate). Set once per render block.
  void setPitch(float w0)
  {
    w0_ = w0;
  }

  // Hardware shape_lfo in (-1.0, 1.0). Amplitude-modulates FM depth per Lyre-1.
  void setShapeLfo(float lfo)
  {
    lfo_ = lfo;
  }

  // ---- Audio render loop -------------------------------------------------------
  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    // Snapshot parameters so they are stable across the entire block.
    const Params p = params_;

    // Precompute per-sample LFO phase increments: (Hz / samplerate).
    // k_samplerate_recipf = 1/48000 ≈ 2.083e-5.
    const float lfo1W0 = p.lfoRate1 * k_samplerate_recipf;
    const float lfo2W0 = p.lfoRate2 * k_samplerate_recipf;

    // Precompute modulator tuning multiplier: (1 + semitone_offset / 100).
    // Range: 1.0 (unison) to 2.0 (+1 octave). Matches Lyre-1 semitone/100 formula.
    const float modFreqMul = 1.f + p.modTune * 0.01f;
    const float oscFreqMul = 1.f + p.oscTune * 0.01f;

    // The hardware shape_lfo is set once per block via setShapeLfo().
    // It amplitude-modulates FM depth: effective FM depth = fmDepth * (1 + lfo_) / 2.
    // Range: 0 (when lfo_ = -1) to fmDepth (when lfo_ = +1).
    const float fmScale = p.fmDepth * (1.f + lfo_) * 0.5f;

    for (const float *out_end = out + frames; out != out_end; in += 2, out += 1)
    {
      // ---- LFO update --------------------------------------------------------
      // Advance both LFO phasors by their per-sample increments and wrap to [0, 1).
      lfo1Phase_ += lfo1W0;
      lfo1Phase_ -= (uint32_t)lfo1Phase_;

      lfo2Phase_ += lfo2W0;
      lfo2Phase_ -= (uint32_t)lfo2Phase_;

      // Sample bipolar sine for each LFO (-1..1).
      const float lfo1Out = osc_sinf(lfo1Phase_);
      const float lfo2Out = osc_sinf(lfo2Phase_);

      // ---- Hyper LFO gate ----------------------------------------------------
      // Both LFOs must be in their positive half-cycle simultaneously (AND gate).
      // When the gate is open, pitch is raised by up to 3 × lfoDepth.
      // This creates the characteristic sudden harmonic pitch-jump of the Lyra-8.
      const float hyperGate = (lfo1Out >= 0.f && lfo2Out >= 0.f) ? 3.f : 0.f;
      const float hyperMod  = 1.f + hyperGate * p.lfoDepth;

      // ---- Modulator oscillator ----------------------------------------------
      // The modulator is a simple sine at the same base pitch as the carrier,
      // offset by modTune (linear) and pitch-jumped by the hyper LFO.
      const float modW0 = w0_ * modFreqMul * hyperMod;

      // Read modulator at its current phase, then advance and wrap.
      const float fmSig = osc_sinf(modPhase_);
      modPhase_ += modW0;
      modPhase_ -= (uint32_t)modPhase_;

      // ---- Carrier oscillator ------------------------------------------------
      // Carrier phase increment: base pitch (with tuning + hyper LFO) plus the
      // FM contribution (modulator output × effective FM depth × samplerate recip).
      const float carrW0 = w0_ * oscFreqMul * hyperMod
                           + fmSig * fmScale * k_samplerate_recipf;

      // Main oscillator: sine amplitude-scaled by waveFold factor.
      // prevSample_ from the previous sample feeds back to create soft chaotic FM.
      // The (1 + prevSample_ * feedback) term mirrors Lyre-1's feedback path.
      const float mainOsc = 0.5f * osc_sinf(carrierPhase_)
                            * (1.f + p.waveFold)
                            * (1.f + prevSample_ * p.feedback);

      // Advance carrier phasor and wrap to [0, 1).
      carrierPhase_ += carrW0;
      carrierPhase_ -= (uint32_t)carrierPhase_;

      // ---- Single-stage wave folder ------------------------------------------
      // Reflects any amplitude that exceeds ±0.5 back inward (triangle fold).
      // Produces even harmonics and the "crumpled" waveform typical of the Lyra-8.
      const float audioOut = (mainOsc < -0.5f) ? (-1.f - mainOsc)
                           : (mainOsc >  0.5f) ? ( 1.f - mainOsc)
                           : mainOsc;

      // Save pre-fold signal for next sample's feedback calculation.
      // Clamped to [-1, 1] to bound the multiplicative loop: without this, any
      // prevSample_ > 1/feedback causes exponential blow-up within a few samples.
      prevSample_ = clipminmaxf(-1.f, mainOsc, 1.f);

      *out = audioOut;
    }
  }

private:
  Params params_;

  // Normalized phase increment for carrier pitch (f / samplerate), set per block.
  float w0_;

  // Hardware shape_lfo in (-1, 1), amplitude-modulates FM depth.
  float lfo_;

  // Oscillator and LFO phasors — all in [0, 1), advanced per sample.
  float carrierPhase_; // carrier sine oscillator phase
  float modPhase_;     // FM modulator sine oscillator phase
  float lfo1Phase_;    // hyper LFO — sine LFO #1 phase
  float lfo2Phase_;    // hyper LFO — sine LFO #2 phase

  // One-sample delay for the FM feedback path. Holds the pre-fold carrier output.
  float prevSample_;
};
