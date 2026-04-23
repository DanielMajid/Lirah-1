#include <algorithm>
#include <cstdint>

#include "unit_osc.h"
#include "utils/int_math.h"

extern "C" {

typedef struct legacy_user_osc_param {
  int32_t shape_lfo;
  uint16_t pitch;
  uint16_t cutoff;
  uint16_t resonance;
  uint16_t reserved0[3];
} legacy_user_osc_param_t;

enum {
  k_user_osc_param_id1 = 0,
  k_user_osc_param_id2,
  k_user_osc_param_id3,
  k_user_osc_param_id4,
  k_user_osc_param_id5,
  k_user_osc_param_id6,
  k_user_osc_param_shape,
  k_user_osc_param_shiftshape,
};

__attribute__((weak)) void _hook_init(uint32_t platform, uint32_t api) {
  (void)platform;
  (void)api;
}

__attribute__((weak)) void _hook_cycle(const legacy_user_osc_param_t *params, int32_t *out, uint32_t frames) {
  (void)params;
  for (uint32_t index = 0; index < frames; ++index) {
    out[index] = 0;
  }
}

__attribute__((weak)) void _hook_cycle_f32(const legacy_user_osc_param_t *params, float *out, uint32_t frames) {
  (void)params;
  for (uint32_t index = 0; index < frames; ++index) {
    out[index] = 0.f;
  }
}

__attribute__((weak)) void _hook_on(const legacy_user_osc_param_t *params) {
  (void)params;
}

__attribute__((weak)) void _hook_off(const legacy_user_osc_param_t *params) {
  (void)params;
}

__attribute__((weak)) void _hook_mute(const legacy_user_osc_param_t *params) {
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
constexpr float kQ31ToF32 = 4.65661287307739e-10f;

const unit_runtime_osc_context_t *runtime_context = nullptr;
int32_t cached_values[UNIT_OSC_MAX_PARAM_COUNT] = {};
int32_t output_q31[kMaxRenderFrames] = {};

legacy_user_osc_param_t make_legacy_params() {
  legacy_user_osc_param_t params = {};
  if (runtime_context) {
    params.shape_lfo = runtime_context->shape_lfo;
    params.pitch = runtime_context->pitch;
    params.cutoff = runtime_context->cutoff;
    params.resonance = runtime_context->resonance;
  }
  return params;
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

  runtime_context = static_cast<const unit_runtime_osc_context_t *>(desc->hooks.runtime_context);
  _hook_init(desc->target, desc->api);

  for (uint8_t index = 0; index < UNIT_OSC_MAX_PARAM_COUNT; ++index) {
    cached_values[index] = static_cast<int32_t>(unit_header.params[index].init);
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

  const legacy_user_osc_param_t params = make_legacy_params();

  uint32_t rendered = 0;
  while (rendered < frames) {
    const uint32_t chunk = std::min(kMaxRenderFrames, frames - rendered);

    _hook_cycle(&params, output_q31, chunk);

    for (uint32_t index = 0; index < chunk; ++index) {
      out[rendered + index] = output_q31[index] * kQ31ToF32;
    }

    rendered += chunk;
  }
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  if (id >= UNIT_OSC_MAX_PARAM_COUNT) {
    return;
  }

  value = clipminmaxi32(unit_header.params[id].min, value, unit_header.params[id].max);
  cached_values[id] = value;

  uint16_t mapped_index = 0xFFFF;
  if (id == k_unit_osc_fixed_param_shape) {
    mapped_index = k_user_osc_param_shape;
  } else if (id == k_unit_osc_fixed_param_altshape) {
    mapped_index = k_user_osc_param_shiftshape;
  } else if (id >= 2 && id <= 7) {
    mapped_index = static_cast<uint16_t>(id - 2);
  }

  if (mapped_index != 0xFFFF) {
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

  legacy_user_osc_param_t params = make_legacy_params();
  params.pitch = static_cast<uint16_t>(note) << 8;
  _hook_on(&params);
}

__unit_callback void unit_note_off(uint8_t note) {
  legacy_user_osc_param_t params = make_legacy_params();
  params.pitch = static_cast<uint16_t>(note) << 8;
  _hook_off(&params);
}

__unit_callback void unit_all_note_off(void) {
  legacy_user_osc_param_t params = make_legacy_params();
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
