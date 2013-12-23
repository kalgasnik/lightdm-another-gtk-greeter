bin_PROGRAMS = lightdm-another-gtk-greeter

lightdm_another_gtk_greeter_SOURCES = \
	main.c \
	shares.c \
	shares.h \
	configuration.c \
	configuration.h \
	composite_widgets.c \
	composite_widgets.h \
	model_listbox.c \
	model_listbox.h \
	model_menu.c \
	model_menu.h \
	indicator_a11y.c \
	indicator_a11y.h \
	indicator_layout.c \
	indicator_layout.h \
	indicator_power.c \
	indicator_power.h \
	indicator_clock.c \
	indicator_clock.h


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
