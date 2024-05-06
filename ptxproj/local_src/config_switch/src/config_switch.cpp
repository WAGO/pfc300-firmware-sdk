// Copyright (c) 2023 WAGO GmbH & Co. KG
// SPDX-License-Identifier: MPL-2.0

#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "switch_config_api.hpp"

//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <glib.h>
//#pragma GCC diagnostic pop

#include "switch_config_handler.hpp"

using wago::libswitchconfig::status;
using wago::libswitchconfig::switch_config;

namespace {

using namespace ::std::string_literals;

[[noreturn]] void show_help() {
  const ::std::string help_text = {R"(
      * Change the Network Switch configuration *,
      Usage: config_switch set=<json>
             config_switch -i | --init
             config_switch -u | --update
             config_switch -h | --help

      Example:
      set='{"broadcast-protection":{"ports":[{"enabled":true,"name":"X1"},{"enabled":true,"name":"X2"}],"unit":"pps","value":1000},"multicast-protection":{"ports":[{"enabled":false, "name":"X1"},{"enabled":false,"name":"X2"}],"unit":"pps","value":2000},"port-mirror":{"destination":"X1","enabled":false,"source":"X2"},"version":1}'

      broadcast-protection:  broadcast protection configuration
      multicast-protection:  multicast protection configuration
      port-mirror:           port mirroring configuration
      version:               version of the configuration parameter set

      parameter set of broadcast/multicast protection
      ports:                 array of ports (enabled, name)
      value:                 value of the broadcast/multicast protection
      unit:                  unit of the broadcast/multicast protection 

      parameter set of port mirroring
      enabled:               true | false
      source:                ethernet port, e.g. X1
      destination:           ethernet port, e.g. X2
      
      )"};

  ::std::cout << help_text << ::std::endl;
  ::std::exit(0);
}

void write_last_error(::std::string const &text) {
  ::std::ofstream last_error;
  last_error.open("/tmp/last_error.txt");
  if (last_error.good()) {
    last_error << text;
    last_error.flush();
    last_error.close();
  }
}

void clear_last_error() {
  write_last_error(::std::string{});
}

[[noreturn]] void exit_with_error(wago::config_switch::switch_config_error code, ::std::string const &text) {
  write_last_error(text);
  ::std::exit(static_cast<int>(code));
}

::std::string uri_unescape(const ::std::string &escaped_uri) {
  auto result_cstr =
      std::unique_ptr<char, decltype(std::free) *>(g_uri_unescape_string(escaped_uri.c_str(), ""), std::free);
  if (result_cstr != nullptr) {
    return std::string{result_cstr.get()};
  }
  return std::string{};
}

[[noreturn]] void init() {
  switch_config current_switch_config{};
  status s = wago::libswitchconfig::read_switch_config(current_switch_config);
  if (s.ok()) {
    s = wago::libswitchconfig::validate(current_switch_config);
  }

  if (!s.ok()) {
    switch_config default_switch_config{};
    s = wago::libswitchconfig::get_default_switch_config(default_switch_config);

    if (s.ok()) {
      s = wago::libswitchconfig::write_switch_config(default_switch_config);
    }
  }

  if (!s.ok()) {
    exit_with_error(wago::config_switch::switch_config_error::invalid_parameter, s.to_string());
  }

  exit(0);
}

[[noreturn]] void update() {
  switch_config current_switch_config{};
  status s = wago::libswitchconfig::read_switch_config(current_switch_config);

  if (s.ok()) {
    wago::libswitchconfig::apply_switch_config(current_switch_config);
  }

  if (!s.ok()) {
    exit_with_error(wago::config_switch::switch_config_error::invalid_parameter, s.to_string());
  }

  exit(0);
}

[[noreturn]] void set(::std::unordered_map<::std::string, ::std::string> parameters) {
  switch_config current_switch_config{};
  switch_config modified_switch_config{};
  status s = wago::libswitchconfig::read_switch_config(current_switch_config);
  if (!s.ok()) {
    exit_with_error(wago::config_switch::switch_config_error::invalid_parameter, s.to_string());
  }
  modified_switch_config = current_switch_config;

  try {
    wago::config_switch::check_that_parameters_are_known(parameters);
    wago::config_switch::edit_parameters(parameters["set"], modified_switch_config);
  } catch (wago::config_switch::switch_config_exception const &e) {
    exit_with_error(e.error(), e.what());
  } catch (::std::exception const &e) {
    exit_with_error(wago::config_switch::switch_config_error::invalid_parameter, e.what());
  } catch (...) {
    exit_with_error(wago::config_switch::switch_config_error::invalid_parameter, "Unknown exception occurred");
  }

  if (current_switch_config != modified_switch_config) {
    s = wago::libswitchconfig::write_switch_config(modified_switch_config);
    if (s.ok()) {
      s = wago::libswitchconfig::apply_switch_config(modified_switch_config);

      if (!s.ok()) {
        wago::libswitchconfig::write_switch_config(current_switch_config);
        wago::libswitchconfig::apply_switch_config(current_switch_config);
        exit_with_error(wago::config_switch::switch_config_error::invalid_parameter, s.to_string());
      }
    } else {
      exit_with_error(wago::config_switch::switch_config_error::invalid_parameter, s.to_string());
    }
  }

  exit(0);
}

}  // namespace

int main(int argc, const char *argv[]) {
  ::std::set<::std::string> help_args{"-h", "--help"};
  ::std::set<::std::string> init_args{"-i", "--init"};
  ::std::set<::std::string> update_args{"-u", "--update"};
  ::std::vector<::std::string> args(argv, argv + argc);
  ::std::unordered_map<::std::string, ::std::string> parameters{};

  clear_last_error();

  for (auto &arg : args) {
    if (help_args.count(arg) > 0) {
      show_help();
    } else if (init_args.count(arg) > 0) {
      init();
    } else if (update_args.count(arg) > 0) {
      update();
    } else if (auto pos = arg.find('='); pos != arg.npos) {
      auto key   = arg.substr(0, pos);
      auto value = uri_unescape(arg.substr(pos + 1));
      parameters.emplace(key, value);
      set(parameters);
    }
  }

  return 0;
}
