// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TypesHelper.hpp"
#include "CollectionUtils.hpp"
#include <algorithm>
#include <boost/asio/ip/address.hpp>
#include <regex>
#include <sstream>
#include <tuple>

namespace netconf {

namespace {
bool IsZeroIP(const ::std::string &address) {
  static const ::std::string zeroIP = "0.0.0.0";
  return address == zeroIP;
}

}

bool IsEmptyOrZeroIP(const ::std::string &address) {
  return address.empty() || IsZeroIP(address);
}

::std::string IPSourceToString(IPSource value) {
  switch (value) {
    case IPSource::STATIC:
      return "static";
    case IPSource::DHCP:
      return "dhcp";
    case IPSource::BOOTP:
      return "bootp";
    case IPSource::NONE:
      return "none";
    default:
      return "unknown";
  }
}

IPSource StringToIPSource(const ::std::string &value) {
  if (value == "static") {
    return IPSource::STATIC;
  }
  if (value == "dhcp") {
    return IPSource::DHCP;
  }
  if (value == "bootp") {
    return IPSource::BOOTP;
  }
  if (value == "none") {
    return IPSource::NONE;
  }
  return IPSource::UNKNOWN;
}

bool IsEqual(IPConfigs &lhs, IPConfigs &rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  std::sort(lhs.begin(), lhs.end());
  std::sort(rhs.begin(), rhs.end());

  return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

void AddIPConfig(const IPConfig &ip_config, IPConfigs &ip_configs) {

  auto it = ::std::find_if(ip_configs.begin(), ip_configs.end(), [&ip_config](const IPConfig &arg) {
    return arg.interface_ == ip_config.interface_;
  });
  if (it != ip_configs.cend()) {
    *it.base() = ip_config;
  } else {
    ip_configs.push_back(ip_config);
  }

}

Interfaces GetBridgesOfBridgeConfig(const BridgeConfig &bridge_config) {
  Interfaces bridges;
  for (auto &bridge : bridge_config) {
    bridges.push_back(bridge.first);
  }
  return bridges;
}

bool ConfigContainsBridgeWithSpecificInterfaces(const BridgeConfig &bridge_config, const Interface &bridge,
                                                const Interfaces &interfaces) {

  bool contains = false;

  auto it = bridge_config.find(bridge);
  if (it != bridge_config.end()) {

    auto &bridge_interfaces = it->second;

    contains = true;
    for (auto &interface : interfaces) {
      if (IsNotIncluded(interface, bridge_interfaces)) {
        contains = false;
      }
    }

  }

  return contains;
}

/**
 * Removes all IP Configuration who's interface member is not found in Bridges selection.
 */
void IpConfigsIntersection(IPConfigs &ip_configs, const Interfaces &selection) {

  auto new_end = std::remove_if(ip_configs.begin(), ip_configs.end(), [&](auto &ip_config) {
    auto it = std::find_if(selection.begin(), selection.end(), [&](Interface itf){
      return ip_config.interface_ == itf.GetName();
    });
    return it == selection.end();
  });

  ip_configs.erase(new_end, ip_configs.end());
}

bool IsEqual(const BridgeConfig &a, const BridgeConfig &b) {
  return std::is_permutation(
      a.begin(),
      a.end(),
      b.begin(),
      b.end(),
      [](auto &a_, auto &b_) {
        return (a_.first == b_.first)
            && std::is_permutation(a_.second.begin(), a_.second.end(), b_.second.begin(), b_.second.end());
      });
}

IPConfig& ComplementNetmask(IPConfig &ip_config) {
  if ((IsEmptyOrZeroIP(ip_config.netmask_)) && (not IsEmptyOrZeroIP(ip_config.address_))) {
    try {
      auto ipv4 = boost::asio::ip::make_address_v4(ip_config.address_);
      auto maskv4 = boost::asio::ip::address_v4::netmask(ipv4);
      ip_config.netmask_ = Netmask{maskv4.to_string()};
    } catch (...) {
      /* nothing to do here */
    }
  }
  return ip_config;
}

IPConfigs& ComplementNetmasks(IPConfigs &ip_configs) {
  std::for_each(ip_configs.begin(), ip_configs.end(), &ComplementNetmask);
  return ip_configs;
}

::std::string IPConfigToString(const IPConfig &config) {
  ::std::stringstream ss;
  ss << config.interface_ << "," << IPSourceToString(config.source_) << "," << config.address_ << ","
     << config.netmask_;
  return ss.str();
}

void RemoveUnnecessaryIPParameter(IPConfigs &ip_configs) {
  for (auto &config : ip_configs) {
    if (config.source_ == IPSource::DHCP || config.source_ == IPSource::BOOTP || config.source_ == IPSource::EXTERNAL
        || config.source_ == IPSource::NONE) {
      config.address_ = ZeroIP;
      config.netmask_ = ZeroNetmask;
    } else if (config.source_ == IPSource::STATIC && config.address_ == ZeroIP) {
      config.netmask_ = ZeroNetmask;
    }
  }
}

std::optional<uint32_t> ExtractInterfaceIndex(const ::std::string &interface) {
  ::std::regex expr { "^([a-zA-Z]+)([0-9]+)$" };
  ::std::smatch what;
  if (::std::regex_search(interface, what, expr)) {
    return {::std::stoi(what[2])};
  }
  return ::std::nullopt;
}

Address IpAddressV4Increment(const Address &address, uint32_t increment) {
  try {
    auto ipv4 = boost::asio::ip::make_address_v4(address);
    auto ipv4_inc = boost::asio::ip::make_address_v4(ipv4.to_uint() + increment);
    return Address{ipv4_inc.to_string()};
  } catch (...) {
    /* nothing to do here */
  }
  return {};
}

IPConfigs::iterator GetIpConfigByInterface(IPConfigs &configs, const Interface &interface) {
  return ::std::find_if(configs.begin(), configs.end(), [&](const IPConfig &cfg) {
    return cfg.interface_ == interface.GetName();
  });
}

IPConfigs GetIpConfigsDifferenceByInterface(const IPConfigs &a, const IPConfigs &b) {
  IPConfigs diff;
  std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(diff),
                      [](const IPConfig &ia, const IPConfig &ib) {
                        return ia.interface_ < ib.interface_;
                      });
  return diff;
}

void SortByInterface(IPConfigs &a) {
  auto interface_less = [](const IPConfig &ia, const IPConfig &ib) {
    return ia.interface_ < ib.interface_;
  };
  std::sort(a.begin(), a.end(), interface_less);
}

bool hasValidIPSourceForDeviceType(const IPConfig &ip_config, const DeviceType &type) {
  if(type == DeviceType::Bridge) {
    return IPConfig::SourceIsAnyOf(ip_config, IPSource::NONE, IPSource::STATIC, IPSource::DHCP, IPSource::BOOTP, IPSource::EXTERNAL, IPSource::FIXIP);
  }

  if(type == DeviceType::Dummy) {
    return IPConfig::SourceIsAnyOf(ip_config, IPSource::NONE, IPSource::STATIC);
  }

  if(type == DeviceType::Vlan) {
    return IPConfig::SourceIsAnyOf(ip_config, IPSource::NONE, IPSource::STATIC, IPSource::DHCP);
  }

  return true;
}

} /* namespace netconf */
