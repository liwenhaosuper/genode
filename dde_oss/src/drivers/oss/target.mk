TARGET      = oss_drv
REQUIRES    = x86_32
LIBS        = cxx env dde_kit signal server
CONTRIB_DIR = $(REP_DIR)/contrib



CC_OPT += -Ulinux -D_KERNEL -fno-omit-frame-pointer

#
# Silence C code
#
CC_C_OPT = -Wno-unused-variable -Wno-unused-but-set-variable \
           -Wno-implicit-function-declaration \
           -Wno-sign-compare

#
# Native Genode sources
#
SRC_CC = os.cc main.cc driver.cc irq.cc quirks.cc
SRC_C  = dummies.c

#
# Driver sources
#
DRV = $(addprefix $(CONTRIB_DIR)/,kernel/drv target)

#
# Framwork sources
#
FRAMEWORK = $(addprefix $(CONTRIB_DIR)/kernel/framework/,\
              osscore audio mixer vmix_core midi ac97)

# find C files
SRC_C += $(shell find $(DRV) $(FRAMEWORK)  -name *.c -exec basename {} ";")

# find directories for VPATH
PATHS  = $(shell find $(DRV) $(FRAMEWORK) -type d)

# add include directories
INC_DIR += $(CONTRIB_DIR)/kernel/framework/include $(CONTRIB_DIR)/include \
           $(PRG_DIR)/include $(PRG_DIR)


vpath %.cc $(PRG_DIR)/signal
vpath %.c  $(PATHS)
