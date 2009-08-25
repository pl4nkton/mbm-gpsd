libexec_PROGRAMS = convert-conpro

convert_conpro_SOURCES = convert_conpro.c

mnc_list.h: ConnectionProfiles.csv convert-conpro
	./convert-conpro ConnectionProfiles.csv

INCLUDES = -I${top_srcdir}                   \
           -I${top_srcdir}/include           \
           -I${top_builddir}/marshallers

sbin_PROGRAMS = mbm-gpsd

mbm_gpsd_SOURCES =				\
		mbm_gpsd.c				\
		gps_settings.c				\
		mbm_options.c			\
		mbm_manager.c			\
		mbm_errors.c			\
		mbm_serial.c			\
		mbm_supl.c				\
		mbm_modem.c			\
		mbm_parse.c

mbm-manager-glue.h: $(top_srcdir)/introspection/mbm-manager.xml
	dbus-binding-tool --prefix=mbm_manager --mode=glib-server --output=$@ $<

mbm-modem-glue.h: $(top_srcdir)/introspection/mbm-modem.xml
	dbus-binding-tool --prefix=mbm_modem --mode=glib-server --output=$@ $<


mbm-gps-glue.h: $(top_srcdir)/introspection/mbm-gps.xml
	dbus-binding-tool --prefix=mbm_gps --mode=glib-server --output=$@ $<

mbm-settings-glue.h: $(top_srcdir)/introspection/mbm-settings.xml
	dbus-binding-tool --prefix=mbm_settings --mode=glib-server --output=$@ $<
	


BUILT_SOURCES = 					\
	mbm-gps-glue.h					\
	mbm-manager-glue.h				\
	mbm-modem-glue.h				\
	mbm-settings-glue.h				\
	mnc_list.h 
	

mbm_gpsd_CPPFLAGS = 							\
	$(DBUS_CFLAGS)									\
	$(GTHREAD_CFLAGS)								\
	$(HAL_CFLAGS)									\
	$(OPENSSL_CFLAGS)								\
	$(LIBNL_CFLAGS)								\
	$(GMIME_CFLAGS)								\	
	-DDBUS_API_SUBJECT_TO_CHANGE						\
	-DG_DISABLE_DEPRECATED							\
	-DBINDIR=\"$(bindir)\"							\
	-DSBINDIR=\"$(sbindir)\"							\
	-DLIBEXECDIR=\"$(libexecdir)\"					\
	-DDATADIR=\"$(datadir)\"							\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DLOCALSTATEDIR=\"$(localstatedir)\"				\
	-DMBM_RUN_DIR=\"$(rundir)\"						\
	-DMBMLOCALEDIR=\"$(datadir)/locale\"				\
	-DARP_DEBUG							

mbm_gpsd_LDADD = 							\
			$(DBUS_LIBS)						\
			$(GTHREAD_LIBS)					\
			$(HAL_LIBS)						\
			$(LIBNL_LIBS)					\
			$(GMIME_LIBS)

mbm_gpsd_LDFLAGS = -rdynamic -lz 

dbusservicedir = $(DBUS_SYS_DIR)
dist_sysconf_DATA = mbm-gpsd.conf

EXTRA_DIST = \
	$(dbusservice_DATA) \
	$(MBM_DATA)

rundir=$(localstatedir)/run/MBM
install-data-hook:
	$(mkinstalldirs) -m 0700 $(DESTDIR)$(rundir)
	chmod 600 $(sysconfdir)/$(dist_sysconf_DATA)

CLEANFILES = $(BUILT_SOURCES)

