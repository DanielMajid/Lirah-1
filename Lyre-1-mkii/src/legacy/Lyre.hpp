#pragma once

#include "userosc.h"

typedef struct Osc {
  float phase;
  float waveFold;
  float fmDepth;
  float lfoDepth;
  float feedback;
  uint8_t octave;
  uint8_t semitone;
  uint8_t cent;
} Osc;