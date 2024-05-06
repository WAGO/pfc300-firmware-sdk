// Copyright (c) 2023 WAGO GmbH & Co. KG
// SPDX-License-Identifier: MPL-2.0

#include "libswitchconfig.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "alphanum.hpp"
#include "file.hpp"
#include "iptool.hpp"
#include "switch_config_api.hpp"
#include "tc.hpp"

using json = nlohmann::json;

namespace wago::libswitchconfig {

namespace {

constexpr auto broadcast_mac                  = "ff:ff:ff:ff:ff:ff";
constexpr auto multicast_mac                  = "01:00:00:00:00:00/01:00:00:00:00:00";
constexpr auto switch_config_persistence_path = "/etc/specific/switchconfig.json";

enum class switch_type { micrel, ti };

bool contains_port(const ::std::vector<::std::string>& ports, const ::std::string& name) {
  auto it = ::std::find(ports.cbegin(), ports.cend(), name);
  return (it != ports.cend());
}

::std::string get_storm_protection_unit(const switch_type& sw_type) {
  switch (sw_type) {
    case switch_type::micrel:
      return "%";
      break;

    case switch_type::ti:
      return "pps";
      break;

    default:
      return "";
      break;
  }
}

status add_qdisc_clsact(const ::std::string& port) {
  nlohmann::json qdiscs;
  auto status = tc_show_qdisc(port, qdiscs);

  if (!status.ok()) {
    return status;
  }

  if (!tc_has_qdisc_clsact(qdiscs)) {
    return tc_add_qdisc_clsact(port);
  }

  return {};
}

status delete_qdisc_clsact(const ::std::string& port) {
  nlohmann::json qdiscs;
  auto status = tc_show_qdisc(port, qdiscs);

  if (!status.ok()) {
    return status;
  }

  if (tc_has_qdisc_clsact(qdiscs)) {
    return tc_delete_qdisc_clsact(port);
  }

  return {};
}

status add_qdisc_clsact(const ::std::vector<::std::string> ports) {
  for (auto const& port : ports) {
    auto status = add_qdisc_clsact(port);

    if (!status.ok()) {
      return status;
    }
  }
  return {};
}

status apply_protection(const port& p, const ::std::string& unit, const ::std::string& value,
                        const ::std::string& dst_mac) {
  nlohmann::json ingress_filters;

  auto status = tc_show_ingress_filter(p.name, ingress_filters);
  if (status.ok()) {
    auto existing_filter_ref = get_ingress_ratelimit_filter_ref(ingress_filters, dst_mac);
    if (p.enabled) {
      status = existing_filter_ref
                   ? tc_change_ingress_rate_filter(existing_filter_ref.value(), p.name, unit, value, dst_mac)
                   : tc_add_ingress_rate_filter(p.name, unit, value, dst_mac);
    } else {
      if (existing_filter_ref) {
        status = tc_delete_ingress_filter(existing_filter_ref.value(), p.name);
      }
    }
  }

  return status;
}

status apply_protection(const storm_protection& protection, const ::std::string& dst_mac,
                        const ::std::vector<::std::string>& ignored_ports) {
  for (auto const& p : protection.ports) {
    if (not contains_port(ignored_ports, p.name)) {
      auto s = apply_protection(p, protection.unit, ::std::to_string(static_cast<size_t>(protection.value)), dst_mac);

      if (!s.ok()) {
        return s;
      }
    }
  }
  return {};
}

status remove_port_mirror(const ::std::string& port_name) {
  nlohmann::json filter;
  auto status = tc_show_ingress_filter(port_name, filter);
  if (status.ok()) {
    auto filter_ref = get_mirror_filter_ref(filter);
    if (filter_ref) {
      status = tc_delete_ingress_filter(filter_ref.value(), port_name);
      if (!status.ok()) {
        return status;
      }
    }
  }

  status = tc_show_egress_filter(port_name, filter);
  if (status.ok()) {
    auto filter_ref = get_mirror_filter_ref(filter);
    if (filter_ref) {
      return tc_delete_egress_filter(filter_ref.value(), port_name);
    }
  }

  return {};
}

status apply_port_mirroring(const port_mirror port_mirror) {
  auto status = tc_add_mirror(port_mirror.source, port_mirror.destination, "egress");
  if (!status.ok()) {
    return status;
  }

  return tc_add_mirror(port_mirror.source, port_mirror.destination, "ingress");
}

status apply_port_mirroring(const port_mirror& port_mirror, const ::std::vector<system_port>& system_ports,
                            const ::std::vector<::std::string>& ignored_ports) {
  status status{};
  for (auto const& p : system_ports) {
    if (!port_mirror.enabled || contains_port(ignored_ports, p.name)) {
      status = remove_port_mirror(p.name);
      if (!status.ok()) {
        return status;
      }
    }
  }

  if (port_mirror.enabled && not contains_port(ignored_ports, port_mirror.source) &&
      not contains_port(ignored_ports, port_mirror.destination)) {
    return apply_port_mirroring(port_mirror);
  }

  return {};
}

::std::string to_system_name(const ::std::string& port_name) {
  if (port_name.rfind("ethX", 0) == 0) {
    return port_name;
  }
  return "eth" + port_name;
}

storm_protection convert_to_system_port_names(const storm_protection& protection) {
  storm_protection sp;
  sp.unit  = protection.unit;
  sp.value = protection.value;

  for (auto& p : protection.ports) {
    port tmp;
    tmp.enabled = p.enabled;
    tmp.name    = to_system_name(p.name);
    sp.ports.push_back(tmp);
  }

  return sp;
}

port_mirror convert_to_system_port_names(const port_mirror& pm) {
  port_mirror converted_pm;
  converted_pm.enabled     = pm.enabled;
  converted_pm.source      = to_system_name(pm.source);
  converted_pm.destination = to_system_name(pm.destination);
  return converted_pm;
}

bool is_valid_port_name(const ::std::string& name, const ::std::vector<::std::string>& ports) {
  // TODO (Team): Request ports from netconfd.
  bool is_valid = false;
  for (const auto& p : ports) {
    if (name == p) {
      is_valid = true;
    }
  }
  return is_valid;
}

::std::string get_port_label(const ::std::string& system_name) {
  return system_name.substr(3);
}

void set_device_specific_storm_protection(storm_protection& sp, bool port_default_state,
                                          const ::std::map<switch_type, ::std::uint16_t>& default_values,
                                          const ::std::vector<system_port>& system_ports) {
  // TODO (Team): add get_switch_type()
  switch_type st = switch_type::ti;

  auto ports = system_ports;
  for (const auto& p : ports) {
    sp.ports.push_back({port_default_state, get_port_label(p.name)});
  }

  sp.unit  = get_storm_protection_unit(st);
  sp.value = default_values.at(st);
}

::std::vector<::std::string> subtract(const ::std::vector<::std::string>& all,
                                      const ::std::vector<::std::string>& part) {
  ::std::vector<::std::string> difference;

  for (const auto& p : all) {
    auto it = std::find(part.cbegin(), part.cend(), p);
    if (it == part.end()) {
      difference.push_back(p);
    }
  }
  return difference;
}

::std::vector<::std::string> get_ignored_ports(const ::std::vector<system_port>& system_ports) {
  ::std::vector<::std::string> ports;

  for (const auto& p : system_ports) {
    if (p.state != "UP") {
      ports.emplace_back(p.name);
    }
  }

  return ports;
}

::std::vector<::std::string> get_filtered_ports(const switch_config& config,
                                                const ::std::vector<::std::string>& ignored_ports) {
  ::std::vector<::std::string> ports;
  ::std::set<::std::string> filtered_ports;

  for (const auto& p : config.broadcast_protection.ports) {
    if (p.enabled) {
      filtered_ports.insert(p.name);
    }
  }

  for (const auto& p : config.multicast_protection.ports) {
    if (p.enabled) {
      filtered_ports.insert(p.name);
    }
  }

  if (config.port_mirroring.enabled) {
    filtered_ports.insert(config.port_mirroring.source);
  }

  ::std::copy_if(filtered_ports.cbegin(), filtered_ports.cend(), ::std::back_inserter(ports),
                 [&](auto port_name) { return not contains_port(ignored_ports, port_name); });

  return ports;
}

::std::vector<::std::string> get_unfiltered_ports(const ::std::vector<system_port>& system_ports,
                                                  const ::std::vector<::std::string>& filtered_ports,
                                                  const ::std::vector<::std::string>& ignored_ports) {
  ::std::vector<::std::string> port_names;
  ::std::transform(system_ports.begin(), system_ports.end(), ::std::back_inserter(port_names),
                   [](const system_port& sp) { return sp.name; });

  ::std::vector<::std::string> unfiltered_ports = subtract(port_names, filtered_ports);

  unfiltered_ports.erase(std::remove_if(unfiltered_ports.begin(), unfiltered_ports.end(),
                                        [&](auto p) { return contains_port(ignored_ports, p); }),
                         unfiltered_ports.end());

  return unfiltered_ports;
}

void sort_storm_protection_ports(storm_protection& protection) {
  // clang-format off
  auto natural_less = [](const port& lhs, const port& rhs) {
     return doj::alphanum_comp(lhs.name, rhs.name) < 0;
  };
  
  ::std::sort(protection.ports.begin(), protection.ports.end(), natural_less);
  // clang-format on
}

}  // namespace

switch_config convert_to_system_port_names(const switch_config& config) {
  switch_config converted_sc;
  converted_sc.version              = config.version;
  converted_sc.broadcast_protection = convert_to_system_port_names(config.broadcast_protection);
  converted_sc.multicast_protection = convert_to_system_port_names(config.multicast_protection);
  converted_sc.port_mirroring       = convert_to_system_port_names(config.port_mirroring);
  return converted_sc;
}

bool validate(const ::std::vector<port>& ports, const ::std::vector<::std::string>& port_names,
              ::std::stringstream& message) {
  bool is_valid = true;

  for (const auto& p : ports) {
    if (!is_valid_port_name(p.name, port_names)) {
      message << "Storm-protection: Invalid port name: " << p.name << ::std::endl;
      is_valid = false;
    }
  }

  return is_valid;
}

bool validate(const storm_protection& sp, const ::std::vector<::std::string>& port_names,
              ::std::stringstream& message) {
  bool is_valid = true;

  // TODO (Team): add get_switch_type()
  switch_type st = switch_type::ti;

  is_valid = validate(sp.ports, port_names, message);

  ::std::map<switch_type, uint16_t> storm_protection_min_values = {{switch_type::micrel, 0}, {switch_type::ti, 1000}};
  ::std::map<switch_type, uint16_t> storm_protection_max_values = {{switch_type::micrel, 100},
                                                                   {switch_type::ti, 10000}};

  try {
    if (sp.value < storm_protection_min_values.at(st)) {
      message << "Storm-protection: value falls below the minimum limit: " << storm_protection_min_values.at(st)
              << ::std::endl;
      is_valid = false;
    } else if (sp.value > storm_protection_max_values.at(st)) {
      message << "Storm-protection: value exceeds the maximum limit: " << storm_protection_max_values.at(st)
              << ::std::endl;
      is_valid = false;
    } else if (sp.value % 1000 != 0) {
      message << "Storm-protection: value has to be a multiple of 1000" << ::std::endl;
      is_valid = false;
    }
  } catch (const ::std::exception& e) {
    message << "Storm-protection: value must be between " << storm_protection_min_values.at(st) << " and "
            << storm_protection_max_values.at(st) << ::std::endl;
    is_valid = false;
  }

  if (sp.unit != get_storm_protection_unit(st)) {
    message << "Storm-protection: Invalid unit. The unit " << get_storm_protection_unit(st) << " will be supported."
            << ::std::endl;
    is_valid = false;
  }

  return is_valid;
}

bool validate(const port_mirror& pm, const ::std::vector<::std::string>& port_names, ::std::stringstream& message) {
  bool is_valid = true;

  if (!is_valid_port_name(pm.source, port_names)) {
    message << "Port-Mirroring: Invalid source port name: " << pm.source << ::std::endl;
    is_valid = false;
  }
  if (!is_valid_port_name(pm.destination, port_names)) {
    message << "Port-Mirroring: Invalid destination port name: " << pm.destination << ::std::endl;
    is_valid = false;
  }
  if (is_valid && pm.source == pm.destination) {
    message << "Port-Mirroring: Source and destination must not be the same." << ::std::endl;
    is_valid = false;
  }

  return is_valid;
}

status validate(const switch_config& config) {
  status s;

  ::std::vector<system_port> system_ports;

  s = get_system_ports(system_ports);

  if (s.ok()) {
    ::std::stringstream error_message;
    ::std::vector<::std::string> port_names;
    ::std::transform(system_ports.begin(), system_ports.end(), ::std::back_inserter(port_names),
                     [](const system_port& sp) { return get_port_label(sp.name); });

    if (!validate(config.broadcast_protection, port_names, error_message) ||
        !validate(config.multicast_protection, port_names, error_message) ||
        !validate(config.port_mirroring, port_names, error_message)) {
      s = status(status_code::VALIDATION_ERROR, error_message.str());
    }
  }

  return s;
}

void to_json(json& j, const port& p);
void to_json(json& j, const port& p) {
  j = json{{"enabled", p.enabled}, {"name", p.name}};
}

void to_json(json& j, const storm_protection& protection);
void to_json(json& j, const storm_protection& protection) {
  j = json{{"ports", protection.ports}, {"unit", protection.unit}, {"value", protection.value}};
}

void to_json(json& j, const port_mirror& mirror);
void to_json(json& j, const port_mirror& mirror) {
  j = json{{"enabled", mirror.enabled}, {"source", mirror.source}, {"destination", mirror.destination}};
}

void to_json(json& j, const switch_config& config);
void to_json(json& j, const switch_config& config) {
  j = json{{"version", config.version},
           {"broadcast-protection", config.broadcast_protection},
           {"multicast-protection", config.multicast_protection},
           {"port-mirror", config.port_mirroring}};
}

void from_json(const json& j, port& p);
void from_json(const json& j, port& p) {
  j.at("enabled").get_to(p.enabled);
  j.at("name").get_to(p.name);
}

void from_json(const json& j, storm_protection& protection);
void from_json(const json& j, storm_protection& protection) {
  j.at("ports").get_to(protection.ports);
  j.at("unit").get_to(protection.unit);
  j.at("value").get_to(protection.value);
}

void from_json(const json& j, port_mirror& mirror);
void from_json(const json& j, port_mirror& mirror) {
  j.at("enabled").get_to(mirror.enabled);
  j.at("source").get_to(mirror.source);
  j.at("destination").get_to(mirror.destination);
}

void from_json(const json& j, switch_config& config);
void from_json(const json& j, switch_config& config) {
  j.at("version").get_to(config.version);
  j.at("broadcast-protection").get_to(config.broadcast_protection);
  j.at("multicast-protection").get_to(config.multicast_protection);
  j.at("port-mirror").get_to(config.port_mirroring);
}

status switch_config_to_json(json& j, const switch_config& config) {
  status s{};
  if (config.version == 1) {
    j = config;
  } else {
    s = status{status_code::UNKNOWN_CONFIG_VERSION, "Unknown config version"};
  }

  return s;
}

status switch_config_from_json(switch_config& config, const json& j) {
  status s{};

  try {
    auto tmp = j.template get<switch_config>();

    if (tmp.version == 1) {
      config = tmp;
    } else {
      s = status{status_code::UNKNOWN_CONFIG_VERSION, "Unknown config version"};
    }
  } catch (const ::std::exception& e) {
    s = status{status_code::JSON_PARSE_ERROR, "Json parse error: " + ::std::string(e.what())};
  }

  return s;
}

status read_switch_config(switch_config& config) {
  status s{};
  ::std::stringstream ss;

  s = read(ss, switch_config_persistence_path);
  if (s.ok()) {
    try {
      json j = json::parse(ss);
      s      = switch_config_from_json(config, j);
    } catch (const ::std::exception& e) {
      s = {status_code::JSON_PARSE_ERROR, "Failed to parse json: " + ::std::string(e.what())};
    }
  }

  return s;
}

status write_switch_config(const switch_config& config) {
  status s{};

  s = validate(config);

  if (s.ok()) {
    auto sorted_config = config;
    sort_storm_protection_ports(sorted_config.broadcast_protection);
    sort_storm_protection_ports(sorted_config.multicast_protection);

    json j = sorted_config;
    std::stringstream ss;
    ss << std::setw(4) << j;
    s = write(ss, switch_config_persistence_path);
  }

  return s;
}

status apply_switch_config(const switch_config& config) {
  status status;
  ::std::vector<system_port> system_ports;

  status = get_system_ports(system_ports);

  if (status.ok()) {
    auto sys_name_config = convert_to_system_port_names(config);
    // We ignore ports that are not in state up.
    auto ignored_ports    = get_ignored_ports(system_ports);
    auto filtered_ports   = get_filtered_ports(sys_name_config, ignored_ports);
    auto unfiltered_ports = get_unfiltered_ports(system_ports, filtered_ports, ignored_ports);

    for (const auto& p : unfiltered_ports) {
      delete_qdisc_clsact(p);
    }

    if (!filtered_ports.empty()) {
      status = add_qdisc_clsact(filtered_ports);
      if (!status.ok()) {
        return status;
      }

      status = apply_protection(sys_name_config.broadcast_protection, broadcast_mac, ignored_ports);
      if (!status.ok()) {
        return status;
      }

      status = apply_protection(sys_name_config.multicast_protection, multicast_mac, ignored_ports);
      if (!status.ok()) {
        return status;
      }

      status = apply_port_mirroring(sys_name_config.port_mirroring, system_ports, ignored_ports);
    }
  }
  return status;
}

status get_default_switch_config(switch_config& config) {
  status status{};
  ::std::vector<system_port> system_ports;

  status = get_system_ports(system_ports);

  if (status.ok()) {
    config.version        = 1;
    config.port_mirroring = {false, "X1", "X2"};

    // TODO (Team): add get_switch_type function
    ::std::map<switch_type, ::std::uint16_t> broadcast_protection_default_values = {{switch_type::micrel, 1},
                                                                                  {switch_type::ti, 1000}};
    set_device_specific_storm_protection(config.broadcast_protection, true, broadcast_protection_default_values,
                                         system_ports);

    ::std::map<switch_type, ::std::uint16_t> multicast_protection_default_values = {{switch_type::micrel, 1},
                                                                                  {switch_type::ti, 2000}};
    set_device_specific_storm_protection(config.multicast_protection, false, multicast_protection_default_values,
                                         system_ports);
  }

  return status;
}

status get_supported_values(supported_values& sv){
  status status{};

  sv.version = 1;

  for (::std::uint16_t i=1000; i<=10000; i+=1000) {
    sv.broadcast_protection_values.push_back(i);
    sv.multicast_protection_values.push_back(i);
  }

  return status;
}

}  // namespace wago::libswitchconfig
