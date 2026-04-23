#pragma once

#include <stdint.h>

#include "osc_api.h"

#ifdef f32_to_q31
#undef f32_to_q31
#endif

// Use saturation in legacy fallback paths to avoid q31 wrap-around artifacts.
#define f32_to_q31(f) ((q31_t)(clipminmaxf((float)(f), -1.f, 0.99999994f) * (float)0x7FFFFFFF))

#ifdef __cplusplus
extern "C" {
#endif

typedef struct user_osc_param {
  int32_t shape_lfo;
  uint16_t pitch;
  uint16_t cutoff;
  uint16_t resonance;
  uint16_t reserved0[3];
} user_osc_param_t;

typedef enum {
  k_user_osc_param_id1 = 0,
  k_user_osc_param_id2,
  k_user_osc_param_id3,
  k_user_osc_param_id4,
  k_user_osc_param_id5,
  k_user_osc_param_id6,
  k_user_osc_param_shape,
  k_user_osc_param_shiftshape,
  k_num_user_osc_param_id
} user_osc_param_id_t;

#define param_val_to_f32(val) ((uint16_t)(val) * 9.77517106549365e-004f)

#define OSC_INIT    __attribute__((used)) _hook_init
#define OSC_CYCLE   __attribute__((used)) _hook_cycle
#define OSC_CYCLE_F32 __attribute__((used)) _hook_cycle_f32
#define OSC_NOTEON  __attribute__((used)) _hook_on
#define OSC_NOTEOFF __attribute__((used)) _hook_off
#define OSC_MUTE    __attribute__((used)) _hook_mute
#define OSC_VALUE   __attribute__((used)) _hook_value
#define OSC_PARAM   __attribute__((used)) _hook_param

void _hook_init(uint32_t platform, uint32_t api);
void _hook_cycle(const user_osc_param_t *params, int32_t *yn, uint32_t frames);
void _hook_cycle_f32(const user_osc_param_t *params, float *yn, uint32_t frames);
void _hook_on(const user_osc_param_t *params);
void _hook_off(const user_osc_param_t *params);
void _hook_mute(const user_osc_param_t *params);
void _hook_value(uint16_t value);
void _hook_param(uint16_t index, uint16_t value);

#ifdef __cplusplus
}  // extern "C"
#endif
