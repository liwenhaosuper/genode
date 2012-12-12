#
# \brief   Platform implementations specific for base-hw and i.MX53 (TrustZone)
# \author  Stefan Kalkowski
# \date    2012-10-30
#

# add include paths
INC_DIR += $(REP_DIR)/src/core/include \
           $(BASE_DIR)/src/core/include \
           $(REP_DIR)/src/core/imx53/trustzone

SRC_CC = platform_support.cc vm_session_component.cc

vpath platform_support.cc $(REP_DIR)/src/core/imx53/trustzone
vpath %.cc $(REP_DIR)/src/core