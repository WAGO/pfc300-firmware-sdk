// Copyright (c) 2024 WAGO GmbH & Co. KG
// SPDX-License-Identifier: MPL-2.0

#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "switch_config_output.hpp"
#include "switch_data_provider_mock.hpp"

namespace {

constexpr auto switch_config_json_str =
    R"({"broadcast-protection":{"ports":[{"enabled":true,"name":"X1"}],"unit":"pps","value":1000},"multicast-protection":{"ports":[{"enabled":true,"name":"X2"}],"unit":"pps","value":2000},"port-mirror":{"destination":"X2","enabled":false,"source":"X1"},"version":1})";
::std::vector<::std::string> split_lines(const ::std::string& data) {
  ::std::string line;
  ::std::vector<::std::string> lines;

  ::std::stringstream ss(data);
  while (getline(ss, line, '\n')) {
    lines.push_back(line);
  }

  return lines;
}

MATCHER_P(StrLengthIsLessOrEqualTo, max_length, "") {  // NOLINT clang-diagnostic-deprecated
  return arg.length() <= static_cast<size_t>(max_length);
}

}  // namespace

namespace nlohmann {
void PrintTo(const ::nlohmann::json& j, std::ostream* os);
void PrintTo(const ::nlohmann::json& j, std::ostream* os) {
  *os << j.dump();
}
}  // namespace nlohmann

namespace wago::get_switch_config {

using namespace wago::libswitchconfig;  // NOLINT google-build-using-namespace
using namespace ::testing;              // NOLINT google-build-using-namespace

TEST(config_to_json, print_json_config) {
  switch_data_provider_mock provider_mock;

  auto expected_json = ::nlohmann::json::parse(switch_config_json_str);

  // clang-format off
  switch_config config = {1, {{{true, "X1"}}, "pps", 1000}, {{{true, "X2"}}, "pps", 2000}, {false, "X1", "X2"}};

  EXPECT_CALL(provider_mock, get_switch_config(_))
    .WillOnce(DoAll(
      SetArgReferee<0>(config), 
      Return(status{})
    ));
  // clang-format on

  testing::internal::CaptureStdout();
  print_json_config(option_map{}, provider_mock);
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(::nlohmann::json::parse(output), expected_json);
}

TEST(config_to_json, format_json_for_backup_v1) {
  switch_data_provider_mock provider_mock;

  auto expected_json = ::nlohmann::json::parse(switch_config_json_str);

  // clang-format off
  switch_config config = {1, {{{true, "X1"}}, "pps", 1000}, {{{true, "X2"}}, "pps", 2000}, {false, "X1", "X2"}};

  EXPECT_CALL(provider_mock, get_switch_config(_))
    .WillOnce(DoAll(
      SetArgReferee<0>(config), 
      Return(status{})
    ));
  // clang-format on

  testing::internal::CaptureStdout();
  print_backup_content(option_map{}, provider_mock);
  std::string output = testing::internal::GetCapturedStdout();

  auto lines = split_lines(output);
  EXPECT_THAT(lines, Each(StartsWith("switch.data=")));
  EXPECT_THAT(lines, Each(StrLengthIsLessOrEqualTo(88)));

  ::boost::replace_all(output, "switch.data=", "");
  ::boost::replace_all(output, "\n", "");
  EXPECT_EQ(::nlohmann::json::parse(output), expected_json);
}

TEST(config_to_json, format_json_for_backup_with_targetversion) {
  switch_data_provider_mock provider_mock;

  auto expected_json = ::nlohmann::json::parse(switch_config_json_str);

  option_map options = {{action_option::targetversion, "V04.04.12"}};

  // clang-format off
  switch_config config = {1, {{{true, "X1"}}, "pps", 1000}, {{{true, "X2"}}, "pps", 2000}, {false, "X1", "X2"}};

  EXPECT_CALL(provider_mock, get_switch_config(_))
    .WillOnce(DoAll(
      SetArgReferee<0>(config), 
      Return(status{})
    ));
  // clang-format on

  testing::internal::CaptureStdout();
  print_backup_content(options, provider_mock);
  std::string output = testing::internal::GetCapturedStdout();

  auto lines = split_lines(output);
  EXPECT_THAT(lines, Each(StartsWith("switch.data=")));
  EXPECT_THAT(lines, Each(StrLengthIsLessOrEqualTo(88)));

  ::boost::replace_all(output, "switch.data=", "");
  ::boost::replace_all(output, "\n", "");
  EXPECT_EQ(::nlohmann::json::parse(output), expected_json);
}

TEST(config_to_json, execute_action_for_help) {
  parameter_list parameters = {action::help};
  option_map options;

  testing::internal::CaptureStdout();
  status s           = execute_action(parameters, options, switch_data_provider_mock{});
  std::string output = testing::internal::GetCapturedStdout();

  ASSERT_THAT(s, Eq(status{}));
  EXPECT_THAT(output, HasSubstr("-c|--config"));
  EXPECT_THAT(output, HasSubstr("-b|--backup"));
  EXPECT_THAT(output, HasSubstr("-s|--supported-values"));
  EXPECT_THAT(output, HasSubstr("-h|--help"));

  EXPECT_THAT(output, HasSubstr("-t|--backup-targetversion"));
  EXPECT_THAT(output, HasSubstr("-p|--pretty"));
}

TEST(config_to_json, execute_action_with_unsupported_parameter_count) {
  parameter_list parameters = {action::help, action::json_config};

  status s = execute_action(parameters, option_map{}, switch_data_provider_mock{});

  ASSERT_THAT(s.get_code(), Eq(status_code::WRONG_PARAMETER_PATTERN));
}

}  // namespace wago::get_switch_config