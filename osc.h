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
 *  Original mki project by jamesdcheetham https://github.com/jamesdcheetham/Lyre-1
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
#include <cstdio>

class Osc : public Processor
{
public:
  // NTS-1 mkII oscillators do not support SDRAM allocation.
  uint32_t getBufferSize() const override final { return 0; }

  // Parameter index map.
  // 0-1: front panel knobs. 2-9: edit menu parameters.
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
    LFO3_TARGET = 8U, // LFO 3 TARGET — selectable modulation destination
    LFO3_RATE   = 9U, // LFO 3 RATE   — piecewise mapping with finer slow control
    NUM_PARAMS
  };

  enum Lfo3Target
  {
    k_lfo3_target_off = 0,
    k_lfo3_target_fm_depth,
    k_lfo3_target_hyper_depth,
    k_lfo3_target_hyper_rate1,
    k_lfo3_target_hyper_rate2,
    k_lfo3_target_fold,
    k_lfo3_target_mod_tune,
    k_lfo3_target_osc_tune,
    k_lfo3_target_feedback,
  };

  // Parameter block used by the audio loop.
  // Values are pre-scaled where possible to keep per-sample work light.
  struct Params
  {
    float fmDepth;  // FM modulation depth (0 to ~2046); scaled for better playability
    float lfoDepth; // Hyper LFO amplitude (0..1); depth of the AND-gated pitch jump
    float lfoRate1; // LFO 1 frequency in Hz (0..10)
    float lfoRate2; // LFO 2 frequency in Hz (0..10)
    float waveFold; // Wave folder amplitude scalar (0..10); added to 1 before multiply
    int32_t modTune;// Modulator tune amount (0-100 -> 1.0x to 2.0x frequency multiplier)
    int32_t oscTune;// Carrier tune amount   (0-100 -> 1.0x to 2.0x frequency multiplier)
    float feedback; // FM feedback scalar (0..2); multiplies prev sample back into output
    uint8_t lfo3Target; // LFO 3 modulation destination selector
    float lfo3Rate; // LFO 3 frequency in Hz (piecewise 0..25, slow-focused in first half)

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
      lfo3Target = k_lfo3_target_off;
      lfo3Rate = 0.f;
    }

    Params() { reset(); }
  };

  // Convert UI parameter values into DSP-friendly values.
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
      // LFO 1 rate: 0-100 -> 0-10 Hz.
      params_.lfoRate1 = value * 0.1f;
      break;

    case LFO_RATE2:
      // LFO 2 rate: same scaling as LFO 1.
      params_.lfoRate2 = value * 0.1f;
      break;

    case WAVE_FOLD:
      // Wave fold: 0-100 -> 0-10 scalar before folding.
      params_.waveFold = value * 0.1f;
      break;

    case MOD_TUNE:
      // Modulator tune as linear multiplier control.
      params_.modTune = value;
      break;

    case OSC_TUNE:
      // Carrier tune as linear multiplier control.
      params_.oscTune = value;
      break;

    case FEEDBACK:
      // Feedback depth: 0-100 -> 0-2.
      // The output clamp in process() keeps this stable at high settings.
      params_.feedback = value * 0.02f;
      break;

    case LFO3_TARGET:
      if (value < k_lfo3_target_off)
        value = k_lfo3_target_off;
      if (value > k_lfo3_target_feedback)
        value = k_lfo3_target_feedback;
      params_.lfo3Target = static_cast<uint8_t>(value);
      break;

    case LFO3_RATE:
      // Piecewise mapping for better low-speed control:
      // 0-50  -> 0-1 Hz (extra-fine slow start)
      // 50-100 -> 1-25 Hz (faster sweep retained)
      {
        const float norm = clipminmaxf(0.f, value * 0.01f, 1.f);
        if (norm <= 0.5f)
        {
          params_.lfo3Rate = norm * 2.f;
        }
        else
        {
          params_.lfo3Rate = 1.f + (norm - 0.5f) * 48.f;
        }
      }
      break;

    default:
      break;
    }
  }

  // Custom text for string-type LFO target display.
  const char *getParameterStrValue(uint8_t id, int32_t value) const override final
  {
    static const char *targetNames[] = {
      "OFF", "FMDEP", "HDEP", "HR1", "HR2", "FOLD", "FMTUN", "OTUN", "FDBK"
    };

    if (id != LFO3_TARGET)
      return nullptr;

    const int32_t target = clipminmaxi32(k_lfo3_target_off, value, k_lfo3_target_feedback);
    return targetNames[target];
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
    lfo3Phase_    = 0.f;

    // No previous sample output to feed back on first cycle.
    prevSample_   = 0.f;

    w0_  = 0.f;
    lfo_ = 0.f;
  }

  // Called once per render block from the unit runtime.
  void setPitch(float w0)
  {
    w0_ = w0;
  }

  // Hardware shape LFO in (-1, 1). Used to scale FM depth.
  void setShapeLfo(float lfo)
  {
    lfo_ = lfo;
  }

  // Main audio render loop.
  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    // Copy parameter block once so values stay stable during this buffer.
    const Params p = params_;

    // LFO 3 phase increment per sample.
    const float lfo3W0 = p.lfo3Rate * k_samplerate_recipf;

    for (const float *out_end = out + frames; out != out_end; in += 2, out += 1)
    {
      float fmDepthNow = p.fmDepth;
      float hyperDepthNow = p.lfoDepth;
      float hyperRate1Now = p.lfoRate1;
      float hyperRate2Now = p.lfoRate2;
      float foldNow = p.waveFold;
      float modTuneNow = static_cast<float>(p.modTune);
      float oscTuneNow = static_cast<float>(p.oscTune);
      float feedbackNow = p.feedback;

      lfo3Phase_ += lfo3W0;
      lfo3Phase_ -= (uint32_t)lfo3Phase_;
      const float lfo3 = osc_sinf(lfo3Phase_);

      applyLfo3Modulation(p.lfo3Target,
              lfo3,
              fmDepthNow,
              hyperDepthNow,
              hyperRate1Now,
              hyperRate2Now,
              foldNow,
              modTuneNow,
              oscTuneNow,
              feedbackNow);

      const float lfo1W0Now = hyperRate1Now * k_samplerate_recipf;
      const float lfo2W0Now = hyperRate2Now * k_samplerate_recipf;

      // Update the two Hyper LFO phases.
      lfo1Phase_ += lfo1W0Now;
      lfo1Phase_ -= (uint32_t)lfo1Phase_;

      lfo2Phase_ += lfo2W0Now;
      lfo2Phase_ -= (uint32_t)lfo2Phase_;

      // Sine outputs in the range -1..1.
      const float lfo1Out = osc_sinf(lfo1Phase_);
      const float lfo2Out = osc_sinf(lfo2Phase_);

      // Hyper gate opens only when both LFO lanes are positive.
      const float hyperGate = (lfo1Out >= 0.f && lfo2Out >= 0.f) ? 3.f : 0.f;
      const float hyperMod  = 1.f + hyperGate * hyperDepthNow;

      // Shape LFO scales FM depth from 0..full depth.
      const float fmScale = fmDepthNow * (1.f + lfo_) * 0.5f;

      const float modFreqMul = 1.f + modTuneNow * 0.01f;
      const float oscFreqMul = 1.f + oscTuneNow * 0.01f;

      // Modulator frequency follows base pitch, tune offset, and Hyper gate.
      const float modW0 = w0_ * modFreqMul * hyperMod;

      // Read modulator sample, then step phase.
      const float fmSig = osc_sinf(modPhase_);
      modPhase_ += modW0;
      modPhase_ -= (uint32_t)modPhase_;

      // Carrier phase step = tuned base pitch + FM contribution.
      const float carrW0 = w0_ * oscFreqMul * hyperMod
                           + fmSig * fmScale * k_samplerate_recipf;

      // Carrier output with fold drive and feedback from the previous sample.
      const float mainOsc = 0.5f * osc_sinf(carrierPhase_)
                            * (1.f + foldNow)
                            * (1.f + prevSample_ * feedbackNow);

      // Step carrier phase.
      carrierPhase_ += carrW0;
      carrierPhase_ -= (uint32_t)carrierPhase_;

      // Single-stage fold with simple reflection above/below +/-0.5.
      const float audioOut = (mainOsc < -0.5f) ? (-1.f - mainOsc)
                           : (mainOsc >  0.5f) ? ( 1.f - mainOsc)
                           : mainOsc;

      // Keep feedback memory bounded for stability.
      prevSample_ = clipminmaxf(-1.f, mainOsc, 1.f);

      *out = audioOut;
    }
  }

private:
  static float clampf(float v, float lo, float hi)
  {
    return clipminmaxf(lo, v, hi);
  }

  static void applyLfo3Modulation(uint8_t target,
                                  float mod,
                                  float &fmDepth,
                                  float &hyperDepth,
                                  float &hyperRate1,
                                  float &hyperRate2,
                                  float &fold,
                                  float &modTune,
                                  float &oscTune,
                                  float &feedback)
  {
    switch (target)
    {
    case k_lfo3_target_fm_depth:
      fmDepth = clampf(fmDepth + mod * 350.f, 0.f, 2046.f);
      break;
    case k_lfo3_target_hyper_depth:
      hyperDepth = clampf(hyperDepth + mod * 0.25f, 0.f, 1.f);
      break;
    case k_lfo3_target_hyper_rate1:
      hyperRate1 = clampf(hyperRate1 + mod * 2.f, 0.f, 10.f);
      break;
    case k_lfo3_target_hyper_rate2:
      hyperRate2 = clampf(hyperRate2 + mod * 2.f, 0.f, 10.f);
      break;
    case k_lfo3_target_fold:
      fold = clampf(fold + mod * 2.f, 0.f, 10.f);
      break;
    case k_lfo3_target_mod_tune:
      modTune = clampf(modTune + mod * 20.f, 0.f, 100.f);
      break;
    case k_lfo3_target_osc_tune:
      oscTune = clampf(oscTune + mod * 20.f, 0.f, 100.f);
      break;
    case k_lfo3_target_feedback:
      feedback = clampf(feedback + mod * 0.2f, 0.f, 2.f);
      break;
    default:
      break;
    }
  }

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
  float lfo3Phase_;    // assignable modulation LFO #3 phase

  // One-sample delay for the FM feedback path. Holds the pre-fold carrier output.
  float prevSample_;

};
