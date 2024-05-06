// Copyright (c) 2023 WAGO GmbH & Co. KG
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "switch_config_api.hpp"

namespace wago::libswitchconfig {

switch_config convert_to_system_port_names(const switch_config& config);
bool validate(const ::std::vector<port>& ports, const ::std::vector<::std::string>& port_names,
              ::std::stringstream& message);
bool validate(const storm_protection& sp, const ::std::vector<::std::string>& port_names, ::std::stringstream& message);
bool validate(const port_mirror& pm, const ::std::vector<::std::string>& port_names, ::std::stringstream& message);

}  // namespace wago::libswitchconfig