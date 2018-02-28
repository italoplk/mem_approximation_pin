ARCH = x86

ifeq ($(ARCH),x86)
        CXX = g++
        ISA_FILE = $(SPM_GEM5_DIR)/util/m5/m5op_x86.S
        ARCH_FLAGS = -DX86_ARCH
        #EXTRAFLAGS = -fstack-usage
else ifeq ($(ARCH),alpha)
        #CXX = alpha-linux-gnu-gcc-4.7
        CXX = alphaev67-unknown-linux-gnu-gcc
        ISA_FILE = $(SPM_GEM5_DIR)/util/m5/m5op_alpha.S
        ARCH_FLAGS = -DALPHA_ARCH
        #EXTRAFLAGS = -fstack-usage
else ifeq ($(ARCH),arm32)
        CXX = arm-linux-gnueabi-gcc
        ISA_FILE = $(SPM_GEM5_DIR)/util/m5/m5op_arm.S
        ARCH_FLAGS = -DARM32_ARCH -mfloat-abi=softfp -mfpu=vfpv3
        #EXTRAFLAGS = -fstack-usage
else ifeq ($(ARCH),arm64)
        CXX = aarch64-linux-gnu-gcc
        ISA_FILE = $(SPM_GEM5_DIR)/util/m5/m5op_arm_A64.S
        ARCH_FLAGS = -DARM64_ARCH
        #EXTRAFLAGS = -fstack-usage
endif

APPROX = on

INCLUDES = -I$(SPM_GEM5_DIR)/src/mem/spm/api 
INCLUDES += -I$(SPM_GEM5_DIR)/util/m5 
INCLUDES += $(ISA_FILE)

BINDIR = ./bin/$(ARCH)

DEBUG = 0
OLEVEL = 3
ifeq ($(DEBUG), 1)
	OPTFLAGS = -g -Wall
else 
	OPTFLAGS = -O$(OLEVEL) -Wall
endif

LDFLAGS = -static -lm -std=c++11

SRCS = canny_edge.c hysteresis.c pgm_io.c MiniPID.cpp

canny: $(SRCS) approx.h Makefile
	$(CXX) $(OPTFLAGS) $(LDFLAGS) $(ARCH_FLAGS) $(EXTRAFLAGS) -DAPPROX_$(SPM) \
	-o $(BINDIR)/canny $(SRCS) $(INCLUDES) -lm

clean:
	rm -rf *.o $(BINDIR)/* output* *.su
