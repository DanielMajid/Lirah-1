PROJECT ?= Lirah-1
MODULE ?= osc
VERSION ?= 1.0.2

LOGUE_SDK_DIR ?= /Users/majid/repos/logue-sdk

SDK_PLATFORM_DIR := $(LOGUE_SDK_DIR)/platform/nts-1_mkii
SDK_TEMPLATE_MAKEFILE := $(SDK_PLATFORM_DIR)/$(if $(filter osc,$(MODULE)),dummy-osc,$(if $(filter modfx,$(MODULE)),dummy-modfx,$(if $(filter delfx,$(MODULE)),dummy-delfx,dummy-revfx)))/Makefile
SDK_GCC_BIN_PATH := $(LOGUE_SDK_DIR)/tools/gcc/gcc-arm-none-eabi-10.3-2021.10/bin

ARM_GCC := $(shell command -v arm-none-eabi-gcc 2>/dev/null)
ARM_GCC_BIN_PATH := $(patsubst %/,%,$(dir $(ARM_GCC)))

UNIT_FILE := $(PROJECT).nts1mkiiunit
PACKAGE := $(PROJECT)-$(VERSION).zip

SDK_MAKE_ARGS := \
  PROJECT_ROOT=$(CURDIR) \
  COMMON_INC_PATH=$(SDK_PLATFORM_DIR)/common \
  COMMON_SRC_PATH=$(SDK_PLATFORM_DIR)/common \
  TOOLSDIR=$(LOGUE_SDK_DIR)/tools \
  EXTDIR=$(LOGUE_SDK_DIR)/platform/ext \
  CMSISDIR=$(LOGUE_SDK_DIR)/platform/ext/CMSIS/CMSIS \
  LDDIR=$(SDK_PLATFORM_DIR)/ld \
  INSTALLDIR=$(CURDIR)

ifeq ($(wildcard $(SDK_GCC_BIN_PATH)/arm-none-eabi-gcc),)
ifneq ($(ARM_GCC),)
SDK_MAKE_ARGS += GCC_BIN_PATH=$(ARM_GCC_BIN_PATH)
endif
endif

.PHONY: all install clean package check-sdk
check-sdk:
	@if [ ! -f "$(SDK_TEMPLATE_MAKEFILE)" ]; then \
	  echo "Error: SDK template not found: $(SDK_TEMPLATE_MAKEFILE)"; \
	  echo "Set LOGUE_SDK_DIR to your local logue-sdk path."; \
	  exit 1; \
	fi

all: check-sdk
	@$(MAKE) -f "$(SDK_TEMPLATE_MAKEFILE)" $(SDK_MAKE_ARGS) all

install: check-sdk
	@$(MAKE) -f "$(SDK_TEMPLATE_MAKEFILE)" $(SDK_MAKE_ARGS) install

clean:
	@if [ -f "$(SDK_TEMPLATE_MAKEFILE)" ]; then \
	  $(MAKE) -f "$(SDK_TEMPLATE_MAKEFILE)" $(SDK_MAKE_ARGS) clean; \
	else \
	  echo "Skipping SDK clean (template Makefile not found)."; \
	fi
	@rm -f "$(PACKAGE)"

package: install
	@echo Packaging $(UNIT_FILE) -\> $(PACKAGE)
	@rm -f "$(PACKAGE)"
	@if [ -f src/credits.txt ]; then \
	  zip -q9 "$(PACKAGE)" "$(UNIT_FILE)" src/credits.txt; \
	else \
	  zip -q9 "$(PACKAGE)" "$(UNIT_FILE)"; \
	fi
	@echo Done
