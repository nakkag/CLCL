# CLCL - build helpers for WSL
#
# The real work lives in scripts/build-wsl.sh (which also runs standalone);
# .EXPORT_ALL_VARIABLES hands it the knobs below as environment variables.

CONFIG    ?= Release
PLATFORM  ?= x86
# Empty: the .vcxproj files decide. Override to build against another toolset,
# e.g. TOOLSET=v145, or TOOLSET=v141_xp SDK=7.0 for the original XP config.
TOOLSET   ?=
SDK       ?=
WIN_BUILD ?= /mnt/c/temp/clcl-build
VSWHERE   ?= /mnt/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe
ARTIFACTS ?= CLCL.exe CLCLHook.dll CLCLSet.exe

.ONESHELL:
.EXPORT_ALL_VARIABLES:
.NOTPARALLEL:

# .ONESHELL feeds a whole recipe to one shell, and the default `sh -c` only
# reports the last line's status. -e restores per-line failure.
.SHELLFLAGS := -eu -c

.PHONY: help build-wsl clean-wsl
.DEFAULT_GOAL := help

help:
	@echo 'make build-wsl   build $(ARTIFACTS) into ./$(CONFIG)'
	@echo 'make clean-wsl   remove ./Release ./Debug and $(WIN_BUILD)'
	@echo
	@echo 'Overridable: CONFIG=$(CONFIG) PLATFORM=$(PLATFORM) WIN_BUILD=$(WIN_BUILD)'
	@echo '             TOOLSET/SDK (empty = as configured in the .vcxproj files)'

build-wsl:
	@./scripts/build-wsl.sh

clean-wsl:
	rm -rf Release Debug "$(WIN_BUILD)"
