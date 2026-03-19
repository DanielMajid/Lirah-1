#include <cstdint>

#include "unit_osc.h"
#include "utils/int_math.h"
#include "userosc.h"

extern "C" {

__attribute__((weak)) void _hook_init(uint32_t platform, uint32_t api) {
  (void)platform;
  (void)api;
}

__attribute__((weak)) void _hook_cycle(const user_osc_param_t *params, int32_t *out, uint32_t frames) {
  (void)params;
  for (uint32_t index = 0; index < frames; ++index) {
    out[index] = 0;
  }
}

__attribute__((weak)) void _hook_on(const user_osc_param_t *params) {
  (void)params;
}

__attribute__((weak)) void _hook_off(const user_osc_param_t *params) {
  (void)params;
}

__attribute__((weak)) void _hook_mute(const user_osc_param_t *params) {
  (void)params;
}

__attribute__((weak)) void _hook_value(uint16_t value) {
  (void)value;
}

__attribute__((weak)) void _hook_param(uint16_t index, uint16_t value) {
  (void)index;
  (void)value;
}

}  // extern "C"

namespace {

constexpr uint32_t kMaxRenderFrames = 64;
constexpr uint8_t kLegacyFirstEditParamId = 2;
constexpr uint8_t kLegacyLastEditParamId = 7;
constexpr uint16_t kInvalidLegacyParamId = 0xFFFF;

const unit_runtime_osc_context_t *runtime_context = nullptr;
int32_t cached_values[UNIT_OSC_MAX_PARAM_COUNT] = {};
int32_t output_q31[kMaxRenderFrames] = {};

user_osc_param_t make_legacy_params() {
  user_osc_param_t params = {};
  if (runtime_context) {
    params.shape_lfo = runtime_context->shape_lfo;
    params.pitch = runtime_context->pitch;
    params.cutoff = runtime_context->cutoff;
    params.resonance = runtime_context->resonance;
  }
  return params;
}

user_osc_param_t make_note_params(uint8_t note) {
  user_osc_param_t params = make_legacy_params();
  params.pitch = static_cast<uint16_t>(note) << 8;
  return params;
}

uint16_t map_param_id(uint8_t id) {
  if (id == k_unit_osc_fixed_param_shape) {
    return k_user_osc_param_shape;
  }

  if (id == k_unit_osc_fixed_param_altshape) {
    return k_user_osc_param_shiftshape;
  }

  if (id >= kLegacyFirstEditParamId && id <= kLegacyLastEditParamId) {
    return static_cast<uint16_t>(id - kLegacyFirstEditParamId);
  }

  return kInvalidLegacyParamId;
}

}  // namespace

__unit_callback int8_t unit_init(const unit_runtime_desc_t *desc) {
  if (!desc) {
    return k_unit_err_undef;
  }

  if (desc->target != unit_header.target) {
    return k_unit_err_target;
  }

  if (!UNIT_API_IS_COMPAT(desc->api)) {
    return k_unit_err_api_version;
  }

  if (desc->input_channels != 2 || desc->output_channels != 1) {
    return k_unit_err_geometry;
  }

  runtime_context = static_cast<const unit_runtime_osc_context_t *>(desc->hooks.runtime_context);
  _hook_init(desc->target, desc->api);

  for (uint8_t index = 0; index < UNIT_OSC_MAX_PARAM_COUNT; ++index) {
    unit_set_param_value(index, static_cast<int32_t>(unit_header.params[index].init));
  }

  return k_unit_err_none;
}

__unit_callback void unit_teardown() {
  runtime_context = nullptr;
}

__unit_callback void unit_reset() {}
__unit_callback void unit_resume() {}
__unit_callback void unit_suspend() {}

__unit_callback void unit_render(const float *in, float *out, uint32_t frames) {
  (void)in;

  if (frames > kMaxRenderFrames) {
    frames = kMaxRenderFrames;
  }

  const user_osc_param_t params = make_legacy_params();
  _hook_cycle(&params, output_q31, frames);

  for (uint32_t index = 0; index < frames; ++index) {
    out[index] = q31_to_f32(output_q31[index]);
  }
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  if (id >= UNIT_OSC_MAX_PARAM_COUNT) {
    return;
  }

  value = clipminmaxi32(unit_header.params[id].min, value, unit_header.params[id].max);
  cached_values[id] = value;

  const uint16_t mapped_index = map_param_id(id);

  if (mapped_index != kInvalidLegacyParamId) {
    _hook_param(mapped_index, static_cast<uint16_t>(value));
  }
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
  if (id >= UNIT_OSC_MAX_PARAM_COUNT) {
    return 0;
  }

  return cached_values[id];
}

__unit_callback const char *unit_get_param_str_value(uint8_t id, int32_t value) {
  (void)id;
  (void)value;
  return nullptr;
}

__unit_callback void unit_note_on(uint8_t note, uint8_t velocity) {
  (void)velocity;

  const user_osc_param_t params = make_note_params(note);
  _hook_on(&params);
}

__unit_callback void unit_note_off(uint8_t note) {
  const user_osc_param_t params = make_note_params(note);
  _hook_off(&params);
}

__unit_callback void unit_all_note_off(void) {
  user_osc_param_t params = make_legacy_params();
  _hook_mute(&params);
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
  (void)tempo;
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
  (void)counter;
}

__unit_callback void unit_pitch_bend(uint16_t bend) {
  _hook_value(bend);
}

__unit_callback void unit_channel_pressure(uint8_t pressure) {
  _hook_value(pressure);
}

__unit_callback void unit_aftertouch(uint8_t note, uint8_t pressure) {
  (void)note;
  _hook_value(pressure);
}
