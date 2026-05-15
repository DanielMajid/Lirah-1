// reference: https://emscripten.org/docs/api_reference/wasm_audio_worklets.html#wasm-audio-worklets
// example code: https://github.com/emscripten-core/emscripten/tree/main/test/webaudio

#include <emscripten/bind.h>
#include <emscripten/webaudio.h>
#include <emscripten/em_math.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

using namespace emscripten;

#include "unit.h"
#include "unit_osc.h"

// this needs to be big enough for the stereo output, inputs, params and the worker stack
uint8_t audioThreadStack[4096];

constexpr int SAMPLE_RATE = 48000;
constexpr int WEB_AUDIO_FRAME_SIZE = 128;
std::array<float, WEB_AUDIO_FRAME_SIZE> interleavedOut;

extern const unit_header_t unit_header;

static float BPM_WASM = 120.f;

namespace {

unit_runtime_osc_context_t runtime_osc_context = {};
unit_runtime_desc_t runtime_desc = {};
bool unit_initialized = false;

uint16_t current_pitch = static_cast<uint16_t>(69U << 8);

// Envelope generator to simulate platform shape_lfo modulation
// This varies over time to dampen FM effect, matching hardware behavior
static uint32_t envelope_sample_counter = 0;
static constexpr uint32_t ENVELOPE_PERIOD_SAMPLES = 96000;  // ~2 seconds at 48kHz

void update_shape_lfo_envelope() {
  // Generate a slow triangular wave that dampens the FM effect
  // Range: -0.5 to 0.5 (will be used as lfoShape multiplier)
  float phase = static_cast<float>(envelope_sample_counter) / static_cast<float>(ENVELOPE_PERIOD_SAMPLES);
  phase = phase - static_cast<float>(static_cast<int>(phase));  // wrap to 0-1
  
  float envelope;
  if (phase < 0.5f) {
    envelope = phase * 2.f - 0.5f;  // 0 -> 0.5: -0.5 -> +0.5
  } else {
    envelope = (1.f - phase) * 2.f - 0.5f;  // 0.5 -> 1: +0.5 -> -0.5
  }
  
  // Convert to q31 fixed point (-1.0 to +1.0 range)
  runtime_osc_context.shape_lfo = static_cast<int32_t>(envelope * static_cast<float>(0x7FFFFFFF));
  envelope_sample_counter += WEB_AUDIO_FRAME_SIZE;
}

void set_runtime_pitch_from_hz(float hz)
{
  if (hz <= 0.f) {
    return;
  }

  float midi = 69.f + 12.f * std::log2(hz / 440.f);
  midi = std::clamp(midi, 0.f, 127.996f);

  int note = static_cast<int>(std::floor(midi));
  int frac = static_cast<int>((midi - static_cast<float>(note)) * 256.f + 0.5f);
  if (frac > 255) {
    frac = 0;
    note = std::min(note + 1, 127);
  }

  current_pitch = static_cast<uint16_t>((note << 8) | frac);
  runtime_osc_context.pitch = current_pitch;
}

void initialize_unit_runtime(uint32_t sample_rate)
{
  if (unit_initialized) {
    return;
  }

  runtime_osc_context.shape_lfo = 0;
  runtime_osc_context.pitch = current_pitch;
  runtime_osc_context.cutoff = 0;
  runtime_osc_context.resonance = 0;
  runtime_osc_context.amp_eg_phase = 0;
  runtime_osc_context.amp_eg_state = 0;
  runtime_osc_context.notify_input_usage = nullptr;

  runtime_desc.target = unit_header.target;
  runtime_desc.api = UNIT_API_VERSION;
  runtime_desc.samplerate = sample_rate;
  runtime_desc.frames_per_buffer = WEB_AUDIO_FRAME_SIZE;
  runtime_desc.input_channels = 2;
  runtime_desc.output_channels = 1;
  runtime_desc.hooks.runtime_context = reinterpret_cast<const unit_runtime_base_context_t *>(&runtime_osc_context);
  runtime_desc.hooks.sdram_alloc = nullptr;
  runtime_desc.hooks.sdram_free = nullptr;
  runtime_desc.hooks.sdram_avail = nullptr;

  const int8_t init_result = unit_init(&runtime_desc);
  if (init_result != k_unit_err_none) {
    std::printf("unit_init failed: %d\n", static_cast<int>(init_result));
    return;
  }

  unit_reset();
  unit_resume();
  unit_initialized = true;
}

}  // namespace

void fx_set_bpm(float bpm)
{
  BPM_WASM = bpm;
  if (unit_initialized) {
    unit_set_tempo(static_cast<uint32_t>(bpm * 65536.f));
  }
}

uint16_t fx_get_bpm(void)
{
  return static_cast<int>(BPM_WASM * 10.f);
}

float fx_get_bpmf(void)
{
  return BPM_WASM;
}

struct AudioWorkletParameter
{
  int min;
  int max;
  int center;
  int init;
  uint8_t type;
  std::string name;
};

std::string getParameterValueString(int index, int value)
{
  const unit_param_t &p = unit_header.params[index];

  std::string suffix;

  switch (p.type)
  {
  case k_unit_param_type_none:
    break;
  case k_unit_param_type_percent:
    suffix = "%";
    break;
  case k_unit_param_type_db:
    suffix = " dB";
    break;
  case k_unit_param_type_cents:
    suffix = " cents";
    break;
  case k_unit_param_type_semi:
    suffix = " semitones";
    break;
  case k_unit_param_type_oct:
    suffix = " octaves";
    break;
  case k_unit_param_type_hertz:
    suffix = " Hz";
    break;
  case k_unit_param_type_khertz:
    suffix = " kHz";
    break;
  case k_unit_param_type_bpm:
    suffix = " bpm";
    break;
  case k_unit_param_type_msec:
    suffix = " ms";
    break;
  case k_unit_param_type_sec:
    suffix = " s";
    break;
  case k_unit_param_type_enum:
    break;
  case k_unit_param_type_strings:
    {
      const char *param_text = unit_get_param_str_value(static_cast<uint8_t>(index), value);
      return param_text ? std::string(param_text) : std::string();
    }
    break;
  case k_unit_param_type_drywet:
    suffix = "%";
    break;
  case k_unit_param_type_pan:
  case k_unit_param_type_spread:

    if (value < 0)
    {
      suffix = "L";
    }
    else if (value > 0)
    {
      suffix = "R";
    }
    else if (value == p.center)
    {
      return "CNTR";
    }
    break;

  case k_unit_param_type_onoff:
    if (value == 0)
    {
      return "OFF";
    }
    else
    {
      return "ON";
    }
    break;
  case k_unit_param_type_midi_note:
    // todo
  default:
    return "unimplemented";
    break;
  };

  std::string numerical;
  if (p.frac_mode == k_unit_param_frac_mode_fixed)
  {
    numerical = std::to_string(value / static_cast<double>(1 << p.frac));
  }
  else
  {
    numerical = std::to_string(value / std::pow(10.0, p.frac));
  }
  numerical.erase(numerical.find_last_not_of('0') + 1);
  if (!numerical.empty() && numerical.back() == '.')
  {
    numerical.pop_back();
  }

  return numerical + suffix;
}

std::vector<AudioWorkletParameter> getValidParameters()
{
  std::vector<AudioWorkletParameter> result;
  for (int i = 0; i < unit_header.num_params; ++i)
  {
    const unit_param_t &p = unit_header.params[i];
    result.push_back({p.min,
                      p.max,
                      p.center,
                      p.init,
                      p.type,
                      std::string(p.name)});
  }
  return result;
}

void setOscPitch(float f0)
{
  set_runtime_pitch_from_hz(f0);
}

void noteOn(uint8_t note, uint8_t velocity)
{
  current_pitch = static_cast<uint16_t>(note) << 8;
  runtime_osc_context.pitch = current_pitch;
  if (unit_initialized) {
    unit_note_on(note, velocity);
  }
  // printf("Note On: %d, velocity %d\n", note, velocity);
}

// note off velocity is not supported by logue-sdk
void noteOff(uint8_t note)
{
  if (unit_initialized) {
    unit_note_off(note);
  }
  // printf("Note Off: %d\n", note);
}

// bind unit parameters
EMSCRIPTEN_BINDINGS(my_module)
{
  value_object<AudioWorkletParameter>("AudioWorkletParameter")
      .field("min", &AudioWorkletParameter::min)
      .field("max", &AudioWorkletParameter::max)
      .field("center", &AudioWorkletParameter::center)
      .field("init", &AudioWorkletParameter::init)
      .field("type", &AudioWorkletParameter::type)
      .field("name", &AudioWorkletParameter::name);

  register_vector<AudioWorkletParameter>("ParameterList");

  function("getValidParameters", &getValidParameters);

  function("getParameterValueString", &getParameterValueString);

  function("fx_set_bpm", &fx_set_bpm);

  function("setOscPitch", &setOscPitch);

  function("noteOn", &noteOn);

  function("noteOff", &noteOff);
}

bool ProcessAudio(int numInputs, const AudioSampleFrame *inputs,
                  int numOutputs, AudioSampleFrame *outputs,
                  int numParams, const AudioParamFrame *params,
                  void *userData)
{
  assert(numInputs == 0);
  assert(numOutputs == 1);
  assert(outputs->numberOfChannels == 1);
  assert(outputs->samplesPerChannel == WEB_AUDIO_FRAME_SIZE);
  auto &output = outputs[0];

  for (int i = 0; i < numParams; ++i)
  {
    // K-rate parameter: use the first sample for the frame
    const float value = params[i].data[0];
    unit_set_param_value(static_cast<uint8_t>(i), static_cast<int32_t>(std::lround(value)));
  }

  // Update shape_lfo envelope to simulate platform modulation
  update_shape_lfo_envelope();

  // emscripten_log(EM_LOG_CONSOLE, "bpm=%d", fx_get_bpmf());
  unit_render(nullptr, interleavedOut.data(), WEB_AUDIO_FRAME_SIZE);

  // de-interleave output buffer
  for (int i = 0; i < WEB_AUDIO_FRAME_SIZE; ++i)
  {
    output.data[i] = interleavedOut[i];
  }
  return true; // Keep the graph output going
}

void AudioWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData)
{
  if (!success)
    return; // Check browser console in a debug build for detailed errors

  // no input, single mono output
  int outputChannelCounts[1] = {1};
  EmscriptenAudioWorkletNodeCreateOptions options = {
      .numberOfInputs = 0,
      .numberOfOutputs = 1,
      .outputChannelCounts = outputChannelCounts};

  EMSCRIPTEN_AUDIO_WORKLET_NODE_T wasmAudioWorklet = emscripten_create_wasm_audio_worklet_node(audioContext,
                                                                                               "logue-osc", &options, &ProcessAudio, 0);

  EM_ASM({ setupWebAudioAndUI(emscriptenGetAudioObject($0), emscriptenGetAudioObject($1)); }, audioContext, wasmAudioWorklet);
}

void AudioThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData)
{
  if (!success)
    return; // Check browser console in a debug build for detailed errors

  initialize_unit_runtime(static_cast<uint32_t>(emscripten_audio_context_sample_rate(audioContext)));
  if (!unit_initialized) {
    return;
  }

  auto valid_parameters = getValidParameters();

  WebAudioParamDescriptor params[valid_parameters.size()];
  for (int i = 0; i < valid_parameters.size(); ++i)
  {
    params[i].automationRate = WEBAUDIO_PARAM_K_RATE;
    params[i].defaultValue = valid_parameters[i].init;
    params[i].minValue = valid_parameters[i].min;
    params[i].maxValue = valid_parameters[i].max;
  }

  WebAudioWorkletProcessorCreateOptions opts = {
      .name = "logue-osc",
      .numAudioParams = static_cast<int>(valid_parameters.size()),
      .audioParamDescriptors = params};

  emscripten_create_wasm_audio_worklet_processor_async(audioContext, &opts, &AudioWorkletProcessorCreated, 0);
}

int main()
{
  EmscriptenWebAudioCreateAttributes attrs = {
      .latencyHint = "interactive",
      .sampleRate = SAMPLE_RATE};

  EMSCRIPTEN_WEBAUDIO_T context = emscripten_create_audio_context(&attrs);

  int sample_rate = emscripten_audio_context_sample_rate(context);
  int frame_size = emscripten_audio_context_quantum_size(context);
  printf("Sample rate: %d\n", sample_rate);
  printf("Frame size: %d\n", frame_size);

  emscripten_start_wasm_audio_worklet_thread_async(context, audioThreadStack, sizeof(audioThreadStack),
                                                   &AudioThreadInitialized, 0);

  emscripten_exit_with_live_runtime();
}