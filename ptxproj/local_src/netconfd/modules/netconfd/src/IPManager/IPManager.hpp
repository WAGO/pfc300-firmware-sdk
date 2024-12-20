// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IIPManager.hpp"

#include <memory>
#include <set>

#include "IPValidator.hpp"
#include "IBridgeInformation.hpp"
#include "IDynamicIPClientAdministrator.hpp"
#include "DynamicIPClientFactory.hpp"
#include "FileEditor.hpp"
#include "IDeviceTypeLabel.hpp"
#include "IDipSwitch.hpp"
#include "IEventManager.hpp"
#include "IIPLinks.hpp"
#include "IIPMonitor.hpp"
#include "INetDevManager.hpp"
#include "IPController.hpp"
#include "IPLink.hpp"
#include "IPersistenceProvider.hpp"
#include "IIPInformation.hpp"
#include "GratuitousArp.hpp"
#include "INetDevEvents.hpp"
#include "IInterfaceInformation.hpp"
#include "IHostnameManager.hpp"

namespace netconf {

class IPManager : public IIPManager, public INetDevEvents, public IIPLinks, public IIPEvent, public IIPInformation {
 public:

  static constexpr DeviceType persistedDevices = DeviceType::Bridge;

  IPManager(IEventManager &event_manager, IPersistenceProvider &persistence_provider, INetDevManager &netdev_manager,
            IDipSwitch &ip_dip_switch, IInterfaceInformation &interface_information,
            IDynamicIPClientAdministrator &dyn_ip_client, IIPController &ip_controller,
            ::std::shared_ptr<IIPMonitor> &ip_monitor, IHostnameManager &hostname_manager);
  ~IPManager() override = default;

  IPManager(const IPManager&) = delete;
  IPManager& operator=(const IPManager&) = delete;
  IPManager(const IPManager&&) = delete;
  IPManager& operator=(const IPManager&&) = delete;

  Status ApplyTempFixIpConfiguration() override;
  Status ApplyIpConfiguration(const IPConfigs &config) override;
  Status ApplyDipConfiguration(const DipSwitchIpConfig &dip_switch_ip_config) override;
  Status ApplyIpConfiguration(const IPConfigs &ip_configs, const DipSwitchIpConfig &dip_switch_ip_config) override;

  Status ValidateIPConfigs(const IPConfigs &configs, const Interfaces &interfaces) const override;

  IPConfigs GetIPConfigs() const override;
  IPConfigs GetCurrentIPConfigs() const override;

  void OnNetDevCreated(NetDevPtr netdev) override;
  void OnNetDevRemoved(NetDevPtr netdev) override;
  void OnBridgePortsChange(NetDevPtr netdev) override;
  void OnLinkChange(NetDevPtr netdev) override;
  void OnAddressChange(ChangeType change_type, int index, Address address,
                       Netmask netmask) override;
  void OnDynamicIPEvent(const Interface &interface, DynamicIPEventAction action) override;
  void OnHostnameChanged() override;

 private:
  Status Configure(const IPConfigs &config);
  ::std::shared_ptr<IPLink> CreateOrGet(const Interface &interface) override;

  IPLinkPtr GetIPLinkByInterface(const Interface &interface) const;
  Status ValidateIPConfigIsApplicableToSystem(const IPConfigs &configs) const;

  Status Apply(const IPConfigs &config);
  Status Persist(const IPConfigs &config);
  bool HasToBePersisted(const IPConfig &config) const;

  Status ConfigureIP(const IPLinkPtr &link) const;
  void ConfigureDynamicIPClient(IPLinkPtr &link);

  IEventManager &event_manager_;
  IPersistenceProvider &persistence_provider_;
  const IDipSwitch &ip_dip_switch_;
  INetDevManager &netdev_manager_;
  IInterfaceInformation &interface_information_;
  IDynamicIPClientAdministrator &dyn_ip_client_admin_;
  IIPController &ip_controller_;
  ::std::shared_ptr<IIPMonitor> ip_monitor_;
  IHostnameManager &hostname_manager_;

  FileEditor file_editor_;
  GratuitousArp gratuitous_arp_;
  IPLinks ip_links_;

};

} /* namespace netconf */
