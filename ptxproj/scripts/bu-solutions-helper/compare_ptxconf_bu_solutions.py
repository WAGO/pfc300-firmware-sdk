#
# Compares two given ptxdist configurations.
# the paths to the configurations have to be provided to the script.
# All known differences are blacklisted and will not be flagged.
#
# WAGO GmbH & Co. KG

import argparse

# ------------------------------------------------------------------------------

blacklist = [
    "PTXCONF_LIBCURL_VERBOSE",
    "PTXCONF_REMOTEACCESSMEDIATOR",
]

blacklist_unwanted_wbm_options = [
    "PTXCONF_WBM_NG_PLUGIN_CLOUDCONNECTIVITY",
    "PTXCONF_WBM_NG_PLUGIN_DOCKER", 
    "PTXCONF_WBM_NG_PLUGIN_IPK_UPLOADS", 
]
blacklist += blacklist_unwanted_wbm_options

blacklist_codesys_runtime = [
    "PTXCONF_CDS3", 
    "PTXCONF_ROOTFS_PASSWD_CODESYS3",
    "PTXCONF_ROOTFS_SHADOW_CODESYS3",
    "PTXCONF_ROOTFS_GROUP_CODESYS3",
    "PTXCONF_PLC_CODESYS_V3",
    "PTXCONF_CODESYS3",
    "PTXCONF_TSCPPCODESYS3APP",
    "PTXCONF_CT_CONFIG_CODESYS3",
    "PTXCONF_CT_TERMINATE_CODESYS",
    "PTXCONF_PP_CODESYS3",

    "PTXCONF_RUNTIME_RC_LINK",
    "PTXCONF_CT_CONFIG_RUNTIME",
    "PTXCONF_CT_GET_RUNTIME_CONFIG",
    "PTXCONF_WAGO_CUSTOM_RUNTIME_STARTSCRIPT",
    "PTXCONF_WBM_NG_PLUGIN_RUNTIME_CONFIGURATION", 
    "PTXCONF_WBM_NG_PLUGIN_RUNTIME_INFORMATION",  
    "PTXCONF_WBM_NG_PLUGIN_RUNTIME_SERVICES",

    "PTXCONF_PLCHANDLER", 
    "PTXCONF_CT_GET_PLC_CONFIG", 
    "PTXCONF_CT_CHANGE_RTS_CONFIG", 
    "PTXCONF_CT_GET_RTS_INFO", 
    "PTXCONF_CT_GET_RTS3SCFG_VALUE", 
    "PTXCONF_CT_IPC_MSG_HEADER",
    "PTXCONF_TSCPARAMSERV",

    "PTXCONF_FFF",
    
    "PTXCONF_HOST_PYTHON3_SETUPTOOLS",
    "PTXCONF_HOST_PYTHON3_JSMIN",
]
blacklist += blacklist_codesys_runtime

blacklist_bacnet = [
    "PTXCONF_BACNET", 
    "PTXCONF_PP_BACNET",
    "PTXCONF_LIBBACNET",
    "PTXCONF_LIBWEBSOCKETS",
    "PTXCONF_WEBSOCKETFRONTEND",
    "PTXCONF_WBM_NG_PLUGIN_BACNET",
]
blacklist += blacklist_bacnet

blacklist_opcua = [
    "PTXCONF_CONFIG_OPCUA", 
    "PTXCONF_WBM_NG_PLUGIN_OPCUA", 
]
blacklist += blacklist_opcua

blacklist_snmp = [
    "PTXCONF_NET_SNMP",
    "PTXCONF_LIBWAGOSNMP",
    "PTXCONF_CONFIG_SNMP",
    "PTXCONF_GET_SNMP_DATA",
    "PTXCONF_PP_SNMP",
    "PTXCONF_WBM_NG_PLUGIN_SNMP"
]
blacklist += blacklist_snmp

blacklist_tests_and_demos = [
    "PTXCONF_RT_TESTS", 
    "PTXCONF_RT_TESTS_SKIP_TARGETINSTALL", 
    "PTXCONF_RT_TESTS_CYCLICTEST",
    "PTXCONF_WDAPHPDEMO",
]
blacklist += blacklist_tests_and_demos

blacklist_active_only_in_bu_solutions_config = [
    "PTXCONF_DOCKER_ACTIVATE_ON_FIRST_BOOT"
]

# ------------------------------------------------------------------------------

def is_comment(line):
    if line[0] == '#':
        return True
    return False

def blacklisted(line, blacklist):
    for item in blacklist:
        if item in line:
            return True
    return False

def main(bu_solutions_configpath, pfc_standard_configpath):
    bu_solutions_lines = []
    pfc_standard_lines = []

    with open(bu_solutions_configpath, 'r') as f:
        bu_solutions_lines = f.readlines()

    with open(pfc_standard_configpath, 'r') as f:
        pfc_standard_lines = f.readlines()

    missing_in_bu_solutions = []
    additional_in_bu_solutions = []

    # check each line in bu-solutions config and make sure that there is no package mentioned that is not also in the default config
    for line in bu_solutions_lines:
        if not is_comment(line) and line not in pfc_standard_lines:
            if "=y" in line and not blacklisted(line, blacklist_active_only_in_bu_solutions_config):
                additional_in_bu_solutions.append(line.strip())

    # check each line in standard config and make sure that no new unknown packages are active
    for line in pfc_standard_lines:
        if not is_comment(line) and line not in bu_solutions_lines:
            if not blacklisted(line, blacklist):
                missing_in_bu_solutions.append(line.strip())

    if missing_in_bu_solutions:
        print ("Found lines in standard image that are not in the bu-solutions config and that are also not blacklisted:")
        print ("\n".join(missing_in_bu_solutions))

    if additional_in_bu_solutions:
        print ("Found lines in bu-solutions image that are not in the standard config:")
        print ("\n".join(additional_in_bu_solutions))

if __name__=='__main__':
    parser = argparse.ArgumentParser(description = 'Compare the standard and solutions ptxdist config\n', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-p', '--pfc_standard', help='path to standard config', required=True)
    parser.add_argument('-b', '--bu_solutions', help='path to bu_solutions config', required=True)

    args = parser.parse_args()

    main(args.bu_solutions, args.pfc_standard)
