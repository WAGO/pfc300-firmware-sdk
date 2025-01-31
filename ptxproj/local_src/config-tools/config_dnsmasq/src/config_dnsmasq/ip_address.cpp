//------------------------------------------------------------------------------
/// Copyright (c) 2020-2024 WAGO GmbH & Co. KG
///
/// PROPRIETARY RIGHTS of WAGO GmbH & Co. KG are involved in
/// the subject matter of this material. All manufacturing, reproduction,
/// use, and sales rights pertaining to this subject matter are governed
/// by the license agreement. The recipient of this software implicitly
/// accepts the terms of the license.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
///
///  \brief    Class representation of a port configuration
///
///  \author   MSc : WAGO GmbH & Co. KG
//------------------------------------------------------------------------------

#include "ip_address.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

#include "error_handling.hpp"
#include "utilities.hpp"

namespace configdnsmasq {

ip_address::ip_address(const std::string &address) {
  string_ = address;
  binary_ = 0;
  auto status = conv_ip_ascii2bin(binary_, address);
  erh_assert(status == SUCCESS, INVALID_PARAMETER,
    boost::format("The ip address %s cannot be parsed to its binary representation") % address);
}

ip_address::ip_address(uint32_t address) {
  binary_ = address;
  conv_ip_bin2ascii(string_, address);
}

std::string ip_address::asString() const { return string_; }

uint32_t ip_address::asBinary() const { return binary_; }

bool ip_address::isZero() const { return binary_ == 0; }

} // namespace configdnsmasq
