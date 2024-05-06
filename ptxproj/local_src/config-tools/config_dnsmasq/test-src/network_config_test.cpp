//------------------------------------------------------------------------------
// Copyright (c) 2020-2024 WAGO GmbH & Co. KG
//
// PROPRIETARY RIGHTS are involved in the subject matter of this material. All
// manufacturing, reproduction, use and sales rights pertaining to this
// subject matter are governed by the license agreement. The recipient of this
// software implicitly accepts the terms of the license.
//------------------------------------------------------------------------------

#include "network_config.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <BaseTypes.hpp>
#include <BridgeConfig.hpp>
#include <IPConfig.hpp>
#include <InterfaceConfig.hpp>
#include <iostream>
#include <ostream>
#include <string>

#include "gmock/gmock-matchers.h"
#include "port_data.hpp"

namespace {

constexpr auto default_ip_config = R"({
    "br0": {
      "bcast": "0.0.0.0",
      "ipaddr": "0.0.0.0",
      "netmask": "0.0.0.0",
      "source": "dhcp"
    },
    "br1": {
      "bcast": "192.168.20.255",
      "ipaddr": "192.168.20.60",
      "netmask": "255.255.255.0",
      "source": "static"
    }})";

constexpr auto default_interface_statuses = R"([
    {
        "device": "X1",
        "duplex": "full",
        "link": "up",
        "mac": "00:30:DE:AD:BE:EF",
        "maclearning": "on",
        "speed": 1000,
        "state": "up"
    },
    {
        "device": "X2",
        "duplex": "full",
        "link": "up",
        "mac": "00:30:BA:DE:AF:FE",
        "maclearning": "on",
        "speed": 100,
        "state": "up"
    }
])";

void addEthernetPort(napi::InterfaceConfigs& interface_configs, const ::std::string& name) {
  interface_configs.AddInterfaceConfig(netconf::InterfaceBase::DefaultConfig(netconf::Interface::CreateEthernet(name)));
}

void addPortData(::std::list<configdnsmasq::port_data>& port_data_list, const ::std::list<::std::string>& port_names,
                 const napi::BridgeConfig& bridge_config, const napi::IPConfigs& ip_configs,
                 const napi::InterfaceConfigs& interface_configs,
                 const netconf::InterfaceStatuses& interface_statuses) {
  for (const auto& name : port_names) {
    configdnsmasq::port_data pd;
    pd.port_name_ = name;
    pd.setState(name, bridge_config, interface_configs);
    pd.setLinkState(name, bridge_config, interface_statuses);
    auto ip_config = *ip_configs.GetIPConfig(name);
    pd.setType(ip_config);
    pd.setIpAddress(ip_config.address_);
    pd.setNetmask(ip_config.netmask_);

    port_data_list.push_back(pd);
  }
}

}  // namespace

namespace configdnsmasq {
inline bool operator==(const port_data& lhs, const port_data& rhs) {
  return lhs.port_name_ == rhs.port_name_ && lhs.ip_addr_ == rhs.ip_addr_ && lhs.netmask_ == rhs.netmask_ &&
         lhs.ip_addr_bin_ == rhs.ip_addr_bin_ && lhs.netmask_bin_ == rhs.netmask_bin_ &&
         lhs.link_state_ == rhs.link_state_ && lhs.state_ == rhs.state_ && lhs.type_ == rhs.type_;
}
}  // namespace configdnsmasq

TEST(test_network_config, generate_legal_ports) {
  napi::InterfaceConfigs interface_configs;
  addEthernetPort(interface_configs, "ethX1");
  addEthernetPort(interface_configs, "ethX2");
  addEthernetPort(interface_configs, "ethX15");

  ::std::vector<::std::string> expected_legal_ports = {"br0", "br1", "br2"};

  ::std::vector<::std::string> legal_ports;
  configdnsmasq::generate_legal_ports(interface_configs, legal_ports);

  ASSERT_THAT(legal_ports, testing::SizeIs(3));
  ASSERT_THAT(legal_ports, testing::ContainerEq(expected_legal_ports));
}

TEST(test_network_config, generate_legal_ports_relying_on_ethX_ports_only) {
  napi::InterfaceConfigs interface_configs;
  addEthernetPort(interface_configs, "ethX1");
  addEthernetPort(interface_configs, "ethY1");
  addEthernetPort(interface_configs, "ethX2");
  addEthernetPort(interface_configs, "usb0");
  addEthernetPort(interface_configs, "ethX15");

  ::std::vector<::std::string> expected_legal_ports = {"br0", "br1", "br2"};

  ::std::vector<::std::string> legal_ports;
  configdnsmasq::generate_legal_ports(interface_configs, legal_ports);

  ASSERT_THAT(legal_ports, testing::SizeIs(3));
  ASSERT_THAT(legal_ports, testing::ContainerEq(expected_legal_ports));
}

TEST(test_network_config, parse_config_parameter) {
  napi::BridgeConfig bridge_config;
  napi::IPConfigs ip_configs;
  napi::InterfaceConfigs interface_configs;
  netconf::InterfaceStatuses interface_statuses;
  configdnsmasq::ip_configuration ipConfig;
  ::std::list<configdnsmasq::port_data> expected_port_data_list;

  ::std::list<::std::string> port_names  = {"br0", "br1"};
  ::std::vector<std::string> legal_ports = {"br0", "br1"};
  bool debugmode                         = true;  // Do not read hostname from system.
  napi::MakeBridgeConfig(R"({"br0": ["X1"], "br1": ["X2"]})", bridge_config);
  napi::MakeIPConfigs(default_ip_config, ip_configs);
  addEthernetPort(interface_configs, "ethX1");
  addEthernetPort(interface_configs, "ethX2");
  napi::MakeInterfaceStatuses(default_interface_statuses, interface_statuses);
  addPortData(expected_port_data_list, port_names, bridge_config, ip_configs, interface_configs, interface_statuses);

  configdnsmasq::parse_config_parameter(ipConfig, legal_ports, debugmode, bridge_config, ip_configs, interface_configs,
                                        interface_statuses);

  ASSERT_THAT(ipConfig.host_name, testing::StrEq("debughostname"));
  ASSERT_THAT(ipConfig.port_name_list, testing::ContainerEq(port_names));
  ASSERT_THAT(ipConfig.port_data_list, testing::ContainerEq(expected_port_data_list));
}
