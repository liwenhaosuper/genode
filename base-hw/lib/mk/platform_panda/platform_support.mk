#
# \brief   Platform implementations specific for base-hw and Panda A2
# \author  Martin Stein
# \date    2012-05-10
#

# add include paths
INC_DIR += $(REP_DIR)/src/core/include \
           $(BASE_DIR)/src/core/include

# add C++ sources
SRC_CC += platform_services.cc platform_support.cc

# declare source paths
vpath platform_services.cc $(BASE_DIR)/src/core
vpath platform_support.cc  $(REP_DIR)/src/core/panda

