// Copyright (c) 2024 WAGO GmbH & Co. KG
// SPDX-License-Identifier: MPL-2.0

#include "switch_data_provider.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

namespace wago::libswitchconfig {
  void to_json(nlohmann::json& j, const ::wago::libswitchconfig::supported_values& sv);
  void to_json(nlohmann::json& j, const ::wago::libswitchconfig::supported_values& sv) {
    j = nlohmann::json{{"version", sv.version},
                  {"broadcast-protection", sv.broadcast_protection_values},
                  {"multicast-protection", sv.multicast_protection_values}};
  }
}

namespace wago::get_switch_config {

namespace {
  ::wago::libswitchconfig::status supported_values_to_json(nlohmann::json& j, const ::wago::libswitchconfig::supported_values& sv) {
    ::wago::libswitchconfig::status s{};
    if (sv.version == 1) {
      j = sv;
    } else {
      s = ::wago::libswitchconfig::status{::wago::libswitchconfig::status_code::UNKNOWN_CONFIG_VERSION, "Unknown config version"};
    }

    return s;
  }
}

::wago::libswitchconfig::status switch_data_provider::get_switch_config(
    ::wago::libswitchconfig::switch_config& config) const {
  return ::wago::libswitchconfig::read_switch_config(config);
}

::wago::libswitchconfig::status switch_data_provider::get_supported_values(nlohmann::json& j) const {
  ::wago::libswitchconfig::supported_values sv;
  
  ::wago::libswitchconfig::get_supported_values(sv);
  return supported_values_to_json(j, sv);
}
}  // namespace wago::get_switch_config