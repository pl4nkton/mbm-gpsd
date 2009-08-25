noinst_LTLIBRARIES = libmarshallers.la

BUILT_SOURCES = \
	mbm-marshal.h \
	mbm-marshal.c

libmarshallers_la_SOURCES = \
	mbm-marshal-main.c

libmarshallers_la_CPPFLAGS = \
	$(GTHREAD_CFLAGS) \
	-DG_DISABLE_DEPRECATED


libmarshallers_la_LIBADD = $(GTHREAD_LIBS)

EXTRA_DIST = mbm-marshal.list
CLEANFILES = $(BUILT_SOURCES)

mbm-marshal.h: mbm-marshal.list
	$(GLIB_GENMARSHAL) $< --prefix=_mbm_marshal --header > $@

mbm-marshal.c: mbm-marshal.list
	$(GLIB_GENMARSHAL) $< --prefix=_mbm_marshal --body > $@

mbm-marshal-main.c: mbm-marshal.c mbm-marshal.h

