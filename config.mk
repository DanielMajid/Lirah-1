PROJECT := Lirah-1
PROJECT_TYPE := osc

UCSRC = $(PROJECT_ROOT)/src/header.c
UCXXSRC = \
  $(PROJECT_ROOT)/src/unit.cc \
  $(PROJECT_ROOT)/src/legacy/Lyre.cc

UASMSRC =
UASMXSRC =

UINCDIR = \
  $(PROJECT_ROOT)/src \
  $(PROJECT_ROOT)/src/legacy \
  $(COMMON_INC_PATH)/dsp

ULIBS = -lm
UDEFS =
