// Copyright (c) 2023 WAGO GmbH & Co. KG
// SPDX-License-Identifier: MPL-2.0

#include "switch_config_handler.hpp"

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "switch_config_api.hpp"

namespace wago::libswitchconfig {

::std::string bool_to_string(bool value);
::std::string bool_to_string(bool value) {
  return (value) ? "true" : "false";
}

void PrintTo(const port& p, std::ostream* os);
void PrintTo(const port& p, std::ostream* os) {
  *os << "(" << p.name << ": " << bool_to_string(p.enabled) << ")";
}

void PrintTo(const ::std::vector<port>& ports, std::ostream* os);
void PrintTo(const ::std::vector<port>& ports, std::ostream* os) {
  *os << "(";
    for(const auto& p : ports) {
      PrintTo(p, os);
    }
  *os << ")";
}

void PrintTo(const storm_protection& sp, std::ostream* os);
void PrintTo(const storm_protection& sp, std::ostream* os) {
  *os << "(";
  PrintTo(sp.ports, os);
  *os << ", " << sp.value << ", " << sp.unit << ")";
}

void PrintTo(const port_mirror& pm, std::ostream* os);
void PrintTo(const port_mirror& pm, std::ostream* os) {
  *os << "(" << bool_to_string(pm.enabled) << ", " << pm.source << ", " << pm.destination << ")";
}

void PrintTo(const switch_config& sc, std::ostream* os);
void PrintTo(const switch_config& sc, std::ostream* os) {
  *os << "(bc: ";
  PrintTo(sc.broadcast_protection, os);
  *os << ", mc: ";
  PrintTo(sc.multicast_protection, os);
  *os << ", pm: ";
  PrintTo(sc.port_mirroring, os);
  *os << ", version: " << ::std::to_string(static_cast<unsigned int>(sc.version)) << ")";
}

}  // namespace wago::libswitchconfig

namespace wago::config_switch {

using namespace wago::libswitchconfig;  // NOLINT google-build-using-namespace
using namespace ::testing;              // NOLINT google-build-using-namespace

const switch_config default_sc = {1,
                                  {{{true, "X1"}, {false, "X2"}}, "pps", 1500},
                                  {{{false, "X1"}, {true, "X2"}}, "pps", 1000},
                                  {false, "X1", "X2"}};

TEST(edit_parameters, edit_default_config) {
  auto json_string =
      R"({"broadcast-protection":{"ports":[{"enabled":false,"name":"X1"},{"enabled":true,"name":"X2"}],"unit":"pps","value":2500},"multicast-protection":{"ports":[{"enabled":true,"name":"X1"},{"enabled":false,"name":"X2"}],"unit":"pps","value":2000},"port-mirror":{"destination":"X1","enabled":true,"source":"X2"},"version":1})";

  switch_config config = default_sc;
  edit_parameters(json_string, config);

  storm_protection bp = {{{false, "X1"}, {true, "X2"}}, "pps", 2500};
  storm_protection mp = {{{true, "X1"}, {false, "X2"}}, "pps", 2000};
  port_mirror pm      = {true, "X2", "X1"};

  EXPECT_THAT(config.broadcast_protection, bp);
  EXPECT_THAT(config.multicast_protection, mp);
  EXPECT_THAT(config.port_mirroring, pm);
  EXPECT_THAT(config.version, 1);
}

TEST(edit_parameters, ignore_broadcast_protection_with_empty_ports) {
  auto json_string =
      R"({"broadcast-protection":{"ports":[],"unit":"pps","value":2500},"multicast-protection":{"ports":[{"enabled":true,"name":"X1"},{"enabled":false,"name":"X2"}],"unit":"pps","value":2000},"port-mirror":{"destination":"X1","enabled":true,"source":"X2"},"version":1})";

  switch_config config = default_sc;
  edit_parameters(json_string, config);

  storm_protection bp = {{{true, "X1"}, {false, "X2"}}, "pps", 2500};
  storm_protection mp = {{{true, "X1"}, {false, "X2"}}, "pps", 2000};
  port_mirror pm      = {true, "X2", "X1"};

  EXPECT_THAT(config.broadcast_protection, bp);
  EXPECT_THAT(config.multicast_protection, mp);
  EXPECT_THAT(config.port_mirroring, pm);
  EXPECT_THAT(config.version, 1);
}

TEST(edit_parameters, broadcast_protection_change_value) {
  auto json_string =
      R"({"broadcast-protection":{"value":2500},"multicast-protection":{"ports":[{"enabled":true,"name":"X1"},{"enabled":false,"name":"X2"}],"unit":"pps","value":2000},"port-mirror":{"destination":"X1","enabled":true,"source":"X2"},"version":1})";

  switch_config config = default_sc;
  edit_parameters(json_string, config);
  
  storm_protection bp = {{{true, "X1"}, {false, "X2"}}, "pps", 2500};
  EXPECT_THAT(config.broadcast_protection, bp);
}

TEST(edit_parameters, broadcast_protection_change_ports) {
  auto json_string =
      R"({"broadcast-protection":{"ports":[{"enabled":false,"name":"X1"},{"enabled":true,"name":"X2"}]},"multicast-protection":{"ports":[{"enabled":true,"name":"X1"},{"enabled":false,"name":"X2"}],"unit":"pps","value":2000},"port-mirror":{"destination":"X1","enabled":true,"source":"X2"},"version":1})";

  switch_config config = default_sc;
  edit_parameters(json_string, config);

  storm_protection bp = {{{false, "X1"}, {true, "X2"}}, "pps", 1500};
  EXPECT_THAT(config.broadcast_protection, bp);
}

TEST(edit_parameters, broadcast_protection_missing_port_name) {
  auto json_string =
      R"({"broadcast-protection":{"ports":[{"enabled":false},{"enabled":true}],"unit":"pps","value":2500},"multicast-protection":{"ports":[{"enabled":true,"name":"X1"},{"enabled":false,"name":"X2"}],"unit":"pps","value":2000},"port-mirror":{"destination":"X1","enabled":true,"source":"X2"},"version":1})";

  switch_config config = default_sc;
  EXPECT_THROW(edit_parameters(json_string, config), switch_config_exception);

  EXPECT_THAT(config, default_sc);
}

TEST(edit_parameters, port_mirror_enabled) {
  auto json_string =
      R"({"port-mirror":{"enabled":true},"broadcast-protection":{"ports":[{"enabled":false,"name":"X1"},{"enabled":true,"name":"X2"}],"unit":"pps","value":2500},"multicast-protection":{"ports":[{"enabled":true,"name":"X1"},{"enabled":false,"name":"X2"}],"unit":"pps","value":2000},"version":1})";

  switch_config config = default_sc;
  edit_parameters(json_string, config);

  storm_protection bp = {{{false, "X1"}, {true, "X2"}}, "pps", 2500};
  storm_protection mp = {{{true, "X1"}, {false, "X2"}}, "pps", 2000};
  port_mirror pm      = {true, "X1", "X2"};

  EXPECT_THAT(config.broadcast_protection, bp);
  EXPECT_THAT(config.multicast_protection, mp);
  EXPECT_THAT(config.port_mirroring, pm);
  EXPECT_THAT(config.version, 1);
}

TEST(edit_parameters, parameter_is_known) {
  parameter_map params;
  params["set"] = R"({"version": 1})";

  EXPECT_NO_THROW(check_that_parameters_are_known(params));
}

TEST(edit_parameters, throw_on_unknown_parameter) {
  parameter_map params;
  params["foobar"] = R"({"version": 1})";

  EXPECT_THROW(check_that_parameters_are_known(params), switch_config_exception);
}

TEST(edit_parameters, throw_for_newer_config_versions) {
  auto input = R"({"version": 2})";

  switch_config config{};
  EXPECT_THROW(edit_parameters(input, config), switch_config_exception);
}

TEST(edit_parameters, throw_for_field_data_type_mismatch) {
  auto input = R"({"version": "true"})";

  switch_config config{};
  EXPECT_THROW(edit_parameters(input, config), switch_config_exception);
}

TEST(edit_parameters, throw_on_invalid_json_parameter) {
  auto input = R"({"asfdasf": 2)";  // missing closing bracket

  switch_config config{};
  EXPECT_THROW(edit_parameters(input, config), switch_config_exception);
}

TEST(edit_parameters, throw_on_unknown_key) {
  auto input = R"({"asfdasf": 2})";

  switch_config config{};
  EXPECT_THROW(edit_parameters(input, config), switch_config_exception);
}

TEST(edit_parameters, throw_on_invalid_parameter_range) {
  auto input = R"({"version": -2})";

  switch_config config{};
  EXPECT_THROW(edit_parameters(input, config), switch_config_exception);
}

TEST(edit_parameters, throw_on_storm_protection_parameter_is_no_array) {
  auto input = R"({"broadcast-protection":{"ports":{"name":"X1","enabled": true}}})";

  switch_config config{};
  EXPECT_THROW(edit_parameters(input, config), switch_config_exception);
}

}  // namespace wago::config_switch