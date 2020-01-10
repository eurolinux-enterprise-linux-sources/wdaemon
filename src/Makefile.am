SUBDIRS = udev conf scripts

bin_PROGRAMS = wdaemon

AM_CPPFLAGS = $(EVEMU_CFLAGS)

wdaemon_SOURCES =  \
                    config.c \
                    debug.c \
                    debug.h \
                    device_types.c \
                    device_types.h \
                    evemu.c \
                    hotplug.c \
                    hotplug.h \
                    input.c \
                    input.h \
                    log.c \
                    log.h \
                    monitored.c \
                    monitored.h \
                    session.c \
                    session.h \
                    simple.c \
                    stddev.c \
                    stddev.h \
                    uinput.c \
                    uinput.h \
                    wacom.c \
                    wacom.h \
                    wdaemon-config.h \
                    wdaemon-evemu.h

wdaemon_CFLAGS = $(AM_CFLAGS) -DUINPUT_DEVICE='"$(UINPUT_DEVICE)"' -DWDAEMON_DEVNAME_PREFIX='"$(WDAEMON_DEVNAME_PREFIX)"'
wdaemon_LDADD = $(EVEMU_LIBS)

# man page generation.
if HAVE_DOCTOOLS
man_pages_src = wdaemon.txt
man_pages = $(man_pages_src:.txt=.1)

SUFFIXES = .1 .txt .xml

.xml.1:
	@$(XMLTO) man $<

.txt.xml:
	@$(ASCIIDOC) -b docbook -d manpage -o $@ $<

dist_man_MANS = $(man_pages)

EXTRA_DIST = $(man_pages_src) $(man_pages) LICENSE
CLEANFILES = $(dist_man_MANS)
MAINTAINERCLEANFILES = $(man_pages) *.xml
endif

