#include "unit_osc.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api = UNIT_API_VERSION,
    .dev_id = 0x4D616A69U,
    .unit_id = 0x00000001U,
    .version = 0x00010002U,
    .name = "Lirah-1",
    .num_params = 8,

    .params = {
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"SHPE"}},
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"ALT"}},
      {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"LFO RATE 1"}},
      {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"LFO RATE 2"}},
      {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"WAVE FOLD"}},
      {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"FM TUNE"}},
      {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"PITCH"}},
      {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"FEEDBACK"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    },
};
