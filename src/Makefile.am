sbin_PROGRAMS = lightdm-another-gtk-greeter

lightdm_another_gtk_greeter_SOURCES = \
	main.c \
	shares.c \
	configuration.c \
	indicator_a11y.c \
	indicator_layout.c \
	indicator_power.c \
	indicator_clock.c \
	configuration.h indicator_layout.h indicator_clock.h indicator_a11y.h indicator_power.h shares.h


lightdm_another_gtk_greeter_CFLAGS = \
    -DPACKAGE_VERSION=\"$(PACKAGE_VERSION)\" \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	-DLOCALE_DIR=\""$(localedir)"\" \
	-DGREETER_DATA_DIR=\""$(datadir)/lightdm-another-gtk-greeter"\" \
	-DCONFIG_FILE=\""$(sysconfdir)/lightdm/lightdm-another-gtk-greeter.conf"\" \
	$(GREETER_CFLAGS) \
	$(IDO_CFLAGS) \
	$(WARN_CFLAGS)

lightdm_another_gtk_greeter_LDADD = \
	$(GREETER_LIBS) $(IDO_LIBS)
