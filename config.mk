# Build configuration for NTS-1 mkII user unit.
# Uses Make wildcards to auto-discover legacy source files in src/legacy/

PROJECT := Lirah-1
PROJECT_TYPE := osc

# Auto-compile all C/C++ files in src/legacy/ to avoid manual config updates
UCSRC = src/header.c $(wildcard src/legacy/*.c)
UCXXSRC = src/unit.cc $(wildcard src/legacy/*.cc) $(wildcard src/legacy/*.cpp)

UASMSRC =
UASMXSRC =

UINCDIR = \
  $(PROJECT_ROOT)/src \
  $(PROJECT_ROOT)/src/legacy

ULIBS = -lm
UDEFS =

# Local mkI osc compatibility shim uses mkII common headers.
UINCDIR += $(abspath $(TOOLSDIR)/../platform/nts-1_mkii/common)
UINCDIR += $(abspath $(TOOLSDIR)/../platform/nts-1_mkii/common/dsp)
UINCDIR += /Users/majid/repos/test_folder/Lyre-1-main
