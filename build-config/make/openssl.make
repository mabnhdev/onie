#-------------------------------------------------------------------------------
#
#  Copyright (C) 2013-2014 Curt Brune <curt@cumulusnetworks.com>
#
#  SPDX-License-Identifier:     GPL-2.0
#
#-------------------------------------------------------------------------------
#
# This is a makefile fragment that defines the build of openssl
#

OPENSSL_VERSION		= 1.0.2l
OPENSSL_TARBALL		= openssl-$(OPENSSL_VERSION).tar.gz
OPENSSL_TARBALL_URLS	+= $(ONIE_MIRROR) http://www.openssl.org/source
OPENSSL_BUILD_DIR	= $(MBUILDDIR)/openssl
OPENSSL_DIR		= $(OPENSSL_BUILD_DIR)/openssl-$(OPENSSL_VERSION)

OPENSSL_DOWNLOAD_STAMP	= $(DOWNLOADDIR)/openssl-download
OPENSSL_SOURCE_STAMP	= $(STAMPDIR)/openssl-source
OPENSSL_CONFIGURE_STAMP	= $(STAMPDIR)/openssl-configure
OPENSSL_BUILD_STAMP	= $(STAMPDIR)/openssl-build
OPENSSL_INSTALL_STAMP	= $(STAMPDIR)/openssl-install
OPENSSL_STAMP		= $(OPENSSL_SOURCE_STAMP) \
			  $(OPENSSL_CONFIGURE_STAMP) \
			  $(OPENSSL_BUILD_STAMP) \
			  $(OPENSSL_INSTALL_STAMP)

PHONY += openssl openssl-download openssl-source openssl-configure \
	openssl-build openssl-install openssl-clean openssl-download-clean

OPENSSL_SO = libssl.so.1.0.0 libcrypto.so.1.0.0
OPENSSL_LIBS = $(OPENSSL_SO) libssl.so libcrypto.so

openssl: $(OPENSSL_STAMP)

DOWNLOAD += $(OPENSSL_DOWNLOAD_STAMP)
openssl-download: $(OPENSSL_DOWNLOAD_STAMP)
$(OPENSSL_DOWNLOAD_STAMP): $(PROJECT_STAMP)
	$(Q) rm -f $@ && eval $(PROFILE_STAMP)
	$(Q) echo "==== Getting upstream openssl ===="
	$(Q) $(SCRIPTDIR)/fetch-package $(DOWNLOADDIR) $(UPSTREAMDIR) \
		$(OPENSSL_TARBALL) $(OPENSSL_TARBALL_URLS)
	$(Q) touch $@

SOURCE += $(OPENSSL_SOURCE_STAMP)
openssl-source: $(OPENSSL_SOURCE_STAMP)
$(OPENSSL_SOURCE_STAMP): $(TREE_STAMP) | $(OPENSSL_DOWNLOAD_STAMP)
	$(Q) rm -f $@ && eval $(PROFILE_STAMP)
	$(Q) echo "==== Extracting upstream openssl ===="
	$(Q) $(SCRIPTDIR)/extract-package $(OPENSSL_BUILD_DIR) $(DOWNLOADDIR)/$(OPENSSL_TARBALL)
	$(Q) touch $@

ifndef MAKE_CLEAN
OPENSSL_NEW_FILES = $(shell test -d $(OPENSSL_DIR) && test -f $(OPENSSL_BUILD_STAMP) && \
		      test -f $(OPENSSL_INSTALL_STAMP) && \
	              find -L $(OPENSSL_DIR) -newer $(OPENSSL_INSTALL_STAMP) \
		      -type f -print -quit)
endif

openssl-configure: $(OPENSSL_CONFIGURE_STAMP)
$(OPENSSL_CONFIGURE_STAMP): $(OPENSSL_SOURCE_STAMP) | $(DEV_SYSROOT_INIT_STAMP)
	$(Q) rm -f $@ && eval $(PROFILE_STAMP)
	$(Q) echo "====  Configure openssl-$(OPENSSL_VERSION) ===="
	$(Q) cd $(OPENSSL_DIR) && PATH='$(CROSSBIN):$(PATH)'	\
		$(OPENSSL_DIR)/Configure			\
		linux-x86_64 shared				\
		--prefix=$(DEV_SYSROOT)/usr			\
		--openssldir=$(DEV_SYSROOT)/usr/openssl		\
		--cross-compile-prefix=$(CROSSPREFIX)
	$(Q) touch $@

openssl-build: $(OPENSSL_BUILD_STAMP)
$(OPENSSL_BUILD_STAMP): $(OPENSSL_CONFIGURE_STAMP) $(OPENSSL_NEW_FILES) | $(DEV_SYSROOT_INIT_STAMP)
	$(Q) rm -f $@ && eval $(PROFILE_STAMP)
	$(Q) echo "====  Building openssl-$(OPENSSL_VERSION) ===="
	$(Q) PATH='$(CROSSBIN):$(PATH)'	$(MAKE) -C $(OPENSSL_DIR)
	$(Q) touch $@

openssl-install: $(OPENSSL_INSTALL_STAMP)
$(OPENSSL_INSTALL_STAMP): $(SYSROOT_INIT_STAMP) $(OPENSSL_BUILD_STAMP)
	$(Q) rm -f $@ && eval $(PROFILE_STAMP)
	$(Q) echo "==== Installing openssl in $(DEV_SYSROOT) ===="
	$(Q) PATH='$(CROSSBIN):$(PATH)'			\
		$(MAKE) -C $(OPENSSL_DIR) install
	$(Q) for file in $(OPENSSL_SO) ; do \
		chmod 755 $(DEV_SYSROOT)/usr/lib64/$$file ; \
	done
	$(Q) for file in $(OPENSSL_LIBS) ; do \
		cp -av $(DEV_SYSROOT)/usr/lib64/$$file $(SYSROOTDIR)/usr/lib/ ; \
	done
	$(Q) touch $(OPENSSL_BUILD_STAMP)
	$(Q) touch $@

USERSPACE_CLEAN += openssl-clean
openssl-clean:
	$(Q) rm -rf $(OPENSSL_BUILD_DIR)
	$(Q) rm -f $(OPENSSL_STAMP)
	$(Q) echo "=== Finished making $@ for $(PLATFORM)"

DOWNLOAD_CLEAN += openssl-download-clean
openssl-download-clean:
	$(Q) rm -f $(OPENSSL_DOWNLOAD_STAMP) $(DOWNLOADDIR)/$(OPENSSL_TARBALL)

#-------------------------------------------------------------------------------
#
# Local Variables:
# mode: makefile-gmake
# End:
