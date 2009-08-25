SUBDIRS = src marshallers initscript utils

EXTRA_DIST =				\
	CONTRIBUTING			\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in

dbusservicedir = $(DBUS_SYS_DIR)
dbusservice_DATA = org.mbm.MbmGps.conf

dbusactivationdir = $(datadir)/dbus-1/system-services
dbusactivation_in_files = org.mbm.MbmGps.service.in
dbusactivation_DATA = $(dbusactivation_in_files:.service.in=.service)

%service: %service.in
	$(edit) $< >$@

edit = @sed \
	-e 's|@sbindir[@]|$(sbindir)|g' \
	-e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	-e 's|@localstatedir[@]|$(localstatedir)|g' \
	-e 's|@libexecdir[@]|$(libexecdir)|g'

DISTCLEANFILES = \
	$(dbusactivation_DATA)


EXTRA_DIST +=				\
	$(dbusservice_DATA) \
	$(dbusactivation_in_files)

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

