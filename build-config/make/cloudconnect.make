#-------------------------------------------------------------------------------
#
#
#-------------------------------------------------------------------------------
#
# This is a makefile fragment that defines the build of the Extreme Networks
# cloudConnect client for ONIE.
#

EXTREME_CLOUDCONNECT_BUILD_DIR		= $(MBUILDDIR)/extremeCloudConnect
EXTREME_CLOUDCONNECT_DIR		= $(EXTREME_CLOUDCONNECT_BUILD_DIR)
EXTREME_CLOUDCONNECT_SOURCE_DIR		= $(MACHINEROOT)/cloudConnect/src
EXTREME_CLOUDCONNECT_DOWNLOAD_STAMP	= $(STAMPDIR)/extremeCloudConnect-download
EXTREME_CLOUDCONNECT_CONFIGURE_STAMP	= $(STAMPDIR)/extremeCloudConnect-configure
EXTREME_CLOUDCONNECT_INSTALL_STAMP	= $(STAMPDIR)/extremeCloudConnect-install
EXTREME_CLOUDCONNECT_SOURCE_STAMP	= $(STAMPDIR)/extremeCloudConnect-source
EXTREME_CLOUDCONNECT_BUILD_STAMP	= $(STAMPDIR)/extremeCloudConnect-build
EXTREME_CLOUDCONNECT_STAMP		= $(EXTREME_CLOUDCONNECT_INSTALL_STAMP) \
					  $(EXTREME_CLOUDCONNECT_CONFIGURE_STAMP) \
					  $(EXTREME_CLOUDCONNECT_BUILD_STAMP) \
					  $(EXTREME_CLOUDCONNECT_SOURCE_STAMP)

PHONY += cloudConnect cloudConnect-download cloudConnect-source cloudConnect-configure \
	 cloudConnect-build cloudConnect-install cloudConnect-clean \
	 cloudConnect-download-clean

cloudConnect: $(EXTREME_CLOUDCONNECT_STAMP)

DOWNLOAD += $(EXTREME_CLOUDCONNECT_DOWNLOAD_STAMP)
cloudConnect-download: $(EXTREME_CLOUDCONNECT_DOWNLOAD_STAMP)
$(EXTREME_CLOUDCONNECT_DOWNLOAD_STAMP): $(PROJECT_STAMP)
	$(Q) rm -f $@ && eval $(PROFILE_STAMP)
	$(Q) echo "==== Getting upstream Extreme cloudConnect ===="
	$(Q) touch $@

SOURCE += $(EXTREME_CLOUDCONNECT_SOURCE_STAMP)
cloudConnect-source: $(EXTREME_CLOUDCONNECT_SOURCE_STAMP)
$(EXTREME_CLOUDCONNECT_SOURCE_STAMP): $(EXTREME_CLOUDCONNECT_DOWNLOAD_STAMP) \
				      $(EXTREME_CLOUDCONNECT_BUILD_DIR)
	$(Q) rm -f $@ && eval $(PROFILE_STAMP)
	$(Q) echo "==== Extracting Extreme cloudConnect ===="
	$(Q) cp -r $(EXTREME_CLOUDCONNECT_SOURCE_DIR)/* $(EXTREME_CLOUDCONNECT_BUILD_DIR)
	$(Q) touch $@

$(EXTREME_CLOUDCONNECT_BUILD_DIR):
	mkdir -p $@

ifndef MAKE_CLEAN
EXTREME_CLOUDCONNECT_NEW_FILES = $(shell test -d $(EXTREME_CLOUDCONNECT_SOURCE_DIR) && \
				  	 test -f $(EXTREME_CLOUDCONNECT_BUILD_STAMP) && \
					 find -L $(EXTREME_CLOUDCONNECT_DIR) \
					   -newer $(EXTREME_CLOUDCONNECT_BUILD_STAMP) \
					   -type f \! -name symlinks \! -name symlinks.o \
					   -print -quit)
endif

cloudConnect-configure: $(EXTREME_CLOUDCONNECT_CONFIGURE_STAMP)
$(EXTREME_CLOUDCONNECT_CONFIGURE_STAMP): $(EXTREME_CLOUDCONNECT_SOURCE_STAMP) | \
					 $(DEV_SYSROOT_INIT_STAMP)
	$(Q) rm -f $@ && eval $(PROFILE_STAMP)
	$(Q) echo "====  Configure EXtreme CloudConnect ===="
	$(Q) touch $@

cloudConnect-build: $(EXTREME_CLOUDCONNECT_BUILD_STAMP)
$(EXTREME_CLOUDCONNECT_BUILD_STAMP): $(EXTREME_CLOUDCONNECT_CONFIGURE_STAMP) \
				     $(EXTREME_CLOUDCONNECT_NEW_FILES) \
				     $(OPENSSL_INSTALL_STAMP) | \
				     $(DEV_SYSROOT_INIT_STAMP)
	$(Q) rm -f $@ && eval $(PROFILE_STAMP)
	$(Q) echo "====  Building Extreme Networks CloudConnect ===="
	$(Q) PATH='$(CROSSBIN):$(PATH)'	$(MAKE) -C $(EXTREME_CLOUDCONNECT_BUILD_DIR) \
		CROSS_COMPILE=$(CROSSPREFIX) CFLAGS="$(ONIE_CFLAGS)"  \
		LDFLAGS="$(ONIE_LDFLAGS)" SYSROOTDIR=$(SYSROOTDIR)
	$(Q) touch $@

cloudConnect-install:$(EXTREME_CLOUDCONNECT_INSTALL_STAMP)
$(EXTREME_CLOUDCONNECT_INSTALL_STAMP): $(SYSROOT_INIT_STAMP) $(DEV_SYSROOT_INIT_STAMP) $(EXTREME_CLOUDCONNECT_BUILD_STAMP)
	$(Q) echo "==== Installing Extreme Networks CloudConnect ===="
	$(Q) cp -v $(EXTREME_CLOUDCONNECT_BUILD_DIR)/cloudConnect  $(SYSROOTDIR)/usr/bin
	$(Q) touch $@

USERSPACE_CLEAN += cloudConnect-clean
cloudConnect-clean:
	$(Q) rm -rf $(EXTREME_CLOUDCONNECT_BUILD_DIR)
	$(Q) rm -f $(EXTREME_CLOUDCONNECT_BUILD_STAMP)
	$(Q) echo "=== Finished making $@ for $(PLATFORM)"

DOWNLOAD_CLEAN += cloudConnect-download-clean
cloudConnect-download-clean:
	$(Q) rm -f $(OPENSSL_DOWNLOAD_STAMP)

#-------------------------------------------------------------------------------
#
# Local Variables:
# mode: makefile-gmake
# End:
