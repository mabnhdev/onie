# Extreme Networks Summit X870

ONIE_ARCH ?= x86_64

VENDOR_REV ?= 1

SWITCH_ASIC_VENDOR ?= bcm

# Translate hardware revision to ONIE hardware revision
ifeq ($(VENDOR_REV),1)
  MACHINE_REV = 1
else
  $(warning Unknown VENDOR_REV '$(VENDOR_REV)' for MACHINE '$(MACHINE)')
  $(error Unknown VENDOR_REV)
endif

# The VENDOR_VERSION string is appended to the overall ONIE version
# string.  HW vendors can use this to appended their own versioning
# information to the base ONIE version string.
VENDOR_VERSION = .0.1

# Vendor ID -- IANA Private Enterprise Number:
# http://www.iana.org/assignments/enterprise-numbers
# Extreme Networks Inc.
VENDOR_ID = 1916

# Enable the i2ctools and the onie-syseeprom command for this platform
I2CTOOLS_ENABLE = yes
I2CTOOLS_SYSEEPROM = no

# Enable the OpenSSL for this platform
OPENSSL_ENABLE = yes

# Enable the Extreme Networks CloudConnect for this platform
CLOUDCONNECT_ENABLE = yes

# Console parameters
CONSOLE_DEV = 1

# Set Linux kernel version
LINUX_VERSION		= 3.14
LINUX_MINOR_VERSION	= 27

#-------------------------------------------------------------------------------
#
# Local Variables:
# mode: makefile-gmake
# End:

