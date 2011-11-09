EXTRA_DIST =		\
	01mbm		\
	99-mbm.rules	\
	mbm_powerstate	\
	mbm_reset

install-data-hook:
	install -D 01mbm $(DESTDIR)/$(sysconfdir)/pm/sleep.d/01mbm
	install -D 99-mbm.rules $(DESTDIR)/$(sysconfdir)/udev/rules.d/99-mbm.rules
	install -D mbm_powerstate $(DESTDIR)/$(bindir)/mbm_powerstate
	install -D mbm_reset $(DESTDIR)/$(bindir)/mbm_reset

