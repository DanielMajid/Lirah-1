#pragma once

#include <cmath>
#include <cstdint>

#include "processor.h"
#include "unit_osc.h"
#include "utils/int_math.h"

class Osc : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; } // NTS-1 osc do not support sdram allocation

  // audio parameters
  enum
  {
    SHAPE = 0U,
    ALT,
    LFO_RATE_1,
    LFO_RATE_2,
    WAVE_FOLD,
    FM_TUNE,
    PITCH,
    FEEDBACK,
    NUM_PARAMS
  };

  // Note: Make sure that default param values correspond to declarations in header.c
  struct Params
  {
    float shape;
    float alt;
    float lfo_rate_1;
    float lfo_rate_2;
    float wave_fold;
    float fm_tune;
    float pitch;
    float feedback;

    void reset()
    {
      shape = 0.f;
      alt = 0.f;
      lfo_rate_1 = 0.f;
      lfo_rate_2 = 0.f;
      wave_fold = 0.f;
      fm_tune = 0.f;
      pitch = 0.f;
      feedback = 0.f;
    }

    Params() { reset(); }
  };

  void setParameter(uint8_t index, int32_t value) override final
  {
    if (index >= UNIT_OSC_MAX_PARAM_COUNT)
      return;

    value = clipminmaxi32(unit_header.params[index].min, value, unit_header.params[index].max);

    switch (index)
    {
    case SHAPE:
      params_.shape = static_cast<float>(value); // 0 .. 1023
      break;

    case ALT:
      params_.alt = static_cast<float>(value); // 0 .. 1023
      break;

    case LFO_RATE_1:
      params_.lfo_rate_1 = static_cast<float>(value); // 0 .. 100
      break;

    case LFO_RATE_2:
      params_.lfo_rate_2 = static_cast<float>(value); // 0 .. 100
      break;

    case WAVE_FOLD:
      params_.wave_fold = static_cast<float>((value * 1023 + 50) / 100); // 0 .. 100 -> 0 .. 1023
      break;

    case FM_TUNE:
      params_.fm_tune = static_cast<float>(value); // 0 .. 100
      break;

    case PITCH:
      params_.pitch = static_cast<float>(value); // 0 .. 100
      break;

    case FEEDBACK:
      params_.feedback = static_cast<float>((value * 1023 + 50) / 100); // 0 .. 100 -> 0 .. 1023
      break;

    default:
      break;
    }
  }

  const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    (void)index;
    (void)value;
    return nullptr;
  }

  // life-cycle methods
  void init(float *) override final
  {
    params_.reset();
    w0_ = 0.f;
    lfo_ = 0.f;
    lfoz_ = 0.f;

    carrier_phase_ = 0.f;
    mod_phase_ = 0.f;
    lfo_a_phase_ = 0.f;
    lfo_b_phase_ = 0.f;
  }

  // audio processing callbacks

  // set frequency in digital w (w = f/samplerate, 0.5 is Nyquist)
  void setPitch(float w0)
  {
    pitch_hz_ = (w0 > 0.0f) ? (w0 * getSampleRate()) : 0.0f;
  }

  // lfo in (-1.0f, 1.0f)
  void setShapeLfo(float lfo)
  {
    lfo_ = (lfo < -1.f) ? -1.f : ((lfo > 1.f) ? 1.f : lfo);
  }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    (void)in;

    // Caching current parameter values. Consider smoothing sensitive parameters in audio loop
    const Params p = params_;

    if (pitch_hz_ <= 0.f)
    {
      for (uint32_t i = 0; i < frames; ++i)
        out[i] = 0.f;
      return;
    }

    const float sample_rate = getSampleRate();
    const float pi2 = 6.283185307179586f;

    const float lfo_a_hz = p.lfo_rate_1 / 10.f;
    const float lfo_b_hz = p.lfo_rate_2 / 10.f;
    const float lfo_a_inc = pi2 * lfo_a_hz / sample_rate;
    const float lfo_b_inc = pi2 * lfo_b_hz / sample_rate;
    const float shape_inc = (lfo_ - lfoz_) / static_cast<float>(frames);

    float shape_lfo = lfoz_;

    for (const float *out_end = out + frames; out != out_end; out += 1)
    {
      lfo_a_phase_ += lfo_a_inc;
      lfo_b_phase_ += lfo_b_inc;

      if (lfo_a_phase_ >= pi2)
        lfo_a_phase_ -= pi2;
      if (lfo_b_phase_ >= pi2)
        lfo_b_phase_ -= pi2;

      const float lfo2_out = std::sin(lfo_a_phase_);
      const float lfo_out = std::sin(lfo_b_phase_);
      const float hyper_lfo = 1.f + ((lfo_out >= 0.f && lfo2_out >= 0.f) ? 3.f * (p.alt / 1023.f) : 0.f);

      const float carrier = std::sin(pi2 * carrier_phase_);
      const float wave_fold = (p.wave_fold / 1023.f) * 10.f;
      const float feedback = (p.feedback / 1023.f) * 20.f;
      const float main_osc = 0.5f * carrier * (1.f + wave_fold) * (1.f + carrier * feedback);

      const float fm_osc = std::sin(pi2 * mod_phase_);
      const float mod_semitone = p.fm_tune;
      const float carrier_semitone = p.pitch;
      const float fm_depth = p.shape * 10.f;

      const float w1 = (pitch_hz_ * (1.f + (mod_semitone / 100.f)) * hyper_lfo) / sample_rate;

      mod_phase_ += w1;
      if (mod_phase_ >= 1.f)
        mod_phase_ -= static_cast<float>(static_cast<uint32_t>(mod_phase_));

      const float audio_out = (main_osc < -0.5f)
                                  ? (-1.f - main_osc)
                                  : ((main_osc > 0.5f) ? (1.f - main_osc) : main_osc);

      const float w0 = (pitch_hz_ * (1.f + (carrier_semitone / 100.f)) * hyper_lfo +
                        (fm_osc * (fm_depth * (1.f + shape_lfo)) / 2.f)) /
                       sample_rate;

      carrier_phase_ += w0;
      if (carrier_phase_ >= 1.f)
        carrier_phase_ -= static_cast<float>(static_cast<uint32_t>(carrier_phase_));

      out[0] = audio_out;

      shape_lfo += shape_inc;
    }

    lfoz_ = lfo_;
  }

private:
  Params params_;
  float w0_;
  float lfo_;

  float pitch_hz_ = 440.0f;
  float lfoz_ = 0.0f;

  // local variables related to audio processing
  float carrier_phase_ = 0.0f;
  float mod_phase_ = 0.0f;
  float lfo_a_phase_ = 0.0f;
  float lfo_b_phase_ = 0.0f;
  float phasor_ = 0.0f;
};
