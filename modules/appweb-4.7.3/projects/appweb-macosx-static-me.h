/*
    me.h -- MakeMe Configure Header for macosx-x64-static

    This header is created by Me during configuration. To change settings, re-run
    configure or define variables in your Makefile to override these default values.
 */

/* Settings */
#ifndef ME_AUTHOR
    #define ME_AUTHOR "Embedthis Software"
#endif
#ifndef ME_COMPANY
    #define ME_COMPANY "embedthis"
#endif
#ifndef ME_COMPATIBLE
    #define ME_COMPATIBLE "4.7"
#endif
#ifndef ME_COMPILER_HAS_ATOMIC
    #define ME_COMPILER_HAS_ATOMIC 1
#endif
#ifndef ME_COMPILER_HAS_ATOMIC64
    #define ME_COMPILER_HAS_ATOMIC64 1
#endif
#ifndef ME_COMPILER_HAS_DOUBLE_BRACES
    #define ME_COMPILER_HAS_DOUBLE_BRACES 1
#endif
#ifndef ME_COMPILER_HAS_DYN_LOAD
    #define ME_COMPILER_HAS_DYN_LOAD 1
#endif
#ifndef ME_COMPILER_HAS_LIB_EDIT
    #define ME_COMPILER_HAS_LIB_EDIT 1
#endif
#ifndef ME_COMPILER_HAS_LIB_RT
    #define ME_COMPILER_HAS_LIB_RT 0
#endif
#ifndef ME_COMPILER_HAS_MMU
    #define ME_COMPILER_HAS_MMU 1
#endif
#ifndef ME_COMPILER_HAS_MTUNE
    #define ME_COMPILER_HAS_MTUNE 1
#endif
#ifndef ME_COMPILER_HAS_PAM
    #define ME_COMPILER_HAS_PAM 1
#endif
#ifndef ME_COMPILER_HAS_STACK_PROTECTOR
    #define ME_COMPILER_HAS_STACK_PROTECTOR 1
#endif
#ifndef ME_COMPILER_HAS_SYNC
    #define ME_COMPILER_HAS_SYNC 1
#endif
#ifndef ME_COMPILER_HAS_SYNC64
    #define ME_COMPILER_HAS_SYNC64 1
#endif
#ifndef ME_COMPILER_HAS_SYNC_CAS
    #define ME_COMPILER_HAS_SYNC_CAS 1
#endif
#ifndef ME_COMPILER_HAS_UNNAMED_UNIONS
    #define ME_COMPILER_HAS_UNNAMED_UNIONS 1
#endif
#ifndef ME_COMPILER_WARN64TO32
    #define ME_COMPILER_WARN64TO32 1
#endif
#ifndef ME_COMPILER_WARN_UNUSED
    #define ME_COMPILER_WARN_UNUSED 1
#endif
#ifndef ME_CONFIG_FILE
    #define ME_CONFIG_FILE "appweb.conf"
#endif
#ifndef ME_DEBUG
    #define ME_DEBUG 1
#endif
#ifndef ME_DEPTH
    #define ME_DEPTH 1
#endif
#ifndef ME_DESCRIPTION
    #define ME_DESCRIPTION "Embedthis Appweb Embedded Web Server"
#endif
#ifndef ME_EJS_DB
    #define ME_EJS_DB 1
#endif
#ifndef ME_EJS_MAIL
    #define ME_EJS_MAIL 1
#endif
#ifndef ME_EJS_MAPPER
    #define ME_EJS_MAPPER 1
#endif
#ifndef ME_EJS_SHELL
    #define ME_EJS_SHELL 1
#endif
#ifndef ME_EJS_TAR
    #define ME_EJS_TAR 1
#endif
#ifndef ME_EJS_TEMPLATE
    #define ME_EJS_TEMPLATE 1
#endif
#ifndef ME_EJS_WEB
    #define ME_EJS_WEB 1
#endif
#ifndef ME_EJS_ZLIB
    #define ME_EJS_ZLIB 1
#endif
#ifndef ME_EJS_ONE_MODULE
    #define ME_EJS_ONE_MODULE 1
#endif
#ifndef ME_ESP_LEGACY
    #define ME_ESP_LEGACY 1
#endif
#ifndef ME_EST_CAMELLIA
    #define ME_EST_CAMELLIA 0
#endif
#ifndef ME_EST_DES
    #define ME_EST_DES 0
#endif
#ifndef ME_EST_GEN_PRIME
    #define ME_EST_GEN_PRIME 0
#endif
#ifndef ME_EST_PADLOCK
    #define ME_EST_PADLOCK 0
#endif
#ifndef ME_EST_ROM_TABLES
    #define ME_EST_ROM_TABLES 0
#endif
#ifndef ME_EST_SSL_CLIENT
    #define ME_EST_SSL_CLIENT 0
#endif
#ifndef ME_EST_TEST_CERTS
    #define ME_EST_TEST_CERTS 0
#endif
#ifndef ME_EST_XTEA
    #define ME_EST_XTEA 0
#endif
#ifndef ME_HTTP_PAM
    #define ME_HTTP_PAM 1
#endif
#ifndef ME_HTTP_WEB_SOCKETS
    #define ME_HTTP_WEB_SOCKETS 1
#endif
#ifndef ME_MANIFEST
    #define ME_MANIFEST "installs/manifest.me"
#endif
#ifndef ME_ME
    #define ME_ME "=>0.8.4"
#endif
#ifndef ME_MPR_LOGGING
    #define ME_MPR_LOGGING 1
#endif
#ifndef ME_MPR_MANAGER
    #define ME_MPR_MANAGER "appman"
#endif
#ifndef ME_MPR_SSL_RENEGOTIATE
    #define ME_MPR_SSL_RENEGOTIATE 1
#endif
#ifndef ME_NAME
    #define ME_NAME "appweb"
#endif
#ifndef ME_PLATFORMS
    #define ME_PLATFORMS "local"
#endif
#ifndef ME_PREFIXES
    #define ME_PREFIXES "install-prefixes"
#endif
#ifndef ME_SERVER_ROOT
    #define ME_SERVER_ROOT "."
#endif
#ifndef ME_STATIC
    #define ME_STATIC 1
#endif
#ifndef ME_TITLE
    #define ME_TITLE "Embedthis Appweb"
#endif
#ifndef ME_TUNE
    #define ME_TUNE "size"
#endif
#ifndef ME_VERSION
    #define ME_VERSION "4.7.3"
#endif

/* Prefixes */
#ifndef ME_ROOT_PREFIX
    #define ME_ROOT_PREFIX "/"
#endif
#ifndef ME_BASE_PREFIX
    #define ME_BASE_PREFIX "/usr/local"
#endif
#ifndef ME_DATA_PREFIX
    #define ME_DATA_PREFIX "/"
#endif
#ifndef ME_STATE_PREFIX
    #define ME_STATE_PREFIX "/var"
#endif
#ifndef ME_APP_PREFIX
    #define ME_APP_PREFIX "/usr/local/lib/appweb"
#endif
#ifndef ME_VAPP_PREFIX
    #define ME_VAPP_PREFIX "/usr/local/lib/appweb/4.7.3"
#endif
#ifndef ME_BIN_PREFIX
    #define ME_BIN_PREFIX "/usr/local/bin"
#endif
#ifndef ME_INC_PREFIX
    #define ME_INC_PREFIX "/usr/local/include"
#endif
#ifndef ME_LIB_PREFIX
    #define ME_LIB_PREFIX "/usr/local/lib"
#endif
#ifndef ME_MAN_PREFIX
    #define ME_MAN_PREFIX "/usr/local/share/man"
#endif
#ifndef ME_SBIN_PREFIX
    #define ME_SBIN_PREFIX "/usr/local/sbin"
#endif
#ifndef ME_ETC_PREFIX
    #define ME_ETC_PREFIX "/etc/appweb"
#endif
#ifndef ME_WEB_PREFIX
    #define ME_WEB_PREFIX "/var/www/appweb"
#endif
#ifndef ME_LOG_PREFIX
    #define ME_LOG_PREFIX "/var/log/appweb"
#endif
#ifndef ME_SPOOL_PREFIX
    #define ME_SPOOL_PREFIX "/var/spool/appweb"
#endif
#ifndef ME_CACHE_PREFIX
    #define ME_CACHE_PREFIX "/var/spool/appweb/cache"
#endif
#ifndef ME_SRC_PREFIX
    #define ME_SRC_PREFIX "appweb-4.7.3"
#endif

/* Suffixes */
#ifndef ME_EXE
    #define ME_EXE ""
#endif
#ifndef ME_SHLIB
    #define ME_SHLIB ".dylib"
#endif
#ifndef ME_SHOBJ
    #define ME_SHOBJ ".dylib"
#endif
#ifndef ME_LIB
    #define ME_LIB ".a"
#endif
#ifndef ME_OBJ
    #define ME_OBJ ".o"
#endif

/* Profile */
#ifndef ME_CONFIG_CMD
    #define ME_CONFIG_CMD "me -d -q -platform macosx-x64-static -static -configure . -with openssl -gen xcode"
#endif
#ifndef ME_APPWEB_PRODUCT
    #define ME_APPWEB_PRODUCT 1
#endif
#ifndef ME_PROFILE
    #define ME_PROFILE "static"
#endif
#ifndef ME_TUNE_SIZE
    #define ME_TUNE_SIZE 1
#endif

/* Miscellaneous */
#ifndef ME_MAJOR_VERSION
    #define ME_MAJOR_VERSION 4
#endif
#ifndef ME_MINOR_VERSION
    #define ME_MINOR_VERSION 7
#endif
#ifndef ME_PATCH_VERSION
    #define ME_PATCH_VERSION 3
#endif
#ifndef ME_VNUM
    #define ME_VNUM 400070003
#endif

/* Components */
#ifndef ME_COM_CGI
    #define ME_COM_CGI 1
#endif
#ifndef ME_COM_CC
    #define ME_COM_CC 1
#endif
#ifndef ME_COM_DIR
    #define ME_COM_DIR 1
#endif
#ifndef ME_COM_EJS
    #define ME_COM_EJS 0
#endif
#ifndef ME_COM_ESP
    #define ME_COM_ESP 1
#endif
#ifndef ME_COM_EST
    #define ME_COM_EST 0
#endif
#ifndef ME_COM_HTTP
    #define ME_COM_HTTP 1
#endif
#ifndef ME_COM_LIB
    #define ME_COM_LIB 1
#endif
#ifndef ME_COM_MDB
    #define ME_COM_MDB 1
#endif
#ifndef ME_COM_MPR
    #define ME_COM_MPR 1
#endif
#ifndef ME_COM_OPENSSL
    #define ME_COM_OPENSSL 1
#endif
#ifndef ME_COM_OSDEP
    #define ME_COM_OSDEP 1
#endif
#ifndef ME_COM_PCRE
    #define ME_COM_PCRE 1
#endif
#ifndef ME_COM_PHP
    #define ME_COM_PHP 0
#endif
#ifndef ME_COM_SQLITE
    #define ME_COM_SQLITE 0
#endif
#ifndef ME_COM_SSL
    #define ME_COM_SSL 1
#endif
#ifndef ME_COM_VXWORKS
    #define ME_COM_VXWORKS 0
#endif
#ifndef ME_COM_WINSDK
    #define ME_COM_WINSDK 1
#endif
#ifndef ME_COM_ZLIB
    #define ME_COM_ZLIB 0
#endif
