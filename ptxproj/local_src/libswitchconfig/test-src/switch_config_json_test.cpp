// Copyright (c) 2023 WAGO GmbH & Co. KG
// SPDX-License-Identifier: MPL-2.0

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <nlohmann/json.hpp>
#include <vector>

#include "switch_config_api.hpp"

namespace {
constexpr auto switch_config_json_str =
    R"({"broadcast-protection":{"ports":[{"enabled":true,"name":"X1"},{"enabled":false,"name":"X2"}],"unit":"pps","value":1500},"multicast-protection":{"ports":[{"enabled":false,"name":"X1"},{"enabled":true,"name":"X2"}],"unit":"pps","value":1000},"port-mirror":{"destination":"X2","enabled":false,"source":"X1"},"version":1})";

constexpr auto switch_config_incomplete_json_str =
    R"({"broadcast-protection":[{"port":"X1"}],"multicast-protection":[{"port":"X1","value":"1000"}],"port-mirror":{"destination":"X2","enabled":false,"source":"X1"},"version":1})";
}  // namespace

namespace wago::libswitchconfig {
namespace {

switch_config create_switch_config(const ::std::uint8_t version, const storm_protection& broadcast_protection,
                                   const storm_protection& multicast_protection, const port_mirror& port_mirroring) {
  switch_config config;

  config.version              = version;
  config.broadcast_protection = broadcast_protection;
  config.multicast_protection = multicast_protection;
  config.port_mirroring       = port_mirroring;

  return config;
}
}  // namespace

TEST(json_conversion, convert_switch_config_to_json) {
  storm_protection bcast  = {{{true, "X1"}, {false, "X2"}}, "pps", 1500};
  storm_protection mcast  = {{{false, "X1"}, {true, "X2"}}, "pps", 1000};
  port_mirror port_mirror = {false, "X1", "X2"};

  switch_config swconfig = create_switch_config(1, bcast, mcast, port_mirror);

  nlohmann::json j{};
  auto status = switch_config_to_json(j, swconfig);

  ASSERT_TRUE(status.ok());
  ASSERT_THAT(j.dump(), switch_config_json_str);
}

TEST(json_conversion, try_to_convert_switch_config_of_unknown_version) {
  storm_protection bcast  = {{{true, "X1"}, {false, "X2"}}, "pps", 1500};
  storm_protection mcast  = {{{false, "X1"}, {true, "X2"}}, "pps", 1000};
  port_mirror port_mirror = {false, "X1", "X2"};

  switch_config swconfig = create_switch_config(10, bcast, mcast, port_mirror);

  nlohmann::json j{};
  auto status = switch_config_to_json(j, swconfig);

  ASSERT_THAT(status.get_code(), status_code::UNKNOWN_CONFIG_VERSION);
}

TEST(json_conversion, convert_json_to_switch_config) {
  storm_protection bcast  = {{{true, "X1"}, {false, "X2"}}, "pps", 1500};
  storm_protection mcast  = {{{false, "X1"}, {true, "X2"}}, "pps", 1000};
  port_mirror port_mirror         = {false, "X1", "X2"};
  switch_config expected_swconfig = create_switch_config(1, {bcast}, {mcast}, port_mirror);

  switch_config swconfig;
  auto status = switch_config_from_json(swconfig, nlohmann::json::parse(switch_config_json_str));

  ASSERT_TRUE(status.ok());
  ASSERT_THAT(swconfig, expected_swconfig);
}

TEST(json_conversion, try_to_convert_incomplete_json_to_switch_config) {
  switch_config swconfig;
  auto status = switch_config_from_json(swconfig, nlohmann::json::parse(switch_config_incomplete_json_str));

  ASSERT_THAT(status.get_code(), status_code::JSON_PARSE_ERROR);
}

TEST(json_conversion, try_to_convert_json_with_unknown_version_to_switch_config) {
  switch_config swconfig;
  auto unknown_version_json_str =
      R"({"broadcast-protection":{"ports":[],"unit":"","value":1000},"multicast-protection":{"ports":[],"unit":"","value":2000},"port-mirror":{"destination":"X2","enabled":false,"source":"X1"},"version":11})";
  auto status = switch_config_from_json(swconfig, nlohmann::json::parse(unknown_version_json_str));

  ASSERT_THAT(status.get_code(), status_code::UNKNOWN_CONFIG_VERSION);
}

}  // namespace wago::libswitchconfig