SRC_NAME=dnscrypt-proxy-1.9.5

CFLAGS +=$(COPT) -ffunction-sections -fdata-sections
CFLAGS += -I$(STAGEDIR)/include
LDFLAGS += -Wl,--gc-sections

all: config_test build

config_test:
	( if [ -f ./config_done ]; then \
		echo "the same configuration"; \
	else \
		make configure && touch config_done; \
	fi )

configure:
	( cd $(SRC_NAME) ; \
	./configure \
		--prefix=/usr \
		--sysconfdir=/etc/storage/dnscrypt-proxy \
		--host=$(HOST_TARGET) \
		--build=$(HOST_BUILD); \
	)

build:
	$(MAKE) V=1 -C $(SRC_NAME)

clean:
	if [ -f $(SRC_NAME)/Makefile ] ; then \
		$(MAKE) -C $(SRC_NAME) distclean ; \
	fi ; \
	rm -f config_done dnscrypt-resolvers.csv

romfs:
	wget -O dnscrypt-resolvers.csv https://raw.githubusercontent.com/jedisct1/dnscrypt-proxy/master/dnscrypt-resolvers.csv || cp $(SRC_NAME)/dnscrypt-resolvers.csv .
	$(ROMFSINST) $(SRC_NAME)/src/proxy/dnscrypt-proxy /usr/sbin/dnscrypt-proxy
	$(ROMFSINST) /usr/bin/dnscrypt-proxy.sh
	mkdir -p $(INSTALLDIR)/usr/share/dnscrypt-proxy
	$(ROMFSINST) dnscrypt-resolvers.csv /usr/share/dnscrypt-proxy
