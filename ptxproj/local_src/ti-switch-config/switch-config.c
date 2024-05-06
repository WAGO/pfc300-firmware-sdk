/*
 * Ethernet Switch configuration management
 *
 * Copyright (C) 2014 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define true 1
#define false 0
typedef int bool;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

#include "sockios_loc.h"

#ifdef USE_LOCAL_INC
#include "net_switch_config.h"
#else
#include <linux/net_switch_config.h>
#endif

#ifdef USE_OLD_API
#define OLD_CMD(name) CONFIG_##name

#define SWITCH_INVALID  OLD_CMD(SWITCH_INVALID)
#define SWITCH_ADD_MULTICAST  OLD_CMD(SWITCH_ADD_MULTICAST)
#define SWITCH_DEL_MULTICAST  OLD_CMD(SWITCH_DEL_MULTICAST)
#define SWITCH_ADD_VLAN  OLD_CMD(SWITCH_ADD_VLAN)
#define SWITCH_DEL_VLAN  OLD_CMD(SWITCH_DEL_VLAN)
#define SWITCH_SET_PORT_CONFIG  OLD_CMD(SWITCH_SET_PORT_CONFIG)
#define SWITCH_GET_PORT_CONFIG  OLD_CMD(SWITCH_GET_PORT_CONFIG)
#define SWITCH_ADD_UNKNOWN_VLAN_INFO  OLD_CMD(SWITCH_ADD_UNKNOWN_VLAN_INFO)
#define SWITCH_GET_PORT_STATE  OLD_CMD(SWITCH_GET_PORT_STATE)
#define SWITCH_SET_PORT_STATE  OLD_CMD(SWITCH_SET_PORT_STATE)
#define SWITCH_GET_PORT_VLAN_CONFIG  OLD_CMD(SWITCH_GET_PORT_VLAN_CONFIG)
#define SWITCH_SET_PORT_VLAN_CONFIG  OLD_CMD(SWITCH_SET_PORT_VLAN_CONFIG)
#define SWITCH_RATELIMIT  OLD_CMD(SWITCH_RATELIMIT)
#endif

typedef unsigned long long u64;
typedef __uint32_t u32;
typedef __uint16_t u16;
typedef __uint8_t u8;
typedef __int32_t s32;

enum {
	EXTENDED_CONFIG_SWITCH_INVALID,
	EXTENDED_CONFIG_SWITCH_DUMP_ALE,
	EXTENDED_CONFIG_SWITCH_DUMP_ALL,
};

#ifndef DIV_ROUND_UP
#define	DIV_ROUND_UP(x,y)	(((x) + ((y) - 1)) / (y))
#endif

#define CPSW_MAJOR_VERSION(reg)		(reg >> 8 & 0x7)
#define CPSW_MINOR_VERSION(reg)		(reg & 0xff)
#define CPSW_RTL_VERSION(reg)		((reg >> 11) & 0x1f)

#define ADDR_FMT_ARGS(addr)	(addr)[0], (addr)[1], (addr)[2], \
				(addr)[3], (addr)[4], (addr)[5]

#define ALE_ENTRY_BITS          68
#define ALE_ENTRY_WORDS         DIV_ROUND_UP(ALE_ENTRY_BITS, 32)

#define BIT(nr)			(1 << (nr))
#define BITMASK(bits)		(BIT(bits) - 1)

#define ALE_TYPE_FREE			0
#define ALE_TYPE_ADDR			1
#define ALE_TYPE_VLAN			2
#define ALE_TYPE_VLAN_ADDR		3

static inline int cpsw_ale_get_field(u32 *ale_entry, u32 start, u32 bits)
{
	int idx;

	idx    = start / 32;
	start -= idx * 32;
	idx    = 2 - idx; /* flip */
	return (ale_entry[idx] >> start) & BITMASK(bits);
}

static inline void cpsw_ale_set_field(u32 *ale_entry, u32 start, u32 bits,
				      u32 value)
{
	int idx;

	value &= BITMASK(bits);
	idx    = start / 32;
	start -= idx * 32;
	idx    = 2 - idx; /* flip */
	ale_entry[idx] &= ~(BITMASK(bits) << start);
	ale_entry[idx] |=  (value << start);
}

#define DEFINE_ALE_FIELD(name, start, bits)				\
static inline int cpsw_ale_get_##name(u32 *ale_entry)			\
{									\
	return cpsw_ale_get_field(ale_entry, start, bits);		\
}									\
static inline void cpsw_ale_set_##name(u32 *ale_entry, u32 value)	\
{									\
	cpsw_ale_set_field(ale_entry, start, bits, value);		\
}

DEFINE_ALE_FIELD(entry_type,		60,	2)
DEFINE_ALE_FIELD(vlan_id,		48,	12)
DEFINE_ALE_FIELD(mcast_state,		62,	2)
DEFINE_ALE_FIELD(port_mask,		66,     3)
DEFINE_ALE_FIELD(super,			65,	1)
DEFINE_ALE_FIELD(ucast_type,		62,     2)
DEFINE_ALE_FIELD(port_num,		66,     2)
DEFINE_ALE_FIELD(blocked,		65,     1)
DEFINE_ALE_FIELD(secure,		64,     1)
DEFINE_ALE_FIELD(vlan_untag_force,	24,	3)
DEFINE_ALE_FIELD(vlan_reg_mcast,	16,	3)
DEFINE_ALE_FIELD(vlan_unreg_mcast,	8,	3)
DEFINE_ALE_FIELD(vlan_member_list,	0,	3)
DEFINE_ALE_FIELD(mcast,			40,	1)

static inline void cpsw_ale_get_addr(u32 *ale_entry, u8 *addr)
{
	int i;

	for (i = 0; i < 6; i++)
		addr[i] = cpsw_ale_get_field(ale_entry, 40 - 8*i, 8);
}

static void cpsw_ale_dump_vlan(int index, u32 *ale_entry)
{
	int vlan = cpsw_ale_get_vlan_id(ale_entry);
	int untag_force = cpsw_ale_get_vlan_untag_force(ale_entry);
	int reg_mcast   = cpsw_ale_get_vlan_reg_mcast(ale_entry);
	int unreg_mcast = cpsw_ale_get_vlan_unreg_mcast(ale_entry);
	int member_list = cpsw_ale_get_vlan_member_list(ale_entry);

	fprintf(stdout, "%-4d: type: vlan , vid = %d, untag_force = 0x%x, reg_mcast = 0x%x, unreg_mcast = 0x%x, member_list = 0x%x\n",
		index, vlan, untag_force, reg_mcast, unreg_mcast, member_list);
}

static void cpsw_ale_dump_addr(int index, u32 *ale_entry)
{
	u8 addr[6];

	cpsw_ale_get_addr(ale_entry, addr);
	if (cpsw_ale_get_mcast(ale_entry)) {
		static const char *str_mcast_state[] = {"f", "blf", "lf", "f"};
		int state     = cpsw_ale_get_mcast_state(ale_entry);
		int port_mask = cpsw_ale_get_port_mask(ale_entry);
		int super     = cpsw_ale_get_super(ale_entry);

		fprintf(stdout, "%-4d: type: mcast, addr = %02x:%02x:%02x:%02x:%02x:%02x, mcast_state = %s, %ssuper, port_mask = 0x%x\n",
			index, ADDR_FMT_ARGS(addr), str_mcast_state[state],
			super ? "" : "no ", port_mask);
	} else {
		static const char *s_ucast_type[] = {"persistant", "untouched ",
						     "oui       ", "touched   "};
		int ucast_type = cpsw_ale_get_ucast_type(ale_entry);
		int port_num   = cpsw_ale_get_port_num(ale_entry);
		int secure     = cpsw_ale_get_secure(ale_entry);
		int blocked    = cpsw_ale_get_blocked(ale_entry);

		fprintf(stdout, "%-4d: type: ucast, addr = %02x:%02x:%02x:%02x:%02x:%02x, ucast_type = %s, port_num = 0x%x%s%s\n",
			index, ADDR_FMT_ARGS(addr), s_ucast_type[ucast_type],
			port_num, secure ? ", Secure" : "",
			blocked ? ", Blocked" : "");
	}
}

static void cpsw_ale_dump_vlan_addr(int index, u32 *ale_entry)
{
	u8 addr[6];
	int vlan = cpsw_ale_get_vlan_id(ale_entry);

	cpsw_ale_get_addr(ale_entry, addr);
	if (cpsw_ale_get_mcast(ale_entry)) {
		static const char *str_mcast_state[] = {"f", "blf", "lf", "f"};
		int state     = cpsw_ale_get_mcast_state(ale_entry);
		int port_mask = cpsw_ale_get_port_mask(ale_entry);
		int super     = cpsw_ale_get_super(ale_entry);

		fprintf(stdout, "%-4d: type: mcast, vid = %d, addr = %02x:%02x:%02x:%02x:%02x:%02x, mcast_state = %s, %ssuper, port_mask = 0x%x\n",
			index, vlan, ADDR_FMT_ARGS(addr),
			str_mcast_state[state], super ? "" : "no ", port_mask);
	} else {
		static const char *s_ucast_type[] = {"persistant", "untouched ",
						     "oui       ", "touched   "};
		int ucast_type = cpsw_ale_get_ucast_type(ale_entry);
		int port_num   = cpsw_ale_get_port_num(ale_entry);
		int secure     = cpsw_ale_get_secure(ale_entry);
		int blocked    = cpsw_ale_get_blocked(ale_entry);

		fprintf(stdout, "%-4d: type: ucast, vid = %d, addr = %02x:%02x:%02x:%02x:%02x:%02x, ucast_type = %s, port_num = 0x%x%s%s\n",
			index, vlan, ADDR_FMT_ARGS(addr),
			s_ucast_type[ucast_type], port_num,
			secure ? ", Secure" : "", blocked ? ", Blocked" : "");
	}
}

struct k3_cpsw_regdump_hdr {
	u32 module_id;
	u32 len;
} __packed;

enum {
	K3_CPSW_REGDUMP_MOD_NUSS = 1,
	K3_CPSW_REGDUMP_MOD_RGMII_STATUS = 2,
	K3_CPSW_REGDUMP_MOD_MDIO = 3,
	K3_CPSW_REGDUMP_MOD_CPSW = 4,
	K3_CPSW_REGDUMP_MOD_CPSW_P0 = 5,
	K3_CPSW_REGDUMP_MOD_CPSW_P1 = 6,
	K3_CPSW_REGDUMP_MOD_CPSW_CPTS = 7,
	K3_CPSW_REGDUMP_MOD_CPSW_ALE = 8,
	K3_CPSW_REGDUMP_MOD_CPSW_ALE_TBL = 9,
	K3_CPSW_REGDUMP_MOD_LAST,
};

static const char* mod_names[K3_CPSW_REGDUMP_MOD_LAST] = {
	[K3_CPSW_REGDUMP_MOD_NUSS] = "cpsw-nuss",
	[K3_CPSW_REGDUMP_MOD_RGMII_STATUS] = "cpsw-nuss-rgmii-status",
	[K3_CPSW_REGDUMP_MOD_MDIO] = "cpsw-nuss-mdio",
	[K3_CPSW_REGDUMP_MOD_CPSW] = "cpsw-nu",
	[K3_CPSW_REGDUMP_MOD_CPSW_P0] = "cpsw-nu-p0",
	[K3_CPSW_REGDUMP_MOD_CPSW_P1] = "cpsw-nu-p1",
	[K3_CPSW_REGDUMP_MOD_CPSW_CPTS] = "cpsw-nu-cpts",
	[K3_CPSW_REGDUMP_MOD_CPSW_ALE] = "cpsw-nu-ale",
	[K3_CPSW_REGDUMP_MOD_CPSW_ALE_TBL] = "cpsw-nu-ale-tbl"
};

void cpsw_dump_ale(struct k3_cpsw_regdump_hdr *ale_hdr, u32 *ale_pos)
{
	int i, ale_ents;

	if (!ale_hdr)
		return;

	ale_ents = (ale_hdr->len - sizeof(struct k3_cpsw_regdump_hdr)) /
		   ALE_ENTRY_WORDS / sizeof(u32);

	fprintf(stdout, "ALE table dump ents(%d): \n", ale_ents);

	ale_pos += 2;

	for(i = 0; i < ale_ents; i++, ale_pos += ALE_ENTRY_WORDS) {
		int type;

		type = cpsw_ale_get_entry_type(ale_pos);

		switch (type) {
		case ALE_TYPE_FREE:
			break;

		case ALE_TYPE_ADDR:
			cpsw_ale_dump_addr(i, ale_pos);
			break;

		case ALE_TYPE_VLAN:
			cpsw_ale_dump_vlan(i, ale_pos);
			break;

		case ALE_TYPE_VLAN_ADDR:
			cpsw_ale_dump_vlan_addr(i, ale_pos);
			break;

		default:
			fprintf(stdout, "%-4d: Invalid Entry type\n", i);
		}
	}
}

int cpsw_dump_regs(struct ethtool_drvinfo *info, struct ethtool_regs *regs,
		    int dump_cmd, int dump_level)
{
	struct k3_cpsw_regdump_hdr *dump_hdr, *ale_hdr = NULL;
	u32 *ale_pos, *reg = (u32 *)regs->data;
	int i;
	bool dump_ale = !!(dump_cmd == EXTENDED_CONFIG_SWITCH_DUMP_ALE);
	u32 mod_id;

	fprintf(stdout, "K3 cpsw dump version (%d) len(%d)\n",
		regs->version, info->regdump_len);

	i = 0;
	do {
		u32 *tmp, j;
		u32 num_items;
		dump_hdr = (struct k3_cpsw_regdump_hdr *)reg;
		mod_id = dump_hdr->module_id;

		if (mod_id == K3_CPSW_REGDUMP_MOD_CPSW_ALE_TBL) {
			ale_hdr = dump_hdr;
			ale_pos = reg;
			if (dump_ale)
				break;
		}

		num_items = dump_hdr->len / sizeof(u32);
		if (dump_ale) {
			reg += num_items;
			i += dump_hdr->len;
			continue;
		}

		if (dump_level && mod_id != dump_level) {
			reg += num_items;
			i += dump_hdr->len;
			continue;
		}

		fprintf(stdout, "%s regdump: num_regs(%d)\n",
			mod_names[mod_id], num_items - 2);
		tmp = reg;
		for (j = 2; j < num_items; j += 2) {
			fprintf(stdout, "%08x:reg(%08X)\n", tmp[j], tmp[j + 1]);
		}

		reg += num_items;
		i += dump_hdr->len;
	} while (i < info->regdump_len);

	if (dump_ale)
		cpsw_dump_ale(ale_hdr, ale_pos);

	return 0;
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define SWITCH_CONFIG_COMMAND(__var__, __cmd__)			\
	if((__var__) == SWITCH_INVALID) {		\
		(__var__) = (__cmd__);				\
	} else {						\
		printf("Two or more commands Cannot be "	\
			"processed simultaneously\n");		\
		return -1;					\
	}

static char options[] = "?vdlD::I:n:B:L:t";

static struct option long_options[] =
	{
		/* These options set a flag. */
{"dump-ale",			no_argument		, 0, 'd'},
{"dump",			optional_argument	, 0, 'D'},
{"ndev",			required_argument	, 0, 'I'},
{"version",			no_argument		, 0, 'v'},
{"rate-limit",			no_argument		, 0, 'l'},
{"port",			required_argument	, 0, 'n'},
{"bcast-limit",			required_argument	, 0, 'B'},
{"mcast-limit",			required_argument	, 0, 'L'},
{"direction",			no_argument		, 0, 't'},

{0, 0, 0, 0}
};

void print_help(void)
{
	int i;
	printf(
		"Switch configuration commands.....\n"
		"switch-config -I,--ndev <dev> <command>\n"
		"\ncommands:\n"
		"switch-config -l,--rate-limit -n,--port <Port No> "
			"-B,--bcast-limit <No of Packet 0-255> "
			"-L,--mcast-limit <No of Packet 0-255> "
			"[-t,--direction :specify for Tx] ]\n"
		"switch-config -d,--dump-ale :dump ALE table\n"
		"switch-config -D,--dump=<0..9> :dump registers (0 - all)\n"
		"switch-config -v,--version\n"
		"\n"
		);
	printf("\tdump values:\n");
	for (i = K3_CPSW_REGDUMP_MOD_NUSS; i < K3_CPSW_REGDUMP_MOD_LAST; i++)
		printf("\t:%d - %s regs\n", i, mod_names[i]);
}

int main(int argc, char **argv)
{
	int option_index = 0;
	int c;
	struct ifreq ifr;
	int sockfd;  /* socket fd we use to manipulate stuff with */
	int dump_cmd  = EXTENDED_CONFIG_SWITCH_INVALID;
	int dump_level = 0;
	struct net_switch_config cmd_struct = { 0 };
	const char *log_msg;

	/* get interface name */
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
	/* Create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Can't open the socket\n");
		return -1;
	}

	cmd_struct.cmd = SWITCH_INVALID;

	/* parse command line arguments */
	while ((c = getopt_long(argc, argv, options, long_options,
		&option_index)) != -1) {
		switch (c) {
		case '?':
			print_help();
			return 0;
		case 'v':
			fprintf(stdout, "switch-config version: %s\n", VERSION);
			return 0;

		case 'd':
			dump_cmd = EXTENDED_CONFIG_SWITCH_DUMP_ALE;
		break;

		case 'D':
			dump_cmd = EXTENDED_CONFIG_SWITCH_DUMP_ALL;
			if (optarg != NULL)
				dump_level = atoi(optarg);
		break;

		case 'I':
			strncpy(ifr.ifr_name, optarg, IFNAMSIZ);
		break;

		case 'l':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				SWITCH_RATELIMIT);
		break;

		/* Command arguments */
		case 'n':
			cmd_struct.port = atoi(optarg);
		break;

		case 'B':
			cmd_struct.bcast_rate_limit = atoi(optarg);
		break;

		case 'L':
			cmd_struct.mcast_rate_limit = atoi(optarg);
		break;

		case 't':
			cmd_struct.direction = true;
		break;

		default:
			print_help();
			close(sockfd);
			return -1;
		}
	}

	if (cmd_struct.cmd == SWITCH_INVALID)
		goto do_dump;

	switch (cmd_struct.cmd) {
		case SWITCH_RATELIMIT:
			if ((cmd_struct.port > 1)) {
				printf("Invalid Arguments\n");
				return -1;
			}
			log_msg = "Set Port B/M ratelimit";
		break;

		default:
			print_help();
			close(sockfd);
			return -1;
	}

	ifr.ifr_data = (char*)&cmd_struct;
	if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
		printf("%s Failed\n", log_msg);
		close(sockfd);
		return -1;

	} else {
		printf("%s successful\n", log_msg);
	}

	close(sockfd);
	return 0;

do_dump:
	if (!dump_cmd) {
		print_help();
		return -1;
	}

	{
		struct ethtool_drvinfo drvinfo;
		struct ethtool_regs *regs;

		drvinfo.cmd = ETHTOOL_GDRVINFO;
		ifr.ifr_data = (void *)&drvinfo;
		if (ioctl(sockfd, SIOCETHTOOL, &ifr) < 0) {
			perror("Cannot get driver information");
			return -1;
		}

		regs = calloc(1, sizeof(*regs)+drvinfo.regdump_len);
		if (!regs) {
			perror("Cannot allocate memory for register dump");
			return -1;
		}

		regs->cmd = ETHTOOL_GREGS;
		regs->len = drvinfo.regdump_len;
		ifr.ifr_data = (void *)regs;
		if (ioctl(sockfd, SIOCETHTOOL, &ifr) < 0) {
			perror("Cannot get driver information");
			return -1;
		}
		cpsw_dump_regs(&drvinfo, regs, dump_cmd, dump_level);
	}

	close(sockfd);
	return 0;
}
