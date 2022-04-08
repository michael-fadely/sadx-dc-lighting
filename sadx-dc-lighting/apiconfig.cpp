#include "stdafx.h"
#include "apiconfig.h"

std::unordered_map<const NJS_MATERIAL*, std::deque<lantern_material_cb>> apiconfig::material_callbacks {};

bool apiconfig::object_vcolor      = true;
bool apiconfig::object_mcolor      = true;
bool apiconfig::override_light_dir = false;

NJS_VECTOR apiconfig::light_dir_override { 0.0f, -1.0f, 0.0f };

float apiconfig::alpha_ref_value   = 16.0f / 255.0f;
bool  apiconfig::alpha_ref_is_temp = false;
