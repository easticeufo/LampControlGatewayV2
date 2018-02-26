#
#   appweb-freebsd-default.mk -- Makefile to build Embedthis Appweb for freebsd
#

NAME                  := appweb
VERSION               := 4.7.3
PROFILE               ?= default
ARCH                  ?= $(shell uname -m | sed 's/i.86/x86/;s/x86_64/x64/;s/arm.*/arm/;s/mips.*/mips/')
CC_ARCH               ?= $(shell echo $(ARCH) | sed 's/x86/i686/;s/x64/x86_64/')
OS                    ?= freebsd
CC                    ?= gcc
CONFIG                ?= $(OS)-$(ARCH)-$(PROFILE)
BUILD                 ?= build/$(CONFIG)
LBIN                  ?= $(BUILD)/bin
PATH                  := $(LBIN):$(PATH)

ME_COM_CGI            ?= 1
ME_COM_COMPILER       ?= 1
ME_COM_DIR            ?= 1
ME_COM_EJS            ?= 0
ME_COM_ESP            ?= 1
ME_COM_EST            ?= 0
ME_COM_HTTP           ?= 1
ME_COM_LIB            ?= 1
ME_COM_MDB            ?= 1
ME_COM_MPR            ?= 1
ME_COM_OPENSSL        ?= 1
ME_COM_OSDEP          ?= 1
ME_COM_PCRE           ?= 1
ME_COM_PHP            ?= 0
ME_COM_SQLITE         ?= 0
ME_COM_SSL            ?= 1
ME_COM_VXWORKS        ?= 0
ME_COM_WINSDK         ?= 1
ME_COM_ZLIB           ?= 0


ifeq ($(ME_COM_EST),1)
    ME_COM_SSL := 1
endif
ifeq ($(ME_COM_LIB),1)
    ME_COM_COMPILER := 1
endif
ifeq ($(ME_COM_OPENSSL),1)
    ME_COM_SSL := 1
endif
ifeq ($(ME_COM_EJS),1)
    ME_COM_ZLIB := 1
endif
ifeq ($(ME_COM_ESP),1)
    ME_COM_MDB := 1
endif

CFLAGS                += -fPIC -w
DFLAGS                += -D_REENTRANT -DPIC $(patsubst %,-D%,$(filter ME_%,$(MAKEFLAGS))) -DME_COM_CGI=$(ME_COM_CGI) -DME_COM_COMPILER=$(ME_COM_COMPILER) -DME_COM_DIR=$(ME_COM_DIR) -DME_COM_EJS=$(ME_COM_EJS) -DME_COM_ESP=$(ME_COM_ESP) -DME_COM_EST=$(ME_COM_EST) -DME_COM_HTTP=$(ME_COM_HTTP) -DME_COM_LIB=$(ME_COM_LIB) -DME_COM_MDB=$(ME_COM_MDB) -DME_COM_MPR=$(ME_COM_MPR) -DME_COM_OPENSSL=$(ME_COM_OPENSSL) -DME_COM_OSDEP=$(ME_COM_OSDEP) -DME_COM_PCRE=$(ME_COM_PCRE) -DME_COM_PHP=$(ME_COM_PHP) -DME_COM_SQLITE=$(ME_COM_SQLITE) -DME_COM_SSL=$(ME_COM_SSL) -DME_COM_VXWORKS=$(ME_COM_VXWORKS) -DME_COM_WINSDK=$(ME_COM_WINSDK) -DME_COM_ZLIB=$(ME_COM_ZLIB) 
IFLAGS                += "-I$(BUILD)/inc"
LDFLAGS               += 
LIBPATHS              += -L$(BUILD)/bin
LIBS                  += -ldl -lpthread -lm

DEBUG                 ?= debug
CFLAGS-debug          ?= -g
DFLAGS-debug          ?= -DME_DEBUG
LDFLAGS-debug         ?= -g
DFLAGS-release        ?= 
CFLAGS-release        ?= -O2
LDFLAGS-release       ?= 
CFLAGS                += $(CFLAGS-$(DEBUG))
DFLAGS                += $(DFLAGS-$(DEBUG))
LDFLAGS               += $(LDFLAGS-$(DEBUG))

ME_ROOT_PREFIX        ?= 
ME_BASE_PREFIX        ?= $(ME_ROOT_PREFIX)/usr/local
ME_DATA_PREFIX        ?= $(ME_ROOT_PREFIX)/
ME_STATE_PREFIX       ?= $(ME_ROOT_PREFIX)/var
ME_APP_PREFIX         ?= $(ME_BASE_PREFIX)/lib/$(NAME)
ME_VAPP_PREFIX        ?= $(ME_APP_PREFIX)/$(VERSION)
ME_BIN_PREFIX         ?= $(ME_ROOT_PREFIX)/usr/local/bin
ME_INC_PREFIX         ?= $(ME_ROOT_PREFIX)/usr/local/include
ME_LIB_PREFIX         ?= $(ME_ROOT_PREFIX)/usr/local/lib
ME_MAN_PREFIX         ?= $(ME_ROOT_PREFIX)/usr/local/share/man
ME_SBIN_PREFIX        ?= $(ME_ROOT_PREFIX)/usr/local/sbin
ME_ETC_PREFIX         ?= $(ME_ROOT_PREFIX)/etc/$(NAME)
ME_WEB_PREFIX         ?= $(ME_ROOT_PREFIX)/var/www/$(NAME)
ME_LOG_PREFIX         ?= $(ME_ROOT_PREFIX)/var/log/$(NAME)
ME_SPOOL_PREFIX       ?= $(ME_ROOT_PREFIX)/var/spool/$(NAME)
ME_CACHE_PREFIX       ?= $(ME_ROOT_PREFIX)/var/spool/$(NAME)/cache
ME_SRC_PREFIX         ?= $(ME_ROOT_PREFIX)$(NAME)-$(VERSION)


TARGETS               += $(BUILD)/bin/appweb
TARGETS               += $(BUILD)/bin/authpass
ifeq ($(ME_COM_CGI),1)
    TARGETS           += $(BUILD)/bin/cgiProgram
endif
ifeq ($(ME_COM_EJS),1)
    TARGETS           += $(BUILD)/bin/ejs.mod
endif
ifeq ($(ME_COM_EJS),1)
    TARGETS           += $(BUILD)/bin/ejs
endif
ifeq ($(ME_COM_ESP),1)
    TARGETS           += $(BUILD)/esp
endif
ifeq ($(ME_COM_ESP),1)
    TARGETS           += $(BUILD)/bin/esp.conf
endif
ifeq ($(ME_COM_ESP),1)
    TARGETS           += $(BUILD)/bin/esp
endif
TARGETS               += $(BUILD)/bin/ca.crt
ifeq ($(ME_COM_HTTP),1)
    TARGETS           += $(BUILD)/bin/http
endif
ifeq ($(ME_COM_CGI),1)
    TARGETS           += $(BUILD)/bin/libmod_cgi.so
endif
ifeq ($(ME_COM_EJS),1)
    TARGETS           += $(BUILD)/bin/libmod_ejs.so
endif
ifeq ($(ME_COM_PHP),1)
    TARGETS           += $(BUILD)/bin/libmod_php.so
endif
ifeq ($(ME_COM_SSL),1)
    TARGETS           += $(BUILD)/bin/libmod_ssl.so
endif
ifeq ($(ME_COM_SQLITE),1)
    TARGETS           += $(BUILD)/bin/libsql.so
endif
TARGETS               += $(BUILD)/bin/makerom
TARGETS               += $(BUILD)/bin/appman
TARGETS               += src/server/cache
ifeq ($(ME_COM_SQLITE),1)
    TARGETS           += $(BUILD)/bin/sqlite
endif
ifeq ($(ME_COM_CGI),1)
    TARGETS           += test/web/auth/basic/basic.cgi
endif
ifeq ($(ME_COM_CGI),1)
    TARGETS           += test/web/caching/cache.cgi
endif
ifeq ($(ME_COM_CGI),1)
    TARGETS           += test/cgi-bin/cgiProgram
endif
ifeq ($(ME_COM_CGI),1)
    TARGETS           += test/cgi-bin/testScript
endif

unexport CDPATH

ifndef SHOW
.SILENT:
endif

all build compile: prep $(TARGETS)

.PHONY: prep

prep:
	@echo "      [Info] Use "make SHOW=1" to trace executed commands."
	@if [ "$(CONFIG)" = "" ] ; then echo WARNING: CONFIG not set ; exit 255 ; fi
	@if [ "$(ME_APP_PREFIX)" = "" ] ; then echo WARNING: ME_APP_PREFIX not set ; exit 255 ; fi
	@[ ! -x $(BUILD)/bin ] && mkdir -p $(BUILD)/bin; true
	@[ ! -x $(BUILD)/inc ] && mkdir -p $(BUILD)/inc; true
	@[ ! -x $(BUILD)/obj ] && mkdir -p $(BUILD)/obj; true
	@[ ! -f $(BUILD)/inc/me.h ] && cp projects/appweb-freebsd-default-me.h $(BUILD)/inc/me.h ; true
	@if ! diff $(BUILD)/inc/me.h projects/appweb-freebsd-default-me.h >/dev/null ; then\
		cp projects/appweb-freebsd-default-me.h $(BUILD)/inc/me.h  ; \
	fi; true
	@if [ -f "$(BUILD)/.makeflags" ] ; then \
		if [ "$(MAKEFLAGS)" != "`cat $(BUILD)/.makeflags`" ] ; then \
			echo "   [Warning] Make flags have changed since the last build" ; \
			echo "   [Warning] Previous build command: "`cat $(BUILD)/.makeflags`"" ; \
		fi ; \
	fi
	@echo "$(MAKEFLAGS)" >$(BUILD)/.makeflags

clean:
	rm -f "$(BUILD)/obj/appweb.o"
	rm -f "$(BUILD)/obj/authpass.o"
	rm -f "$(BUILD)/obj/cgiHandler.o"
	rm -f "$(BUILD)/obj/cgiProgram.o"
	rm -f "$(BUILD)/obj/config.o"
	rm -f "$(BUILD)/obj/convenience.o"
	rm -f "$(BUILD)/obj/dirHandler.o"
	rm -f "$(BUILD)/obj/ejs.o"
	rm -f "$(BUILD)/obj/ejsHandler.o"
	rm -f "$(BUILD)/obj/ejsLib.o"
	rm -f "$(BUILD)/obj/ejsc.o"
	rm -f "$(BUILD)/obj/esp.o"
	rm -f "$(BUILD)/obj/espLib.o"
	rm -f "$(BUILD)/obj/estLib.o"
	rm -f "$(BUILD)/obj/fileHandler.o"
	rm -f "$(BUILD)/obj/http.o"
	rm -f "$(BUILD)/obj/httpLib.o"
	rm -f "$(BUILD)/obj/log.o"
	rm -f "$(BUILD)/obj/makerom.o"
	rm -f "$(BUILD)/obj/manager.o"
	rm -f "$(BUILD)/obj/mprLib.o"
	rm -f "$(BUILD)/obj/mprSsl.o"
	rm -f "$(BUILD)/obj/pcre.o"
	rm -f "$(BUILD)/obj/phpHandler.o"
	rm -f "$(BUILD)/obj/server.o"
	rm -f "$(BUILD)/obj/slink.o"
	rm -f "$(BUILD)/obj/sqlite.o"
	rm -f "$(BUILD)/obj/sqlite3.o"
	rm -f "$(BUILD)/obj/sslModule.o"
	rm -f "$(BUILD)/obj/testAppweb.o"
	rm -f "$(BUILD)/obj/testHttp.o"
	rm -f "$(BUILD)/obj/zlib.o"
	rm -f "$(BUILD)/bin/appweb"
	rm -f "$(BUILD)/bin/authpass"
	rm -f "$(BUILD)/bin/cgiProgram"
	rm -f "$(BUILD)/bin/ejsc"
	rm -f "$(BUILD)/bin/ejs"
	rm -f "$(BUILD)/bin/esp.conf"
	rm -f "$(BUILD)/bin/esp"
	rm -f "$(BUILD)/bin/ca.crt"
	rm -f "$(BUILD)/bin/http"
	rm -f "$(BUILD)/bin/libappweb.so"
	rm -f "$(BUILD)/bin/libejs.so"
	rm -f "$(BUILD)/bin/libhttp.so"
	rm -f "$(BUILD)/bin/libmod_cgi.so"
	rm -f "$(BUILD)/bin/libmod_ejs.so"
	rm -f "$(BUILD)/bin/libmod_esp.so"
	rm -f "$(BUILD)/bin/libmod_php.so"
	rm -f "$(BUILD)/bin/libmod_ssl.so"
	rm -f "$(BUILD)/bin/libmpr.so"
	rm -f "$(BUILD)/bin/libmprssl.so"
	rm -f "$(BUILD)/bin/libpcre.so"
	rm -f "$(BUILD)/bin/libslink.so"
	rm -f "$(BUILD)/bin/libsql.so"
	rm -f "$(BUILD)/bin/libzlib.so"
	rm -f "$(BUILD)/bin/makerom"
	rm -f "$(BUILD)/bin/appman"
	rm -f "$(BUILD)/bin/sqlite"
	rm -f "$(BUILD)/bin/testAppweb"

clobber: clean
	rm -fr ./$(BUILD)

#
#   me.h
#

$(BUILD)/inc/me.h: $(DEPS_1)

#
#   osdep.h
#
DEPS_2 += src/osdep/osdep.h
DEPS_2 += $(BUILD)/inc/me.h

$(BUILD)/inc/osdep.h: $(DEPS_2)
	@echo '      [Copy] $(BUILD)/inc/osdep.h'
	mkdir -p "$(BUILD)/inc"
	cp src/osdep/osdep.h $(BUILD)/inc/osdep.h

#
#   mpr.h
#
DEPS_3 += src/mpr/mpr.h
DEPS_3 += $(BUILD)/inc/me.h
DEPS_3 += $(BUILD)/inc/osdep.h

$(BUILD)/inc/mpr.h: $(DEPS_3)
	@echo '      [Copy] $(BUILD)/inc/mpr.h'
	mkdir -p "$(BUILD)/inc"
	cp src/mpr/mpr.h $(BUILD)/inc/mpr.h

#
#   http.h
#
DEPS_4 += src/http/http.h
DEPS_4 += $(BUILD)/inc/mpr.h

$(BUILD)/inc/http.h: $(DEPS_4)
	@echo '      [Copy] $(BUILD)/inc/http.h'
	mkdir -p "$(BUILD)/inc"
	cp src/http/http.h $(BUILD)/inc/http.h

#
#   customize.h
#

src/customize.h: $(DEPS_5)

#
#   appweb.h
#
DEPS_6 += src/appweb.h
DEPS_6 += $(BUILD)/inc/mpr.h
DEPS_6 += $(BUILD)/inc/http.h
DEPS_6 += src/customize.h

$(BUILD)/inc/appweb.h: $(DEPS_6)
	@echo '      [Copy] $(BUILD)/inc/appweb.h'
	mkdir -p "$(BUILD)/inc"
	cp src/appweb.h $(BUILD)/inc/appweb.h

#
#   customize.h
#
DEPS_7 += src/customize.h

$(BUILD)/inc/customize.h: $(DEPS_7)
	@echo '      [Copy] $(BUILD)/inc/customize.h'
	mkdir -p "$(BUILD)/inc"
	cp src/customize.h $(BUILD)/inc/customize.h

#
#   ejs.h
#
DEPS_8 += src/ejs/ejs.h

$(BUILD)/inc/ejs.h: $(DEPS_8)
	@echo '      [Copy] $(BUILD)/inc/ejs.h'
	mkdir -p "$(BUILD)/inc"
	cp src/ejs/ejs.h $(BUILD)/inc/ejs.h

#
#   ejs.slots.h
#
DEPS_9 += src/ejs/ejs.slots.h

$(BUILD)/inc/ejs.slots.h: $(DEPS_9)
	@echo '      [Copy] $(BUILD)/inc/ejs.slots.h'
	mkdir -p "$(BUILD)/inc"
	cp src/ejs/ejs.slots.h $(BUILD)/inc/ejs.slots.h

#
#   ejsByteGoto.h
#
DEPS_10 += src/ejs/ejsByteGoto.h

$(BUILD)/inc/ejsByteGoto.h: $(DEPS_10)
	@echo '      [Copy] $(BUILD)/inc/ejsByteGoto.h'
	mkdir -p "$(BUILD)/inc"
	cp src/ejs/ejsByteGoto.h $(BUILD)/inc/ejsByteGoto.h

#
#   esp.h
#
DEPS_11 += src/esp/esp.h
DEPS_11 += $(BUILD)/inc/me.h
DEPS_11 += $(BUILD)/inc/osdep.h
DEPS_11 += $(BUILD)/inc/appweb.h

$(BUILD)/inc/esp.h: $(DEPS_11)
	@echo '      [Copy] $(BUILD)/inc/esp.h'
	mkdir -p "$(BUILD)/inc"
	cp src/esp/esp.h $(BUILD)/inc/esp.h

#
#   est.h
#
DEPS_12 += src/est/est.h
DEPS_12 += $(BUILD)/inc/me.h
DEPS_12 += $(BUILD)/inc/osdep.h

$(BUILD)/inc/est.h: $(DEPS_12)
	@echo '      [Copy] $(BUILD)/inc/est.h'
	mkdir -p "$(BUILD)/inc"
	cp src/est/est.h $(BUILD)/inc/est.h

#
#   pcre.h
#
DEPS_13 += src/pcre/pcre.h

$(BUILD)/inc/pcre.h: $(DEPS_13)
	@echo '      [Copy] $(BUILD)/inc/pcre.h'
	mkdir -p "$(BUILD)/inc"
	cp src/pcre/pcre.h $(BUILD)/inc/pcre.h

#
#   sqlite3.h
#
DEPS_14 += src/sqlite/sqlite3.h

$(BUILD)/inc/sqlite3.h: $(DEPS_14)
	@echo '      [Copy] $(BUILD)/inc/sqlite3.h'
	mkdir -p "$(BUILD)/inc"
	cp src/sqlite/sqlite3.h $(BUILD)/inc/sqlite3.h

#
#   testAppweb.h
#
DEPS_15 += test/src/testAppweb.h
DEPS_15 += $(BUILD)/inc/mpr.h
DEPS_15 += $(BUILD)/inc/http.h

$(BUILD)/inc/testAppweb.h: $(DEPS_15)
	@echo '      [Copy] $(BUILD)/inc/testAppweb.h'
	mkdir -p "$(BUILD)/inc"
	cp test/src/testAppweb.h $(BUILD)/inc/testAppweb.h

#
#   zlib.h
#
DEPS_16 += src/zlib/zlib.h

$(BUILD)/inc/zlib.h: $(DEPS_16)
	@echo '      [Copy] $(BUILD)/inc/zlib.h'
	mkdir -p "$(BUILD)/inc"
	cp src/zlib/zlib.h $(BUILD)/inc/zlib.h

#
#   appweb.o
#
DEPS_17 += $(BUILD)/inc/appweb.h

$(BUILD)/obj/appweb.o: \
    src/server/appweb.c $(DEPS_17)
	@echo '   [Compile] $(BUILD)/obj/appweb.o'
	$(CC) -c -o $(BUILD)/obj/appweb.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/server/appweb.c

#
#   authpass.o
#
DEPS_18 += $(BUILD)/inc/appweb.h

$(BUILD)/obj/authpass.o: \
    src/utils/authpass.c $(DEPS_18)
	@echo '   [Compile] $(BUILD)/obj/authpass.o'
	$(CC) -c -o $(BUILD)/obj/authpass.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/utils/authpass.c

#
#   cgiHandler.o
#
DEPS_19 += $(BUILD)/inc/appweb.h

$(BUILD)/obj/cgiHandler.o: \
    src/modules/cgiHandler.c $(DEPS_19)
	@echo '   [Compile] $(BUILD)/obj/cgiHandler.o'
	$(CC) -c -o $(BUILD)/obj/cgiHandler.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/modules/cgiHandler.c

#
#   cgiProgram.o
#

$(BUILD)/obj/cgiProgram.o: \
    src/utils/cgiProgram.c $(DEPS_20)
	@echo '   [Compile] $(BUILD)/obj/cgiProgram.o'
	$(CC) -c -o $(BUILD)/obj/cgiProgram.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/utils/cgiProgram.c

#
#   appweb.h
#

src/appweb.h: $(DEPS_21)

#
#   config.o
#
DEPS_22 += src/appweb.h
DEPS_22 += $(BUILD)/inc/pcre.h

$(BUILD)/obj/config.o: \
    src/config.c $(DEPS_22)
	@echo '   [Compile] $(BUILD)/obj/config.o'
	$(CC) -c -o $(BUILD)/obj/config.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/config.c

#
#   convenience.o
#
DEPS_23 += src/appweb.h

$(BUILD)/obj/convenience.o: \
    src/convenience.c $(DEPS_23)
	@echo '   [Compile] $(BUILD)/obj/convenience.o'
	$(CC) -c -o $(BUILD)/obj/convenience.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/convenience.c

#
#   dirHandler.o
#
DEPS_24 += src/appweb.h

$(BUILD)/obj/dirHandler.o: \
    src/dirHandler.c $(DEPS_24)
	@echo '   [Compile] $(BUILD)/obj/dirHandler.o'
	$(CC) -c -o $(BUILD)/obj/dirHandler.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/dirHandler.c

#
#   ejs.h
#

src/ejs/ejs.h: $(DEPS_25)

#
#   ejs.o
#
DEPS_26 += src/ejs/ejs.h

$(BUILD)/obj/ejs.o: \
    src/ejs/ejs.c $(DEPS_26)
	@echo '   [Compile] $(BUILD)/obj/ejs.o'
	$(CC) -c -o $(BUILD)/obj/ejs.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/ejs/ejs.c

#
#   ejsHandler.o
#
DEPS_27 += $(BUILD)/inc/appweb.h

$(BUILD)/obj/ejsHandler.o: \
    src/modules/ejsHandler.c $(DEPS_27)
	@echo '   [Compile] $(BUILD)/obj/ejsHandler.o'
	$(CC) -c -o $(BUILD)/obj/ejsHandler.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/modules/ejsHandler.c

#
#   ejsLib.o
#
DEPS_28 += src/ejs/ejs.h
DEPS_28 += $(BUILD)/inc/mpr.h
DEPS_28 += $(BUILD)/inc/pcre.h
DEPS_28 += $(BUILD)/inc/me.h

$(BUILD)/obj/ejsLib.o: \
    src/ejs/ejsLib.c $(DEPS_28)
	@echo '   [Compile] $(BUILD)/obj/ejsLib.o'
	$(CC) -c -o $(BUILD)/obj/ejsLib.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/ejs/ejsLib.c

#
#   ejsc.o
#
DEPS_29 += src/ejs/ejs.h

$(BUILD)/obj/ejsc.o: \
    src/ejs/ejsc.c $(DEPS_29)
	@echo '   [Compile] $(BUILD)/obj/ejsc.o'
	$(CC) -c -o $(BUILD)/obj/ejsc.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/ejs/ejsc.c

#
#   esp.h
#

src/esp/esp.h: $(DEPS_30)

#
#   esp.o
#
DEPS_31 += src/esp/esp.h

$(BUILD)/obj/esp.o: \
    src/esp/esp.c $(DEPS_31)
	@echo '   [Compile] $(BUILD)/obj/esp.o'
	$(CC) -c -o $(BUILD)/obj/esp.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/esp/esp.c

#
#   espLib.o
#
DEPS_32 += src/esp/esp.h
DEPS_32 += $(BUILD)/inc/pcre.h

$(BUILD)/obj/espLib.o: \
    src/esp/espLib.c $(DEPS_32)
	@echo '   [Compile] $(BUILD)/obj/espLib.o'
	$(CC) -c -o $(BUILD)/obj/espLib.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/esp/espLib.c

#
#   est.h
#

src/est/est.h: $(DEPS_33)

#
#   estLib.o
#
DEPS_34 += src/est/est.h

$(BUILD)/obj/estLib.o: \
    src/est/estLib.c $(DEPS_34)
	@echo '   [Compile] $(BUILD)/obj/estLib.o'
	$(CC) -c -o $(BUILD)/obj/estLib.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/est/estLib.c

#
#   fileHandler.o
#
DEPS_35 += src/appweb.h

$(BUILD)/obj/fileHandler.o: \
    src/fileHandler.c $(DEPS_35)
	@echo '   [Compile] $(BUILD)/obj/fileHandler.o'
	$(CC) -c -o $(BUILD)/obj/fileHandler.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/fileHandler.c

#
#   http.h
#

src/http/http.h: $(DEPS_36)

#
#   http.o
#
DEPS_37 += src/http/http.h

$(BUILD)/obj/http.o: \
    src/http/http.c $(DEPS_37)
	@echo '   [Compile] $(BUILD)/obj/http.o'
	$(CC) -c -o $(BUILD)/obj/http.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/http/http.c

#
#   httpLib.o
#
DEPS_38 += src/http/http.h

$(BUILD)/obj/httpLib.o: \
    src/http/httpLib.c $(DEPS_38)
	@echo '   [Compile] $(BUILD)/obj/httpLib.o'
	$(CC) -c -o $(BUILD)/obj/httpLib.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/http/httpLib.c

#
#   log.o
#
DEPS_39 += src/appweb.h

$(BUILD)/obj/log.o: \
    src/log.c $(DEPS_39)
	@echo '   [Compile] $(BUILD)/obj/log.o'
	$(CC) -c -o $(BUILD)/obj/log.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/log.c

#
#   mpr.h
#

src/mpr/mpr.h: $(DEPS_40)

#
#   makerom.o
#
DEPS_41 += src/mpr/mpr.h

$(BUILD)/obj/makerom.o: \
    src/mpr/makerom.c $(DEPS_41)
	@echo '   [Compile] $(BUILD)/obj/makerom.o'
	$(CC) -c -o $(BUILD)/obj/makerom.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mpr/makerom.c

#
#   manager.o
#
DEPS_42 += src/mpr/mpr.h

$(BUILD)/obj/manager.o: \
    src/mpr/manager.c $(DEPS_42)
	@echo '   [Compile] $(BUILD)/obj/manager.o'
	$(CC) -c -o $(BUILD)/obj/manager.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mpr/manager.c

#
#   mprLib.o
#
DEPS_43 += src/mpr/mpr.h

$(BUILD)/obj/mprLib.o: \
    src/mpr/mprLib.c $(DEPS_43)
	@echo '   [Compile] $(BUILD)/obj/mprLib.o'
	$(CC) -c -o $(BUILD)/obj/mprLib.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mpr/mprLib.c

#
#   mprSsl.o
#
DEPS_44 += $(BUILD)/inc/me.h
DEPS_44 += src/mpr/mpr.h
DEPS_44 += $(BUILD)/inc/est.h

$(BUILD)/obj/mprSsl.o: \
    src/mpr/mprSsl.c $(DEPS_44)
	@echo '   [Compile] $(BUILD)/obj/mprSsl.o'
	$(CC) -c -o $(BUILD)/obj/mprSsl.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) "-I$(ME_COM_OPENSSL_PATH)/include" src/mpr/mprSsl.c

#
#   pcre.h
#

src/pcre/pcre.h: $(DEPS_45)

#
#   pcre.o
#
DEPS_46 += $(BUILD)/inc/me.h
DEPS_46 += src/pcre/pcre.h

$(BUILD)/obj/pcre.o: \
    src/pcre/pcre.c $(DEPS_46)
	@echo '   [Compile] $(BUILD)/obj/pcre.o'
	$(CC) -c -o $(BUILD)/obj/pcre.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/pcre/pcre.c

#
#   phpHandler.o
#
DEPS_47 += $(BUILD)/inc/appweb.h

$(BUILD)/obj/phpHandler.o: \
    src/modules/phpHandler.c $(DEPS_47)
	@echo '   [Compile] $(BUILD)/obj/phpHandler.o'
	$(CC) -c -o $(BUILD)/obj/phpHandler.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/modules/phpHandler.c

#
#   server.o
#
DEPS_48 += src/appweb.h

$(BUILD)/obj/server.o: \
    src/server.c $(DEPS_48)
	@echo '   [Compile] $(BUILD)/obj/server.o'
	$(CC) -c -o $(BUILD)/obj/server.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/server.c

#
#   slink.o
#
DEPS_49 += $(BUILD)/inc/mpr.h
DEPS_49 += $(BUILD)/inc/esp.h

$(BUILD)/obj/slink.o: \
    src/slink.c $(DEPS_49)
	@echo '   [Compile] $(BUILD)/obj/slink.o'
	$(CC) -c -o $(BUILD)/obj/slink.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/slink.c

#
#   sqlite3.h
#

src/sqlite/sqlite3.h: $(DEPS_50)

#
#   sqlite.o
#
DEPS_51 += $(BUILD)/inc/me.h
DEPS_51 += src/sqlite/sqlite3.h

$(BUILD)/obj/sqlite.o: \
    src/sqlite/sqlite.c $(DEPS_51)
	@echo '   [Compile] $(BUILD)/obj/sqlite.o'
	$(CC) -c -o $(BUILD)/obj/sqlite.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/sqlite/sqlite.c

#
#   sqlite3.o
#
DEPS_52 += $(BUILD)/inc/me.h
DEPS_52 += src/sqlite/sqlite3.h

$(BUILD)/obj/sqlite3.o: \
    src/sqlite/sqlite3.c $(DEPS_52)
	@echo '   [Compile] $(BUILD)/obj/sqlite3.o'
	$(CC) -c -o $(BUILD)/obj/sqlite3.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/sqlite/sqlite3.c

#
#   sslModule.o
#
DEPS_53 += $(BUILD)/inc/appweb.h

$(BUILD)/obj/sslModule.o: \
    src/modules/sslModule.c $(DEPS_53)
	@echo '   [Compile] $(BUILD)/obj/sslModule.o'
	$(CC) -c -o $(BUILD)/obj/sslModule.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) "-I$(ME_COM_OPENSSL_PATH)/include" src/modules/sslModule.c

#
#   testAppweb.o
#
DEPS_54 += $(BUILD)/inc/testAppweb.h

$(BUILD)/obj/testAppweb.o: \
    test/src/testAppweb.c $(DEPS_54)
	@echo '   [Compile] $(BUILD)/obj/testAppweb.o'
	$(CC) -c -o $(BUILD)/obj/testAppweb.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) test/src/testAppweb.c

#
#   testHttp.o
#
DEPS_55 += $(BUILD)/inc/testAppweb.h

$(BUILD)/obj/testHttp.o: \
    test/src/testHttp.c $(DEPS_55)
	@echo '   [Compile] $(BUILD)/obj/testHttp.o'
	$(CC) -c -o $(BUILD)/obj/testHttp.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) test/src/testHttp.c

#
#   zlib.h
#

src/zlib/zlib.h: $(DEPS_56)

#
#   zlib.o
#
DEPS_57 += $(BUILD)/inc/me.h
DEPS_57 += src/zlib/zlib.h

$(BUILD)/obj/zlib.o: \
    src/zlib/zlib.c $(DEPS_57)
	@echo '   [Compile] $(BUILD)/obj/zlib.o'
	$(CC) -c -o $(BUILD)/obj/zlib.o $(LDFLAGS) $(CFLAGS) $(DFLAGS) $(IFLAGS) src/zlib/zlib.c

#
#   libmpr
#
DEPS_58 += $(BUILD)/inc/mpr.h
DEPS_58 += $(BUILD)/obj/mprLib.o

$(BUILD)/bin/libmpr.so: $(DEPS_58)
	@echo '      [Link] $(BUILD)/bin/libmpr.so'
	$(CC) -shared -o $(BUILD)/bin/libmpr.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/mprLib.o" $(LIBS) 

ifeq ($(ME_COM_PCRE),1)
#
#   libpcre
#
DEPS_59 += $(BUILD)/inc/pcre.h
DEPS_59 += $(BUILD)/obj/pcre.o

$(BUILD)/bin/libpcre.so: $(DEPS_59)
	@echo '      [Link] $(BUILD)/bin/libpcre.so'
	$(CC) -shared -o $(BUILD)/bin/libpcre.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/pcre.o" $(LIBS) 
endif

ifeq ($(ME_COM_HTTP),1)
#
#   libhttp
#
DEPS_60 += $(BUILD)/bin/libmpr.so
ifeq ($(ME_COM_PCRE),1)
    DEPS_60 += $(BUILD)/bin/libpcre.so
endif
DEPS_60 += $(BUILD)/inc/http.h
DEPS_60 += $(BUILD)/obj/httpLib.o

LIBS_60 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_60 += -lpcre
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_60 += -lpcre
endif
LIBS_60 += -lmpr

$(BUILD)/bin/libhttp.so: $(DEPS_60)
	@echo '      [Link] $(BUILD)/bin/libhttp.so'
	$(CC) -shared -o $(BUILD)/bin/libhttp.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/httpLib.o" $(LIBPATHS_60) $(LIBS_60) $(LIBS_60) $(LIBS) 
endif

#
#   libappweb
#
ifeq ($(ME_COM_HTTP),1)
    DEPS_61 += $(BUILD)/bin/libhttp.so
endif
DEPS_61 += $(BUILD)/bin/libmpr.so
DEPS_61 += $(BUILD)/inc/appweb.h
DEPS_61 += $(BUILD)/inc/customize.h
DEPS_61 += $(BUILD)/obj/config.o
DEPS_61 += $(BUILD)/obj/convenience.o
DEPS_61 += $(BUILD)/obj/dirHandler.o
DEPS_61 += $(BUILD)/obj/fileHandler.o
DEPS_61 += $(BUILD)/obj/log.o
DEPS_61 += $(BUILD)/obj/server.o

LIBS_61 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_61 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_61 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_61 += -lpcre
endif
LIBS_61 += -lmpr
ifeq ($(ME_COM_HTTP),1)
    LIBS_61 += -lhttp
endif

$(BUILD)/bin/libappweb.so: $(DEPS_61)
	@echo '      [Link] $(BUILD)/bin/libappweb.so'
	$(CC) -shared -o $(BUILD)/bin/libappweb.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/config.o" "$(BUILD)/obj/convenience.o" "$(BUILD)/obj/dirHandler.o" "$(BUILD)/obj/fileHandler.o" "$(BUILD)/obj/log.o" "$(BUILD)/obj/server.o" $(LIBPATHS_61) $(LIBS_61) $(LIBS_61) $(LIBS) 

#
#   slink.c
#

src/slink.c: $(DEPS_62)
	( \
	cd src; \
	[ ! -f slink.c ] && cp slink.empty slink.c ; true ; \
	)

#
#   libslink
#
DEPS_63 += src/slink.c
DEPS_63 += $(BUILD)/obj/slink.o

$(BUILD)/bin/libslink.so: $(DEPS_63)
	@echo '      [Link] $(BUILD)/bin/libslink.so'
	$(CC) -shared -o $(BUILD)/bin/libslink.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/slink.o" $(LIBS) 

#
#   appweb
#
DEPS_64 += $(BUILD)/bin/libappweb.so
DEPS_64 += $(BUILD)/bin/libslink.so
DEPS_64 += $(BUILD)/obj/appweb.o

LIBS_64 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_64 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_64 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_64 += -lpcre
endif
LIBS_64 += -lmpr
LIBS_64 += -lappweb
ifeq ($(ME_COM_HTTP),1)
    LIBS_64 += -lhttp
endif
LIBS_64 += -lslink

$(BUILD)/bin/appweb: $(DEPS_64)
	@echo '      [Link] $(BUILD)/bin/appweb'
	$(CC) -o $(BUILD)/bin/appweb $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/appweb.o" $(LIBPATHS_64) $(LIBS_64) $(LIBS_64) $(LIBS) $(LIBS) 

#
#   authpass
#
DEPS_65 += $(BUILD)/bin/libappweb.so
DEPS_65 += $(BUILD)/obj/authpass.o

LIBS_65 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_65 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_65 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_65 += -lpcre
endif
LIBS_65 += -lmpr
LIBS_65 += -lappweb
ifeq ($(ME_COM_HTTP),1)
    LIBS_65 += -lhttp
endif

$(BUILD)/bin/authpass: $(DEPS_65)
	@echo '      [Link] $(BUILD)/bin/authpass'
	$(CC) -o $(BUILD)/bin/authpass $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/authpass.o" $(LIBPATHS_65) $(LIBS_65) $(LIBS_65) $(LIBS) $(LIBS) 

ifeq ($(ME_COM_CGI),1)
#
#   cgiProgram
#
DEPS_66 += $(BUILD)/obj/cgiProgram.o

$(BUILD)/bin/cgiProgram: $(DEPS_66)
	@echo '      [Link] $(BUILD)/bin/cgiProgram'
	$(CC) -o $(BUILD)/bin/cgiProgram $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/cgiProgram.o" $(LIBS) $(LIBS) 
endif

ifeq ($(ME_COM_ZLIB),1)
#
#   libzlib
#
DEPS_67 += $(BUILD)/inc/zlib.h
DEPS_67 += $(BUILD)/obj/zlib.o

$(BUILD)/bin/libzlib.so: $(DEPS_67)
	@echo '      [Link] $(BUILD)/bin/libzlib.so'
	$(CC) -shared -o $(BUILD)/bin/libzlib.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/zlib.o" $(LIBS) 
endif

ifeq ($(ME_COM_EJS),1)
#
#   libejs
#
ifeq ($(ME_COM_HTTP),1)
    DEPS_68 += $(BUILD)/bin/libhttp.so
endif
ifeq ($(ME_COM_PCRE),1)
    DEPS_68 += $(BUILD)/bin/libpcre.so
endif
DEPS_68 += $(BUILD)/bin/libmpr.so
ifeq ($(ME_COM_ZLIB),1)
    DEPS_68 += $(BUILD)/bin/libzlib.so
endif
DEPS_68 += $(BUILD)/inc/ejs.h
DEPS_68 += $(BUILD)/inc/ejs.slots.h
DEPS_68 += $(BUILD)/inc/ejsByteGoto.h
DEPS_68 += $(BUILD)/obj/ejsLib.o
ifeq ($(ME_COM_SQLITE),1)
    DEPS_68 += $(BUILD)/bin/libsql.so
endif

LIBS_68 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_68 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_68 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_68 += -lpcre
endif
LIBS_68 += -lmpr
ifeq ($(ME_COM_ZLIB),1)
    LIBS_68 += -lzlib
endif
ifeq ($(ME_COM_ZLIB),1)
    LIBS_68 += -lzlib
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_68 += -lhttp
endif

$(BUILD)/bin/libejs.so: $(DEPS_68)
	@echo '      [Link] $(BUILD)/bin/libejs.so'
	$(CC) -shared -o $(BUILD)/bin/libejs.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/ejsLib.o" $(LIBPATHS_68) $(LIBS_68) $(LIBS_68) $(LIBS) 
endif

ifeq ($(ME_COM_EJS),1)
#
#   ejsc
#
DEPS_69 += $(BUILD)/bin/libejs.so
ifeq ($(ME_COM_SQLITE),1)
    DEPS_69 += $(BUILD)/bin/libsql.so
endif
DEPS_69 += $(BUILD)/obj/ejsc.o

LIBS_69 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_69 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_69 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_69 += -lpcre
endif
LIBS_69 += -lmpr
ifeq ($(ME_COM_ZLIB),1)
    LIBS_69 += -lzlib
endif
LIBS_69 += -lejs
ifeq ($(ME_COM_ZLIB),1)
    LIBS_69 += -lzlib
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_69 += -lhttp
endif

$(BUILD)/bin/ejsc: $(DEPS_69)
	@echo '      [Link] $(BUILD)/bin/ejsc'
	$(CC) -o $(BUILD)/bin/ejsc $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/ejsc.o" $(LIBPATHS_69) $(LIBS_69) $(LIBS_69) $(LIBS) $(LIBS) 
endif

ifeq ($(ME_COM_EJS),1)
#
#   ejs.mod
#
DEPS_70 += src/ejs/ejs.es
DEPS_70 += $(BUILD)/bin/ejsc

$(BUILD)/bin/ejs.mod: $(DEPS_70)
	( \
	cd src/ejs; \
	../../$(BUILD)/bin/ejsc --out ../../$(BUILD)/bin/ejs.mod --optimize 9 --bind --require null ejs.es ; \
	)
endif

ifeq ($(ME_COM_EJS),1)
#
#   ejscmd
#
DEPS_71 += $(BUILD)/bin/libejs.so
ifeq ($(ME_COM_SQLITE),1)
    DEPS_71 += $(BUILD)/bin/libsql.so
endif
DEPS_71 += $(BUILD)/obj/ejs.o

LIBS_71 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_71 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_71 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_71 += -lpcre
endif
LIBS_71 += -lmpr
ifeq ($(ME_COM_ZLIB),1)
    LIBS_71 += -lzlib
endif
LIBS_71 += -lejs
ifeq ($(ME_COM_ZLIB),1)
    LIBS_71 += -lzlib
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_71 += -lhttp
endif

$(BUILD)/bin/ejs: $(DEPS_71)
	@echo '      [Link] $(BUILD)/bin/ejs'
	$(CC) -o $(BUILD)/bin/ejs $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/ejs.o" $(LIBPATHS_71) $(LIBS_71) $(LIBS_71) $(LIBS) $(LIBS) 
endif

ifeq ($(ME_COM_ESP),1)
#
#   esp-paks
#
DEPS_72 += src/esp-html-mvc/client/assets/favicon.ico
DEPS_72 += src/esp-html-mvc/client/css/all.css
DEPS_72 += src/esp-html-mvc/client/css/all.less
DEPS_72 += src/esp-html-mvc/client/index.esp
DEPS_72 += src/esp-html-mvc/css/app.less
DEPS_72 += src/esp-html-mvc/css/theme.less
DEPS_72 += src/esp-html-mvc/generate/appweb.conf
DEPS_72 += src/esp-html-mvc/generate/controller.c
DEPS_72 += src/esp-html-mvc/generate/controllerSingleton.c
DEPS_72 += src/esp-html-mvc/generate/edit.esp
DEPS_72 += src/esp-html-mvc/generate/list.esp
DEPS_72 += src/esp-html-mvc/layouts/default.esp
DEPS_72 += src/esp-html-mvc/package.json
DEPS_72 += src/esp-legacy-mvc/generate/appweb.conf
DEPS_72 += src/esp-legacy-mvc/generate/controller.c
DEPS_72 += src/esp-legacy-mvc/generate/edit.esp
DEPS_72 += src/esp-legacy-mvc/generate/list.esp
DEPS_72 += src/esp-legacy-mvc/generate/migration.c
DEPS_72 += src/esp-legacy-mvc/generate/src/app.c
DEPS_72 += src/esp-legacy-mvc/layouts/default.esp
DEPS_72 += src/esp-legacy-mvc/package.json
DEPS_72 += src/esp-legacy-mvc/static/css/all.css
DEPS_72 += src/esp-legacy-mvc/static/images/banner.jpg
DEPS_72 += src/esp-legacy-mvc/static/images/favicon.ico
DEPS_72 += src/esp-legacy-mvc/static/images/splash.jpg
DEPS_72 += src/esp-legacy-mvc/static/index.esp
DEPS_72 += src/esp-legacy-mvc/static/js/jquery.esp.js
DEPS_72 += src/esp-legacy-mvc/static/js/jquery.js
DEPS_72 += src/esp-mvc/generate/appweb.conf
DEPS_72 += src/esp-mvc/generate/controller.c
DEPS_72 += src/esp-mvc/generate/migration.c
DEPS_72 += src/esp-mvc/generate/src/app.c
DEPS_72 += src/esp-mvc/LICENSE.md
DEPS_72 += src/esp-mvc/package.json
DEPS_72 += src/esp-mvc/README.md
DEPS_72 += src/esp-server/generate/appweb.conf
DEPS_72 += src/esp-server/package.json

$(BUILD)/esp: $(DEPS_72)
	mkdir -p "$(BUILD)/esp/paks/esp-html-mvc/4.7.3" ; \
	mkdir -p "$(BUILD)/esp/paks/esp-html-mvc/4.7.3/client" ; \
	mkdir -p "$(BUILD)/esp/paks/esp-html-mvc/4.7.3/client/assets" ; \
	cp paks/esp-html-mvc/client/assets/favicon.ico $(BUILD)/esp/paks/esp-html-mvc/4.7.3/client/assets/favicon.ico ; \
	mkdir -p "$(BUILD)/esp/paks/esp-html-mvc/4.7.3/client/css" ; \
	cp paks/esp-html-mvc/client/css/all.css $(BUILD)/esp/paks/esp-html-mvc/4.7.3/client/css/all.css ; \
	cp paks/esp-html-mvc/client/css/all.less $(BUILD)/esp/paks/esp-html-mvc/4.7.3/client/css/all.less ; \
	cp paks/esp-html-mvc/client/index.esp $(BUILD)/esp/paks/esp-html-mvc/4.7.3/client/index.esp ; \
	mkdir -p "$(BUILD)/esp/paks/esp-html-mvc/4.7.3/css" ; \
	cp paks/esp-html-mvc/css/app.less $(BUILD)/esp/paks/esp-html-mvc/4.7.3/css/app.less ; \
	cp paks/esp-html-mvc/css/theme.less $(BUILD)/esp/paks/esp-html-mvc/4.7.3/css/theme.less ; \
	mkdir -p "$(BUILD)/esp/paks/esp-html-mvc/4.7.3/generate" ; \
	cp paks/esp-html-mvc/generate/appweb.conf $(BUILD)/esp/paks/esp-html-mvc/4.7.3/generate/appweb.conf ; \
	cp paks/esp-html-mvc/generate/controller.c $(BUILD)/esp/paks/esp-html-mvc/4.7.3/generate/controller.c ; \
	cp paks/esp-html-mvc/generate/controllerSingleton.c $(BUILD)/esp/paks/esp-html-mvc/4.7.3/generate/controllerSingleton.c ; \
	cp paks/esp-html-mvc/generate/edit.esp $(BUILD)/esp/paks/esp-html-mvc/4.7.3/generate/edit.esp ; \
	cp paks/esp-html-mvc/generate/list.esp $(BUILD)/esp/paks/esp-html-mvc/4.7.3/generate/list.esp ; \
	mkdir -p "$(BUILD)/esp/paks/esp-html-mvc/4.7.3/layouts" ; \
	cp paks/esp-html-mvc/layouts/default.esp $(BUILD)/esp/paks/esp-html-mvc/4.7.3/layouts/default.esp ; \
	cp paks/esp-html-mvc/package.json $(BUILD)/esp/paks/esp-html-mvc/4.7.3/package.json ; \
	mkdir -p "$(BUILD)/esp/paks/esp-legacy-mvc/4.7.3" ; \
	mkdir -p "$(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/generate" ; \
	cp paks/esp-legacy-mvc/generate/appweb.conf $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/generate/appweb.conf ; \
	cp paks/esp-legacy-mvc/generate/controller.c $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/generate/controller.c ; \
	cp paks/esp-legacy-mvc/generate/edit.esp $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/generate/edit.esp ; \
	cp paks/esp-legacy-mvc/generate/list.esp $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/generate/list.esp ; \
	cp paks/esp-legacy-mvc/generate/migration.c $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/generate/migration.c ; \
	mkdir -p "$(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/generate/src" ; \
	cp paks/esp-legacy-mvc/generate/src/app.c $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/generate/src/app.c ; \
	mkdir -p "$(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/layouts" ; \
	cp paks/esp-legacy-mvc/layouts/default.esp $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/layouts/default.esp ; \
	cp paks/esp-legacy-mvc/package.json $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/package.json ; \
	mkdir -p "$(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/static" ; \
	mkdir -p "$(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/static/css" ; \
	cp paks/esp-legacy-mvc/static/css/all.css $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/static/css/all.css ; \
	mkdir -p "$(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/static/images" ; \
	cp paks/esp-legacy-mvc/static/images/banner.jpg $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/static/images/banner.jpg ; \
	cp paks/esp-legacy-mvc/static/images/favicon.ico $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/static/images/favicon.ico ; \
	cp paks/esp-legacy-mvc/static/images/splash.jpg $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/static/images/splash.jpg ; \
	cp paks/esp-legacy-mvc/static/index.esp $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/static/index.esp ; \
	mkdir -p "$(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/static/js" ; \
	cp paks/esp-legacy-mvc/static/js/jquery.esp.js $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/static/js/jquery.esp.js ; \
	cp paks/esp-legacy-mvc/static/js/jquery.js $(BUILD)/esp/paks/esp-legacy-mvc/4.7.3/static/js/jquery.js ; \
	mkdir -p "$(BUILD)/esp/paks/esp-mvc/4.7.3" ; \
	mkdir -p "$(BUILD)/esp/paks/esp-mvc/4.7.3/generate" ; \
	cp paks/esp-mvc/generate/appweb.conf $(BUILD)/esp/paks/esp-mvc/4.7.3/generate/appweb.conf ; \
	cp paks/esp-mvc/generate/controller.c $(BUILD)/esp/paks/esp-mvc/4.7.3/generate/controller.c ; \
	cp paks/esp-mvc/generate/migration.c $(BUILD)/esp/paks/esp-mvc/4.7.3/generate/migration.c ; \
	mkdir -p "$(BUILD)/esp/paks/esp-mvc/4.7.3/generate/src" ; \
	cp paks/esp-mvc/generate/src/app.c $(BUILD)/esp/paks/esp-mvc/4.7.3/generate/src/app.c ; \
	cp paks/esp-mvc/LICENSE.md $(BUILD)/esp/paks/esp-mvc/4.7.3/LICENSE.md ; \
	cp paks/esp-mvc/package.json $(BUILD)/esp/paks/esp-mvc/4.7.3/package.json ; \
	cp paks/esp-mvc/README.md $(BUILD)/esp/paks/esp-mvc/4.7.3/README.md ; \
	mkdir -p "$(BUILD)/esp/paks/esp-server/4.7.3" ; \
	mkdir -p "$(BUILD)/esp/paks/esp-server/4.7.3/generate" ; \
	cp paks/esp-server/generate/appweb.conf $(BUILD)/esp/paks/esp-server/4.7.3/generate/appweb.conf ; \
	cp paks/esp-server/package.json $(BUILD)/esp/paks/esp-server/4.7.3/package.json
endif

ifeq ($(ME_COM_ESP),1)
#
#   esp.conf
#
DEPS_73 += src/esp/esp.conf

$(BUILD)/bin/esp.conf: $(DEPS_73)
	@echo '      [Copy] $(BUILD)/bin/esp.conf'
	mkdir -p "$(BUILD)/bin"
	cp src/esp/esp.conf $(BUILD)/bin/esp.conf
endif

ifeq ($(ME_COM_ESP),1)
#
#   libmod_esp
#
DEPS_74 += $(BUILD)/bin/libappweb.so
DEPS_74 += $(BUILD)/inc/esp.h
DEPS_74 += $(BUILD)/obj/espLib.o
ifeq ($(ME_COM_SQLITE),1)
    DEPS_74 += $(BUILD)/bin/libsql.so
endif

LIBS_74 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_74 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_74 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_74 += -lpcre
endif
LIBS_74 += -lmpr
LIBS_74 += -lappweb
ifeq ($(ME_COM_HTTP),1)
    LIBS_74 += -lhttp
endif
LIBS_74 += -lappweb

$(BUILD)/bin/libmod_esp.so: $(DEPS_74)
	@echo '      [Link] $(BUILD)/bin/libmod_esp.so'
	$(CC) -shared -o $(BUILD)/bin/libmod_esp.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/espLib.o" $(LIBPATHS_74) $(LIBS_74) $(LIBS_74) $(LIBS) 
endif

ifeq ($(ME_COM_ESP),1)
#
#   espcmd
#
DEPS_75 += $(BUILD)/bin/libmod_esp.so
ifeq ($(ME_COM_SQLITE),1)
    DEPS_75 += $(BUILD)/bin/libsql.so
endif
DEPS_75 += $(BUILD)/obj/esp.o

LIBS_75 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_75 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_75 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_75 += -lpcre
endif
LIBS_75 += -lmpr
LIBS_75 += -lappweb
ifeq ($(ME_COM_HTTP),1)
    LIBS_75 += -lhttp
endif
LIBS_75 += -lmod_esp
LIBS_75 += -lappweb

$(BUILD)/bin/esp: $(DEPS_75)
	@echo '      [Link] $(BUILD)/bin/esp'
	$(CC) -o $(BUILD)/bin/esp $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/esp.o" $(LIBPATHS_75) $(LIBS_75) $(LIBS_75) $(LIBS) $(LIBS) 
endif

#
#   genslink
#

genslink: $(DEPS_76)
	( \
	cd src; \
	esp --static --genlink slink.c compile ; \
	)

#
#   http-ca-crt
#
DEPS_77 += src/http/ca.crt

$(BUILD)/bin/ca.crt: $(DEPS_77)
	@echo '      [Copy] $(BUILD)/bin/ca.crt'
	mkdir -p "$(BUILD)/bin"
	cp src/http/ca.crt $(BUILD)/bin/ca.crt

ifeq ($(ME_COM_HTTP),1)
#
#   httpcmd
#
DEPS_78 += $(BUILD)/bin/libhttp.so
DEPS_78 += $(BUILD)/obj/http.o

LIBS_78 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_78 += -lpcre
endif
LIBS_78 += -lhttp
ifeq ($(ME_COM_PCRE),1)
    LIBS_78 += -lpcre
endif
LIBS_78 += -lmpr

$(BUILD)/bin/http: $(DEPS_78)
	@echo '      [Link] $(BUILD)/bin/http'
	$(CC) -o $(BUILD)/bin/http $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/http.o" $(LIBPATHS_78) $(LIBS_78) $(LIBS_78) $(LIBS) $(LIBS) 
endif

ifeq ($(ME_COM_EST),1)
#
#   libest
#
DEPS_79 += $(BUILD)/inc/osdep.h
DEPS_79 += $(BUILD)/inc/est.h
DEPS_79 += $(BUILD)/obj/estLib.o

$(BUILD)/bin/libest.so: $(DEPS_79)
	@echo '      [Link] $(BUILD)/bin/libest.so'
	$(CC) -shared -o $(BUILD)/bin/libest.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/estLib.o" $(LIBS) 
endif

ifeq ($(ME_COM_CGI),1)
#
#   libmod_cgi
#
DEPS_80 += $(BUILD)/bin/libappweb.so
DEPS_80 += $(BUILD)/obj/cgiHandler.o

LIBS_80 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_80 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_80 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_80 += -lpcre
endif
LIBS_80 += -lmpr
LIBS_80 += -lappweb
ifeq ($(ME_COM_HTTP),1)
    LIBS_80 += -lhttp
endif
LIBS_80 += -lappweb

$(BUILD)/bin/libmod_cgi.so: $(DEPS_80)
	@echo '      [Link] $(BUILD)/bin/libmod_cgi.so'
	$(CC) -shared -o $(BUILD)/bin/libmod_cgi.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/cgiHandler.o" $(LIBPATHS_80) $(LIBS_80) $(LIBS_80) $(LIBS) 
endif

ifeq ($(ME_COM_EJS),1)
#
#   libmod_ejs
#
DEPS_81 += $(BUILD)/bin/libappweb.so
DEPS_81 += $(BUILD)/bin/libejs.so
ifeq ($(ME_COM_SQLITE),1)
    DEPS_81 += $(BUILD)/bin/libsql.so
endif
DEPS_81 += $(BUILD)/obj/ejsHandler.o

LIBS_81 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_81 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_81 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_81 += -lpcre
endif
LIBS_81 += -lmpr
LIBS_81 += -lappweb
ifeq ($(ME_COM_HTTP),1)
    LIBS_81 += -lhttp
endif
ifeq ($(ME_COM_ZLIB),1)
    LIBS_81 += -lzlib
endif
LIBS_81 += -lejs
ifeq ($(ME_COM_ZLIB),1)
    LIBS_81 += -lzlib
endif
LIBS_81 += -lejs
LIBS_81 += -lappweb

$(BUILD)/bin/libmod_ejs.so: $(DEPS_81)
	@echo '      [Link] $(BUILD)/bin/libmod_ejs.so'
	$(CC) -shared -o $(BUILD)/bin/libmod_ejs.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/ejsHandler.o" $(LIBPATHS_81) $(LIBS_81) $(LIBS_81) $(LIBS) 
endif

ifeq ($(ME_COM_PHP),1)
#
#   libmod_php
#
DEPS_82 += $(BUILD)/bin/libappweb.so
DEPS_82 += $(BUILD)/obj/phpHandler.o

LIBS_82 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_82 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_82 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_82 += -lpcre
endif
LIBS_82 += -lmpr
LIBS_82 += -lappweb
ifeq ($(ME_COM_HTTP),1)
    LIBS_82 += -lhttp
endif
LIBS_82 += -lappweb

$(BUILD)/bin/libmod_php.so: $(DEPS_82)
	@echo '      [Link] $(BUILD)/bin/libmod_php.so'
	$(CC) -shared -o $(BUILD)/bin/libmod_php.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/phpHandler.o" $(LIBPATHS_82) $(LIBS_82) $(LIBS_82) $(LIBS) 
endif

#
#   libmprssl
#
DEPS_83 += $(BUILD)/bin/libmpr.so
ifeq ($(ME_COM_EST),1)
    DEPS_83 += $(BUILD)/bin/libest.so
endif
DEPS_83 += $(BUILD)/obj/mprSsl.o

LIBS_83 += -lmpr
ifeq ($(ME_COM_EST),1)
    LIBS_83 += -lest
endif
ifeq ($(ME_COM_OPENSSL),1)
ifeq ($(ME_COM_SSL),1)
    LIBS_83 += -lssl
    LIBPATHS_83 += -L"$(ME_COM_OPENSSL_PATH)"
endif
endif
ifeq ($(ME_COM_OPENSSL),1)
    LIBS_83 += -lcrypto
    LIBPATHS_83 += -L"$(ME_COM_OPENSSL_PATH)"
endif
LIBS_83 += -lmpr

$(BUILD)/bin/libmprssl.so: $(DEPS_83)
	@echo '      [Link] $(BUILD)/bin/libmprssl.so'
	$(CC) -shared -o $(BUILD)/bin/libmprssl.so $(LDFLAGS) $(LIBPATHS)  "$(BUILD)/obj/mprSsl.o" $(LIBPATHS_83) $(LIBS_83) $(LIBS_83) $(LIBS) 

ifeq ($(ME_COM_SSL),1)
#
#   libmod_ssl
#
DEPS_84 += $(BUILD)/bin/libappweb.so
DEPS_84 += $(BUILD)/bin/libmprssl.so
DEPS_84 += $(BUILD)/obj/sslModule.o

LIBS_84 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_84 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_84 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_84 += -lpcre
endif
LIBS_84 += -lmpr
LIBS_84 += -lappweb
ifeq ($(ME_COM_HTTP),1)
    LIBS_84 += -lhttp
endif
ifeq ($(ME_COM_EST),1)
    LIBS_84 += -lest
endif
LIBS_84 += -lmprssl
ifeq ($(ME_COM_OPENSSL),1)
    LIBS_84 += -lssl
    LIBPATHS_84 += -L"$(ME_COM_OPENSSL_PATH)"
endif
ifeq ($(ME_COM_OPENSSL),1)
    LIBS_84 += -lcrypto
    LIBPATHS_84 += -L"$(ME_COM_OPENSSL_PATH)"
endif
LIBS_84 += -lmprssl
LIBS_84 += -lappweb

$(BUILD)/bin/libmod_ssl.so: $(DEPS_84)
	@echo '      [Link] $(BUILD)/bin/libmod_ssl.so'
	$(CC) -shared -o $(BUILD)/bin/libmod_ssl.so $(LDFLAGS) $(LIBPATHS)  "$(BUILD)/obj/sslModule.o" $(LIBPATHS_84) $(LIBS_84) $(LIBS_84) $(LIBS) 
endif

ifeq ($(ME_COM_SQLITE),1)
#
#   libsql
#
DEPS_85 += $(BUILD)/inc/sqlite3.h
DEPS_85 += $(BUILD)/obj/sqlite3.o

$(BUILD)/bin/libsql.so: $(DEPS_85)
	@echo '      [Link] $(BUILD)/bin/libsql.so'
	$(CC) -shared -o $(BUILD)/bin/libsql.so $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/sqlite3.o" $(LIBS) 
endif

#
#   makerom
#
DEPS_86 += $(BUILD)/bin/libmpr.so
DEPS_86 += $(BUILD)/obj/makerom.o

LIBS_86 += -lmpr

$(BUILD)/bin/makerom: $(DEPS_86)
	@echo '      [Link] $(BUILD)/bin/makerom'
	$(CC) -o $(BUILD)/bin/makerom $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/makerom.o" $(LIBPATHS_86) $(LIBS_86) $(LIBS_86) $(LIBS) $(LIBS) 

#
#   manager
#
DEPS_87 += $(BUILD)/bin/libmpr.so
DEPS_87 += $(BUILD)/obj/manager.o

LIBS_87 += -lmpr

$(BUILD)/bin/appman: $(DEPS_87)
	@echo '      [Link] $(BUILD)/bin/appman'
	$(CC) -o $(BUILD)/bin/appman $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/manager.o" $(LIBPATHS_87) $(LIBS_87) $(LIBS_87) $(LIBS) $(LIBS) 

#
#   server-cache
#

src/server/cache: $(DEPS_88)
	( \
	cd src/server; \
	mkdir -p cache ; \
	)

ifeq ($(ME_COM_SQLITE),1)
#
#   sqliteshell
#
DEPS_89 += $(BUILD)/bin/libsql.so
DEPS_89 += $(BUILD)/obj/sqlite.o

LIBS_89 += -lsql

$(BUILD)/bin/sqlite: $(DEPS_89)
	@echo '      [Link] $(BUILD)/bin/sqlite'
	$(CC) -o $(BUILD)/bin/sqlite $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/sqlite.o" $(LIBPATHS_89) $(LIBS_89) $(LIBS_89) $(LIBS) $(LIBS) 
endif

#
#   testAppweb
#
DEPS_90 += $(BUILD)/bin/libappweb.so
DEPS_90 += $(BUILD)/inc/testAppweb.h
DEPS_90 += $(BUILD)/obj/testAppweb.o
DEPS_90 += $(BUILD)/obj/testHttp.o

LIBS_90 += -lmpr
ifeq ($(ME_COM_PCRE),1)
    LIBS_90 += -lpcre
endif
ifeq ($(ME_COM_HTTP),1)
    LIBS_90 += -lhttp
endif
ifeq ($(ME_COM_PCRE),1)
    LIBS_90 += -lpcre
endif
LIBS_90 += -lmpr
LIBS_90 += -lappweb
ifeq ($(ME_COM_HTTP),1)
    LIBS_90 += -lhttp
endif

$(BUILD)/bin/testAppweb: $(DEPS_90)
	@echo '      [Link] $(BUILD)/bin/testAppweb'
	$(CC) -o $(BUILD)/bin/testAppweb $(LDFLAGS) $(LIBPATHS) "$(BUILD)/obj/testAppweb.o" "$(BUILD)/obj/testHttp.o" $(LIBPATHS_90) $(LIBS_90) $(LIBS_90) $(LIBS) $(LIBS) 

ifeq ($(ME_COM_CGI),1)
#
#   test-basic.cgi
#
DEPS_91 += $(BUILD)/bin/testAppweb

test/web/auth/basic/basic.cgi: $(DEPS_91)
	( \
	cd test; \
	echo "#!`type -p ejs`" >web/auth/basic/basic.cgi ; \
	echo 'print("HTTP/1.0 200 OK\nContent-Type: text/plain\n\n" + serialize(App.env, {pretty: true}) + "\n")' >>web/auth/basic/basic.cgi ; \
	chmod +x web/auth/basic/basic.cgi ; \
	)
endif

ifeq ($(ME_COM_CGI),1)
#
#   test-cache.cgi
#
DEPS_92 += $(BUILD)/bin/testAppweb

test/web/caching/cache.cgi: $(DEPS_92)
	( \
	cd test; \
	echo "#!`type -p ejs`" >web/caching/cache.cgi ; \
	echo 'print("HTTP/1.0 200 OK\nContent-Type: text/plain\n\n{number:" + Date().now() + "}\n")' >>web/caching/cache.cgi ; \
	chmod +x web/caching/cache.cgi ; \
	)
endif

ifeq ($(ME_COM_CGI),1)
#
#   test-cgiProgram
#
DEPS_93 += $(BUILD)/bin/cgiProgram

test/cgi-bin/cgiProgram: $(DEPS_93)
	( \
	cd test; \
	cp ../$(BUILD)/bin/cgiProgram cgi-bin/cgiProgram ; \
	cp ../$(BUILD)/bin/cgiProgram cgi-bin/nph-cgiProgram ; \
	cp ../$(BUILD)/bin/cgiProgram 'cgi-bin/cgi Program' ; \
	cp ../$(BUILD)/bin/cgiProgram web/cgiProgram.cgi ; \
	chmod +x cgi-bin/* web/cgiProgram.cgi ; \
	)
endif

ifeq ($(ME_COM_CGI),1)
#
#   test-testScript
#
DEPS_94 += $(BUILD)/bin/testAppweb

test/cgi-bin/testScript: $(DEPS_94)
	( \
	cd test; \
	echo '#!../$(BUILD)/bin/cgiProgram' >cgi-bin/testScript ; chmod +x cgi-bin/testScript ; \
	)
endif

#
#   stop
#

stop: $(DEPS_95)
	@./$(BUILD)/bin/appman stop disable uninstall >/dev/null 2>&1 ; true

#
#   installBinary
#

installBinary: $(DEPS_96)
	mkdir -p "$(ME_APP_PREFIX)" ; \
	rm -f "$(ME_APP_PREFIX)/latest" ; \
	ln -s "$(VERSION)" "$(ME_APP_PREFIX)/latest" ; \
	mkdir -p "$(ME_LOG_PREFIX)" ; \
	chmod 755 "$(ME_LOG_PREFIX)" ; \
	[ `id -u` = 0 ] && chown $(WEB_USER):$(WEB_GROUP) "$(ME_LOG_PREFIX)"; true ; \
	mkdir -p "$(ME_CACHE_PREFIX)" ; \
	chmod 755 "$(ME_CACHE_PREFIX)" ; \
	[ `id -u` = 0 ] && chown $(WEB_USER):$(WEB_GROUP) "$(ME_CACHE_PREFIX)"; true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/appweb $(ME_VAPP_PREFIX)/bin/appweb ; \
	mkdir -p "$(ME_BIN_PREFIX)" ; \
	rm -f "$(ME_BIN_PREFIX)/appweb" ; \
	ln -s "$(ME_VAPP_PREFIX)/bin/appweb" "$(ME_BIN_PREFIX)/appweb" ; \
	if [ "$(ME_COM_SSL)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp src/est/ca.crt $(ME_VAPP_PREFIX)/bin/ca.crt ; \
	fi ; \
	if [ "$(ME_COM_PHP)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/libphp5.so $(ME_VAPP_PREFIX)/bin/libphp5.so ; \
	fi ; \
	mkdir -p "$(ME_ETC_PREFIX)" ; \
	cp src/server/mime.types $(ME_ETC_PREFIX)/mime.types ; \
	mkdir -p "$(ME_ETC_PREFIX)" ; \
	cp src/server/self.crt $(ME_ETC_PREFIX)/self.crt ; \
	cp src/server/self.key $(ME_ETC_PREFIX)/self.key ; \
	mkdir -p "$(ME_ETC_PREFIX)" ; \
	cp src/server/appweb.conf $(ME_ETC_PREFIX)/appweb.conf ; \
	mkdir -p "$(ME_ETC_PREFIX)" ; \
	cp src/server/sample.conf $(ME_ETC_PREFIX)/sample.conf ; \
	mkdir -p "$(ME_ETC_PREFIX)" ; \
	cp src/server/self.crt $(ME_ETC_PREFIX)/self.crt ; \
	cp src/server/self.key $(ME_ETC_PREFIX)/self.key ; \
	echo 'set LOG_DIR "$(ME_LOG_PREFIX)"\nset CACHE_DIR "$(ME_CACHE_PREFIX)"\nDocuments "$(ME_WEB_PREFIX)\nListen 80\n<if SSL_MODULE>\nListenSecure 443\n</if>\n' >$(ME_ETC_PREFIX)/install.conf ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/libappweb.so $(ME_VAPP_PREFIX)/bin/libappweb.so ; \
	cp $(BUILD)/bin/libhttp.so $(ME_VAPP_PREFIX)/bin/libhttp.so ; \
	cp $(BUILD)/bin/libmpr.so $(ME_VAPP_PREFIX)/bin/libmpr.so ; \
	cp $(BUILD)/bin/libpcre.so $(ME_VAPP_PREFIX)/bin/libpcre.so ; \
	cp $(BUILD)/bin/libslink.so $(ME_VAPP_PREFIX)/bin/libslink.so ; \
	if [ "$(ME_COM_SSL)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/libmprssl.so $(ME_VAPP_PREFIX)/bin/libmprssl.so ; \
	cp $(BUILD)/bin/libmod_ssl.so $(ME_VAPP_PREFIX)/bin/libmod_ssl.so ; \
	fi ; \
	if [ "$(ME_COM_EST)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/libest.so $(ME_VAPP_PREFIX)/bin/libest.so ; \
	fi ; \
	if [ "$(ME_COM_SQLITE)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/libsql.so $(ME_VAPP_PREFIX)/bin/libsql.so ; \
	fi ; \
	if [ "$(ME_COM_ESP)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/libmod_esp.so $(ME_VAPP_PREFIX)/bin/libmod_esp.so ; \
	fi ; \
	if [ "$(ME_COM_CGI)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/libmod_cgi.so $(ME_VAPP_PREFIX)/bin/libmod_cgi.so ; \
	fi ; \
	if [ "$(ME_COM_EJS)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/libejs.so $(ME_VAPP_PREFIX)/bin/libejs.so ; \
	cp $(BUILD)/bin/libmod_ejs.so $(ME_VAPP_PREFIX)/bin/libmod_ejs.so ; \
	cp $(BUILD)/bin/libzlib.so $(ME_VAPP_PREFIX)/bin/libzlib.so ; \
	fi ; \
	if [ "$(ME_COM_PHP)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/libmod_php.so $(ME_VAPP_PREFIX)/bin/libmod_php.so ; \
	fi ; \
	mkdir -p "$(ME_WEB_PREFIX)" ; \
	cp src/server/web/favicon.ico $(ME_WEB_PREFIX)/favicon.ico ; \
	cp src/server/web/iehacks.css $(ME_WEB_PREFIX)/iehacks.css ; \
	cp src/server/web/index.html $(ME_WEB_PREFIX)/index.html ; \
	cp src/server/web/min-index.html $(ME_WEB_PREFIX)/min-index.html ; \
	cp src/server/web/print.css $(ME_WEB_PREFIX)/print.css ; \
	cp src/server/web/screen.css $(ME_WEB_PREFIX)/screen.css ; \
	mkdir -p "$(ME_WEB_PREFIX)/test" ; \
	cp src/server/web/test/test.cgi $(ME_WEB_PREFIX)/test/test.cgi ; \
	chmod 755 "$(ME_WEB_PREFIX)/test/test.cgi" ; \
	cp src/server/web/test/test.pl $(ME_WEB_PREFIX)/test/test.pl ; \
	chmod 755 "$(ME_WEB_PREFIX)/test/test.pl" ; \
	cp src/server/web/test/test.py $(ME_WEB_PREFIX)/test/test.py ; \
	chmod 755 "$(ME_WEB_PREFIX)/test/test.py" ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/appman $(ME_VAPP_PREFIX)/bin/appman ; \
	mkdir -p "$(ME_BIN_PREFIX)" ; \
	rm -f "$(ME_BIN_PREFIX)/appman" ; \
	ln -s "$(ME_VAPP_PREFIX)/bin/appman" "$(ME_BIN_PREFIX)/appman" ; \
	if [ "$(ME_COM_ESP)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/esp $(ME_VAPP_PREFIX)/bin/appesp ; \
	mkdir -p "$(ME_BIN_PREFIX)" ; \
	rm -f "$(ME_BIN_PREFIX)/appesp" ; \
	ln -s "$(ME_VAPP_PREFIX)/bin/appesp" "$(ME_BIN_PREFIX)/appesp" ; \
	fi ; \
	if [ "$(ME_COM_ESP)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/angular/1.2.6" ; \
	cp ./paks/angular/angular-animate.js $(ME_VAPP_PREFIX)/esp/angular/1.2.6/angular-animate.js ; \
	cp ./paks/angular/angular-csp.css $(ME_VAPP_PREFIX)/esp/angular/1.2.6/angular-csp.css ; \
	cp ./paks/angular/angular-route.js $(ME_VAPP_PREFIX)/esp/angular/1.2.6/angular-route.js ; \
	cp ./paks/angular/angular.js $(ME_VAPP_PREFIX)/esp/angular/1.2.6/angular.js ; \
	cp ./paks/angular/package.json $(ME_VAPP_PREFIX)/esp/angular/1.2.6/package.json ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3" ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/client" ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/client/assets" ; \
	cp ./paks/esp-html-mvc/client/assets/favicon.ico $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/client/assets/favicon.ico ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/client/css" ; \
	cp ./paks/esp-html-mvc/client/css/all.css $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/client/css/all.css ; \
	cp ./paks/esp-html-mvc/client/css/all.less $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/client/css/all.less ; \
	cp ./paks/esp-html-mvc/client/index.esp $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/client/index.esp ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/css" ; \
	cp ./paks/esp-html-mvc/css/app.less $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/css/app.less ; \
	cp ./paks/esp-html-mvc/css/theme.less $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/css/theme.less ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/generate" ; \
	cp ./paks/esp-html-mvc/generate/appweb.conf $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/generate/appweb.conf ; \
	cp ./paks/esp-html-mvc/generate/controller.c $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/generate/controller.c ; \
	cp ./paks/esp-html-mvc/generate/controllerSingleton.c $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/generate/controllerSingleton.c ; \
	cp ./paks/esp-html-mvc/generate/edit.esp $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/generate/edit.esp ; \
	cp ./paks/esp-html-mvc/generate/list.esp $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/generate/list.esp ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/layouts" ; \
	cp ./paks/esp-html-mvc/layouts/default.esp $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/layouts/default.esp ; \
	cp ./paks/esp-html-mvc/package.json $(ME_VAPP_PREFIX)/esp/esp-html-mvc/4.7.3/package.json ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3" ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/generate" ; \
	cp ./paks/esp-legacy-mvc/generate/appweb.conf $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/generate/appweb.conf ; \
	cp ./paks/esp-legacy-mvc/generate/controller.c $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/generate/controller.c ; \
	cp ./paks/esp-legacy-mvc/generate/edit.esp $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/generate/edit.esp ; \
	cp ./paks/esp-legacy-mvc/generate/list.esp $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/generate/list.esp ; \
	cp ./paks/esp-legacy-mvc/generate/migration.c $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/generate/migration.c ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/generate/src" ; \
	cp ./paks/esp-legacy-mvc/generate/src/app.c $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/generate/src/app.c ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/layouts" ; \
	cp ./paks/esp-legacy-mvc/layouts/default.esp $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/layouts/default.esp ; \
	cp ./paks/esp-legacy-mvc/package.json $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/package.json ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/static" ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/static/css" ; \
	cp ./paks/esp-legacy-mvc/static/css/all.css $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/static/css/all.css ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/static/images" ; \
	cp ./paks/esp-legacy-mvc/static/images/banner.jpg $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/static/images/banner.jpg ; \
	cp ./paks/esp-legacy-mvc/static/images/favicon.ico $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/static/images/favicon.ico ; \
	cp ./paks/esp-legacy-mvc/static/images/splash.jpg $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/static/images/splash.jpg ; \
	cp ./paks/esp-legacy-mvc/static/index.esp $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/static/index.esp ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/static/js" ; \
	cp ./paks/esp-legacy-mvc/static/js/jquery.esp.js $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/static/js/jquery.esp.js ; \
	cp ./paks/esp-legacy-mvc/static/js/jquery.js $(ME_VAPP_PREFIX)/esp/esp-legacy-mvc/4.7.3/static/js/jquery.js ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-mvc/4.7.3" ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-mvc/4.7.3/generate" ; \
	cp ./paks/esp-mvc/generate/appweb.conf $(ME_VAPP_PREFIX)/esp/esp-mvc/4.7.3/generate/appweb.conf ; \
	cp ./paks/esp-mvc/generate/controller.c $(ME_VAPP_PREFIX)/esp/esp-mvc/4.7.3/generate/controller.c ; \
	cp ./paks/esp-mvc/generate/migration.c $(ME_VAPP_PREFIX)/esp/esp-mvc/4.7.3/generate/migration.c ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-mvc/4.7.3/generate/src" ; \
	cp ./paks/esp-mvc/generate/src/app.c $(ME_VAPP_PREFIX)/esp/esp-mvc/4.7.3/generate/src/app.c ; \
	cp ./paks/esp-mvc/LICENSE.md $(ME_VAPP_PREFIX)/esp/esp-mvc/4.7.3/LICENSE.md ; \
	cp ./paks/esp-mvc/package.json $(ME_VAPP_PREFIX)/esp/esp-mvc/4.7.3/package.json ; \
	cp ./paks/esp-mvc/README.md $(ME_VAPP_PREFIX)/esp/esp-mvc/4.7.3/README.md ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-server/4.7.3" ; \
	mkdir -p "$(ME_VAPP_PREFIX)/esp/esp-server/4.7.3/generate" ; \
	cp ./paks/esp-server/generate/appweb.conf $(ME_VAPP_PREFIX)/esp/esp-server/4.7.3/generate/appweb.conf ; \
	cp ./paks/esp-server/package.json $(ME_VAPP_PREFIX)/esp/esp-server/4.7.3/package.json ; \
	fi ; \
	if [ "$(ME_COM_ESP)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/esp.conf $(ME_VAPP_PREFIX)/bin/esp.conf ; \
	fi ; \
	if [ "$(ME_COM_EJS)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/ejs.mod $(ME_VAPP_PREFIX)/bin/ejs.mod ; \
	fi ; \
	if [ "$(ME_COM_PHP)" = 1 ]; then true ; \
	mkdir -p "$(ME_ETC_PREFIX)" ; \
	cp src/server/php.ini $(ME_ETC_PREFIX)/php.ini ; \
	fi ; \
	mkdir -p "$(ME_VAPP_PREFIX)/bin" ; \
	cp $(BUILD)/bin/http $(ME_VAPP_PREFIX)/bin/http ; \
	mkdir -p "$(ME_VAPP_PREFIX)/inc" ; \
	cp $(BUILD)/inc/me.h $(ME_VAPP_PREFIX)/inc/me.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/me.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/me.h" "$(ME_INC_PREFIX)/appweb/me.h" ; \
	cp src/osdep/osdep.h $(ME_VAPP_PREFIX)/inc/osdep.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/osdep.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/osdep.h" "$(ME_INC_PREFIX)/appweb/osdep.h" ; \
	cp src/appweb.h $(ME_VAPP_PREFIX)/inc/appweb.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/appweb.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/appweb.h" "$(ME_INC_PREFIX)/appweb/appweb.h" ; \
	cp src/customize.h $(ME_VAPP_PREFIX)/inc/customize.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/customize.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/customize.h" "$(ME_INC_PREFIX)/appweb/customize.h" ; \
	cp src/est/est.h $(ME_VAPP_PREFIX)/inc/est.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/est.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/est.h" "$(ME_INC_PREFIX)/appweb/est.h" ; \
	cp src/http/http.h $(ME_VAPP_PREFIX)/inc/http.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/http.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/http.h" "$(ME_INC_PREFIX)/appweb/http.h" ; \
	cp src/mpr/mpr.h $(ME_VAPP_PREFIX)/inc/mpr.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/mpr.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/mpr.h" "$(ME_INC_PREFIX)/appweb/mpr.h" ; \
	cp src/pcre/pcre.h $(ME_VAPP_PREFIX)/inc/pcre.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/pcre.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/pcre.h" "$(ME_INC_PREFIX)/appweb/pcre.h" ; \
	cp src/sqlite/sqlite3.h $(ME_VAPP_PREFIX)/inc/sqlite3.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/sqlite3.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/sqlite3.h" "$(ME_INC_PREFIX)/appweb/sqlite3.h" ; \
	if [ "$(ME_COM_ESP)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/inc" ; \
	cp src/esp/esp.h $(ME_VAPP_PREFIX)/inc/esp.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/esp.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/esp.h" "$(ME_INC_PREFIX)/appweb/esp.h" ; \
	fi ; \
	if [ "$(ME_COM_EJS)" = 1 ]; then true ; \
	mkdir -p "$(ME_VAPP_PREFIX)/inc" ; \
	cp src/ejs/ejs.h $(ME_VAPP_PREFIX)/inc/ejs.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/ejs.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/ejs.h" "$(ME_INC_PREFIX)/appweb/ejs.h" ; \
	cp src/ejs/ejs.slots.h $(ME_VAPP_PREFIX)/inc/ejs.slots.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/ejs.slots.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/ejs.slots.h" "$(ME_INC_PREFIX)/appweb/ejs.slots.h" ; \
	cp src/ejs/ejsByteGoto.h $(ME_VAPP_PREFIX)/inc/ejsByteGoto.h ; \
	mkdir -p "$(ME_INC_PREFIX)/appweb" ; \
	rm -f "$(ME_INC_PREFIX)/appweb/ejsByteGoto.h" ; \
	ln -s "$(ME_VAPP_PREFIX)/inc/ejsByteGoto.h" "$(ME_INC_PREFIX)/appweb/ejsByteGoto.h" ; \
	fi ; \
	mkdir -p "$(ME_VAPP_PREFIX)/doc/man1" ; \
	cp doc/contents/man/appman.1 $(ME_VAPP_PREFIX)/doc/man1/appman.1 ; \
	mkdir -p "$(ME_MAN_PREFIX)/man1" ; \
	rm -f "$(ME_MAN_PREFIX)/man1/appman.1" ; \
	ln -s "$(ME_VAPP_PREFIX)/doc/man1/appman.1" "$(ME_MAN_PREFIX)/man1/appman.1" ; \
	cp doc/contents/man/appweb.1 $(ME_VAPP_PREFIX)/doc/man1/appweb.1 ; \
	mkdir -p "$(ME_MAN_PREFIX)/man1" ; \
	rm -f "$(ME_MAN_PREFIX)/man1/appweb.1" ; \
	ln -s "$(ME_VAPP_PREFIX)/doc/man1/appweb.1" "$(ME_MAN_PREFIX)/man1/appweb.1" ; \
	cp doc/contents/man/appwebMonitor.1 $(ME_VAPP_PREFIX)/doc/man1/appwebMonitor.1 ; \
	mkdir -p "$(ME_MAN_PREFIX)/man1" ; \
	rm -f "$(ME_MAN_PREFIX)/man1/appwebMonitor.1" ; \
	ln -s "$(ME_VAPP_PREFIX)/doc/man1/appwebMonitor.1" "$(ME_MAN_PREFIX)/man1/appwebMonitor.1" ; \
	cp doc/contents/man/authpass.1 $(ME_VAPP_PREFIX)/doc/man1/authpass.1 ; \
	mkdir -p "$(ME_MAN_PREFIX)/man1" ; \
	rm -f "$(ME_MAN_PREFIX)/man1/authpass.1" ; \
	ln -s "$(ME_VAPP_PREFIX)/doc/man1/authpass.1" "$(ME_MAN_PREFIX)/man1/authpass.1" ; \
	cp doc/contents/man/esp.1 $(ME_VAPP_PREFIX)/doc/man1/esp.1 ; \
	mkdir -p "$(ME_MAN_PREFIX)/man1" ; \
	rm -f "$(ME_MAN_PREFIX)/man1/esp.1" ; \
	ln -s "$(ME_VAPP_PREFIX)/doc/man1/esp.1" "$(ME_MAN_PREFIX)/man1/esp.1" ; \
	cp doc/contents/man/http.1 $(ME_VAPP_PREFIX)/doc/man1/http.1 ; \
	mkdir -p "$(ME_MAN_PREFIX)/man1" ; \
	rm -f "$(ME_MAN_PREFIX)/man1/http.1" ; \
	ln -s "$(ME_VAPP_PREFIX)/doc/man1/http.1" "$(ME_MAN_PREFIX)/man1/http.1" ; \
	cp doc/contents/man/makerom.1 $(ME_VAPP_PREFIX)/doc/man1/makerom.1 ; \
	mkdir -p "$(ME_MAN_PREFIX)/man1" ; \
	rm -f "$(ME_MAN_PREFIX)/man1/makerom.1" ; \
	ln -s "$(ME_VAPP_PREFIX)/doc/man1/makerom.1" "$(ME_MAN_PREFIX)/man1/makerom.1" ; \
	cp doc/contents/man/manager.1 $(ME_VAPP_PREFIX)/doc/man1/manager.1 ; \
	mkdir -p "$(ME_MAN_PREFIX)/man1" ; \
	rm -f "$(ME_MAN_PREFIX)/man1/manager.1" ; \
	ln -s "$(ME_VAPP_PREFIX)/doc/man1/manager.1" "$(ME_MAN_PREFIX)/man1/manager.1"

#
#   start
#
DEPS_97 += stop

start: $(DEPS_97)
	./$(BUILD)/bin/appman install enable start

#
#   install
#
DEPS_98 += stop
DEPS_98 += installBinary
DEPS_98 += start

install: $(DEPS_98)

#
#   installPrep
#

installPrep: $(DEPS_99)
	if [ "`id -u`" != 0 ] ; \
	then echo "Must run as root. Rerun with "sudo"" ; \
	exit 255 ; \
	fi

#
#   run
#

run: $(DEPS_100)
	( \
	cd src/server; \
	sudo ../../$(BUILD)/bin/appweb -v ; \
	)

#
#   uninstall
#
DEPS_101 += stop

uninstall: $(DEPS_101)
	( \
	cd installs; \
	rm -fr "$(ME_WEB_PREFIX)" ; \
	rm -fr "$(ME_SPOOL_PREFIX)" ; \
	rm -fr "$(ME_CACHE_PREFIX)" ; \
	rm -fr "$(ME_LOG_PREFIX)" ; \
	rm -fr "$(ME_VAPP_PREFIX)" ; \
	rmdir -p "$(ME_ETC_PREFIX)" 2>/dev/null ; true ; \
	rmdir -p "$(ME_WEB_PREFIX)" 2>/dev/null ; true ; \
	rmdir -p "$(ME_LOG_PREFIX)" 2>/dev/null ; true ; \
	rmdir -p "$(ME_SPOOL_PREFIX)" 2>/dev/null ; true ; \
	rmdir -p "$(ME_CACHE_PREFIX)" 2>/dev/null ; true ; \
	rm -f "$(ME_APP_PREFIX)/latest" ; \
	rmdir -p "$(ME_APP_PREFIX)" 2>/dev/null ; true ; \
	rm -f "$(ME_ETC_PREFIX)/appweb.conf" ; \
	rm -f "$(ME_ETC_PREFIX)/esp.conf" ; \
	rm -f "$(ME_ETC_PREFIX)/mine.types" ; \
	rm -f "$(ME_ETC_PREFIX)/install.conf" ; \
	rm -fr "$(ME_INC_PREFIX)/appweb" ; \
	)

#
#   version
#

version: $(DEPS_102)
	echo $(VERSION)

