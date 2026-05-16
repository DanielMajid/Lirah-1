##############################################################################
# Configuration for Makefile
#

PROJECT := Lirah-1
PROJECT_TYPE := osc

##############################################################################
# Sources
#

# C sources 
UCSRC = header.c

# C++ sources 
UCXXSRC = unit.cc

# List ASM source files here
UASMSRC = 

UASMXSRC = 

##############################################################################
# Include Paths
#

UINCDIR  = $(PROJECT_ROOT) \
            $(PROJECT_ROOT)/logue-sdk/platform/nts-1_mkii/common \
            $(PROJECT_ROOT)/logue-sdk/platform/common

##############################################################################
# Library Paths
#

ULIBDIR = 

##############################################################################
# Libraries
#

ULIBS  = -lm

##############################################################################
# Macros
#

UDEFS = 

