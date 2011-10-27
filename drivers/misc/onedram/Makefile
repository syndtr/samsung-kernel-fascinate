#
# Makefile for kernel Onedram drivers.
#

obj-$(CONFIG_SAMSUNG_PHONE_TTY)	+= dpram.o

ifeq ($(CONFIG_DPRAM_RECOVERY),y)
obj-m				+= dpram_recovery.o
endif
