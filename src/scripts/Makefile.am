# initrd script
SCRIPT_IN_FILES=wdaemon.in

wdaemon: $(SCRIPT_IN_FILES) Makefile
	$(edit) $< >$@

edit = sed \
        -e 's|@sbindir[@]|$(sbindir)|g' \
        -e 's|@sysconfdir[@]|$(sysconfdir)|g' \
        -e 's|@localstatedir[@]|$(localstatedir)|g'

initddir=$(sysconfdir)/rc.d/init.d
initd_SCRIPTS= wdaemon


