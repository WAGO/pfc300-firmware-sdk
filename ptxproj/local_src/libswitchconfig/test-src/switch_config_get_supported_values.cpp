// Copyright (c) 2023 WAGO GmbH & Co. KG
// SPDX-License-Identifier: MPL-2.0

#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include <vector>

#include "libswitchconfig.hpp"
#include "switch_config_api.hpp"

namespace wago::libswitchconfig {

TEST(switch_config_get_supported_values, get_supported_values) {

  supported_values sv;
  status s = get_supported_values(sv);

  EXPECT_TRUE(s.ok());
  EXPECT_EQ(sv.version, 1);
  EXPECT_EQ(sv.broadcast_protection_values.size(), 10);
  EXPECT_EQ(sv.broadcast_protection_values.at(0), 1000);
  EXPECT_EQ(sv.broadcast_protection_values.at(4), 5000);
  EXPECT_EQ(sv.broadcast_protection_values.at(9), 10000);
  EXPECT_EQ(sv.multicast_protection_values.size(), 10);
  EXPECT_EQ(sv.multicast_protection_values.at(0), 1000);
  EXPECT_EQ(sv.multicast_protection_values.at(4), 5000);
  EXPECT_EQ(sv.multicast_protection_values.at(9), 10000);
}

}  // namespace wago::libswitchconfig