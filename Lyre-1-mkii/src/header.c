#include "unit_osc.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api = UNIT_API_VERSION,
    .dev_id = 0x00000000U,
    .unit_id = 0x00000000U,
    .version = 0x00000100U,
    .name = "Lyre 1",
    .num_params = 8,

    .params = {
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"FM Depth"}},
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"LFO Depth"}},
        {0, 100, 0, 50, k_unit_param_type_none, 0, 0, 0, {"LFO 1"}},
        {0, 100, 0, 50, k_unit_param_type_none, 0, 0, 0, {"LFO 2"}},
        {0, 100, 0, 50, k_unit_param_type_none, 0, 0, 0, {"Wave Fold"}},
        {0, 100, 0, 50, k_unit_param_type_none, 0, 0, 0, {"FM Tune"}},
        {0, 100, 0, 50, k_unit_param_type_none, 0, 0, 0, {"Pitch"}},
        {0, 100, 0, 50, k_unit_param_type_none, 0, 0, 0, {"Feedback"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    },
};
