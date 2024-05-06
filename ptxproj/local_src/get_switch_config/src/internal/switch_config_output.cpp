// Copyright (c) 2024 WAGO GmbH & Co. KG
// SPDX-License-Identifier: MPL-2.0

#include "switch_config_output.hpp"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <set>
#include <string>

using json = nlohmann::json;

namespace {

void show_help() {
  const ::std::string help_text = {R"(
      Usage: get_switch_config [--backup [--backup-targetversion=<fw-version>] | --pretty ]

      Print the switch configuration.

      -c|--config                              Print the network switch configuration.
      -b|--backup                              Output the configuration in backup file format.
      -s|--supported-values                    Output the supported values.
      -h|--help                                Print this help text.

      -t|--backup-targetversion=<fw-version>   Request a specific target firmware version, e.g. 04.05.00(27)
      -p|--pretty                              Output the configuration in json format that is more readable for humans.
      )"};

  ::std::cout << help_text << ::std::endl;
}

void print_json(const json& j, const bool pretty) {
  if (pretty) {
    ::std::cout << std::setw(4) << j;
  } else {
    ::std::cout << j;
  }
}

void print_backup_content_v1(const json& j) {
  const ::std::string key           = "switch.data";
  const int chars_per_line_         = 88;
  const size_t value_chars_per_line = chars_per_line_ - (key.length() + 1);

  std::stringstream ss;
  ::std::string data = j.dump();
  auto lines         = static_cast<uint32_t>(::std::floor(data.size() / value_chars_per_line));
  for (uint32_t line = 0; line < lines; line++) {
    ::std::string substring = data.substr(line * value_chars_per_line, value_chars_per_line);
    ss << key << '=' << substring << '\n';
  }

  auto remaining_chars = data.size() % value_chars_per_line;
  if (remaining_chars > 0) {
    ::std::string substring = data.substr(lines * value_chars_per_line, data.size());
    ss << key << '=' << substring << '\n';
  }

  ::std::cout << ss.str();
}

void write_last_error(::std::string const& text) {
  ::std::ofstream last_error;
  last_error.open("/tmp/last_error.txt");
  if (last_error.good()) {
    last_error << text;
    last_error.flush();
    last_error.close();
  }
}

}  // namespace

namespace wago::get_switch_config {

wago::libswitchconfig::status parse_commandline_args(const ::std::vector<::std::string>& args,
                                                     parameter_list& parameters, option_map& options) {
  wago::libswitchconfig::status s{};

  // actions
  ::std::set<::std::string> config_args{"-c", "--config"};
  ::std::set<::std::string> backup_args{"-b", "--backup"};
  ::std::set<::std::string> supported_values{"-s", "--supported-values"};
  ::std::set<::std::string> help_args{"-h", "--help"};
  // options
  ::std::set<::std::string> backup_targetversion_args{"-t=", "--backup-targetversion="};
  ::std::set<::std::string> pretty_args{"-p", "--pretty"};

  if (args.size() > 2) {
    return {wago::libswitchconfig::status_code::WRONG_PARAMETER_PATTERN, "Invalid parameter count."};
  }

  for (const auto& arg : args) {
    action action_arg;

    if (arg.rfind("-t=", 0) == 0 || arg.rfind("--backup-targetversion=", 0) == 0) {
      auto pos = arg.find('=');
      if (pos + 1 < arg.length()) {
        ::std::string version = arg.substr(pos + 1);
        if (is_valid_fw_version(version)) {
          options.emplace(action_option::targetversion, version);
          continue;
        }
        s = {wago::libswitchconfig::status_code::WRONG_PARAMETER_PATTERN, "Invalid targetversion value"};
      } else {
        s = {wago::libswitchconfig::status_code::WRONG_PARAMETER_PATTERN, "Missing targetversion value"};
      }
    } else if (config_args.count(arg) > 0) {
      action_arg = action::json_config;
    } else if (backup_args.count(arg) > 0) {
      action_arg = action::backup;
    } else if (supported_values.count(arg) > 0) {
      action_arg = action::supported_values;
    } else if (help_args.count(arg) > 0) {
      action_arg = action::help;
    } else if (pretty_args.count(arg) > 0) {
      options.emplace(action_option::pretty, "");
      continue;
    } else {
      s = {wago::libswitchconfig::status_code::WRONG_PARAMETER_PATTERN, "Unknown parameter"};
    }

    if (s.ok()) {
      if (::std::find(parameters.cbegin(), parameters.cend(), action_arg) == parameters.cend()) {
        parameters.emplace_back(action_arg);
      } else {
        s = {wago::libswitchconfig::status_code::WRONG_PARAMETER_PATTERN, "Found duplicate parameter"};
      }
    }

    if (!s.ok()) {
      break;
    }
  }

  if (parameters.empty()) {
    parameters.emplace_back(action::help);
  }

  return s;
}

libswitchconfig::status execute_action(const parameter_list& parameters, const option_map& options,
                                       const switch_data_provider_i& provider) {
  if (parameters.size() != 1) {
    return libswitchconfig::status{libswitchconfig::status_code::WRONG_PARAMETER_PATTERN, "Invalid parameter count."};
  }

  action_map action_funcs = {
      {action::json_config, [](const option_map& om, const switch_data_provider_i& p) { print_json_config(om, p); }},
      {action::backup, [](const option_map& om, const switch_data_provider_i& p) { print_backup_content(om, p); }},
      {action::supported_values, [](const option_map& om, const switch_data_provider_i& p) { print_supported_values(om, p); }},
      {action::help, [](const option_map&, const switch_data_provider_i&) { show_help(); }}};

  action selected_action = parameters.front();
  action_funcs.at(selected_action)(options, provider);

  return libswitchconfig::status{};
}

[[noreturn]] void exit_with_error(get_switch_config_error code, ::std::string const& text) {
  write_last_error(text);
  ::std::exit(static_cast<int>(code));
}

void clear_last_error() {
  write_last_error(::std::string{});
}

void print_json_config(const option_map& options, const switch_data_provider_i& provider) {
  json j;
  libswitchconfig::switch_config config;

  libswitchconfig::status s = provider.get_switch_config(config);
  if (s.ok()) {
    s = libswitchconfig::switch_config_to_json(j, config);
  }

  if (s.ok()) {
    print_json(j, options.count(action_option::pretty) > 0);
  } else {
    exit_with_error(get_switch_config_error::internal_error, s.to_string());
  }
}

void print_backup_content(const option_map& options, const switch_data_provider_i& provider) {
  json j;
  libswitchconfig::switch_config config;

  libswitchconfig::status s = provider.get_switch_config(config);
  if (s.ok()) {
    s = libswitchconfig::switch_config_to_json(j, config);
  }

  if (s.ok()) {
    ::std::string version = "04.05.00(27)";
    if (options.count(action_option::targetversion) > 0) {
      version = options.at(action_option::targetversion);
    }

    print_backup_content_v1(j);

  } else {
    exit_with_error(get_switch_config_error::internal_error, s.to_string());
  }
}

void print_supported_values(const option_map& options, const switch_data_provider_i& provider) {
  nlohmann::json supported_values_json;

  libswitchconfig::status s = provider.get_supported_values(supported_values_json);

  if (s.ok()) {
    print_json(supported_values_json, options.count(action_option::pretty) > 0);
  } else {
    exit_with_error(get_switch_config_error::internal_error, s.to_string());
  }
}

bool is_valid_fw_version(const ::std::string& version) {
  return std::regex_match(version, std::regex(R"(^[0-9]{2}\.[0-9]{2}\.[0-9]{2}\([0-9]{2}\))"));
}

}  // namespace wago::get_switch_config