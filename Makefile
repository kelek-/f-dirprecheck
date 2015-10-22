# f-dirprecheck Makefile
# f-dirprecheck--release--1.1 2005-10-30

CONFIG = config.h

CC = gcc
CFLAGS = -pipe -g -W -Wall -pedantic '-DCONFIG_H="$(CONFIG)"'

all: regextest f-dirprecheck

real: CONFIG=/glftpd/etc/f-dirprecheck.h
real: $(CONFIG) clean all

inc-config.h: $(CONFIG)
	cat $(CONFIG) | \
		perl -e 'print "const char confighdata[] = {"; \
			 print(join(",", map({ sprintf("0x%02X",ord($$_)); } split(//,join("",<>))))); \
			 print "};\n";' \
		> inc-config.h

regextest: regextest.c $(CONFIG)
	$(CC) $(CFLAGS) $< -o $@
	./regextest || ( rm ./regextest; exit 1 )

f-dirprecheck: f-dirprecheck.c $(CONFIG) inc-config.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f f-dirprecheck regextest inc-config.h core

install: all
	install -m 755 f-dirprecheck /glftpd/bin/f-dirprecheck

realinstall: real install
