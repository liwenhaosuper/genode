TARGET    = vmm
REQUIRES  = trustzone platform_imx53
LIBS      = env cxx elf signal
SRC_CC    = main.cc
INC_DIR  += $(PRG_DIR)/../include
