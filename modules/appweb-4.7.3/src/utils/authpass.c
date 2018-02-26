/*
    authpass.c -- Authorization password management.

    This file provides facilities for creating passwords in an auth.conf file. It uses basic encoding and decoding 
    using the base64 encoding scheme and MD5 Message-Digest algorithms.
 */

/********************************* Includes ***********************************/

#include    "appweb.h"

/********************************** Locals ************************************/

#define MAX_PASS    64

static cchar        *programName;

/********************************* Forwards ***********************************/

static char *getPassword();
static void printUsage(cchar *programName);

#if ME_WIN_LIKE || VXWORKS
static char *getpass(char *prompt);
#endif

/*********************************** Code *************************************/

PUBLIC int main(int argc, char *argv[])
{
    Mpr         *mpr;
    MprBuf      *buf;
    MaAppweb    *appweb;
    MaServer    *server;
    HttpAuth    *auth;
    char        *password, *authFile, *username, *encodedPassword, *realm, *cp, *roles, *cipher;
    int         i, errflg, create, nextArg;

    mpr = mprCreate(argc, argv, 0);
    programName = mprGetAppName(mpr);

    username = 0;
    create = errflg = 0;
    password = 0;
    cipher = "blowfish";

    for (i = 1; i < argc && !errflg; i++) {
        if (argv[i][0] != '-') {
            break;
        }
        for (cp = &argv[i][1]; *cp && !errflg; cp++) {
            if (*cp == '-') {
                cp++;
            }
            if (smatch(cp, "create") || smatch(cp, "c")) {
                create++;
                break;

            } else if (smatch(cp, "cipher")) {
                if (++i == argc) {
                    errflg++;
                } else {
                    cipher = argv[i];
                    if (!smatch(cipher, "md5") && !smatch(cipher, "blowfish")) {
                        mprError("Unknown cipher \"%s\". Use \"md5\" or \"blowfish\".", cipher);
                    }
                    break;
                }

            } else if (smatch(cp, "password") || smatch(cp, "p")) {
                if (++i == argc) {
                    errflg++;
                } else {
                    password = argv[i];
                    break;
                }

            } else {
                errflg++;
            }
        }
    }
    nextArg = i;

    if ((nextArg + 3) > argc) {
        errflg++;
    }
    if (errflg) {
        printUsage(programName);
        exit(2);
    }   
    authFile = argv[nextArg++];
    realm = argv[nextArg++];
    username = argv[nextArg++];

    buf = mprCreateBuf(0, 0);
    for (i = nextArg; i < argc; ) {
        mprPutStringToBuf(buf, argv[i]);
        if (++i < argc) {
            mprPutCharToBuf(buf, ' ');
        }
    }
    roles = sclone(mprGetBufStart(buf));

    appweb = maCreateAppweb();
    server = maCreateServer(appweb, "default");
    auth = maGetDefaultAuth(server);

    if (!create) {
        if (maParseConfig(server, authFile, 0) < 0) {
            exit(2);
        }
        if (!mprPathExists(authFile, W_OK)) {
            mprError("%s: Cannot write to %s", programName, authFile);
            exit(4);
        }
    } else if (mprPathExists(authFile, R_OK)) {
        mprError("%s: Cannot create %s, already exists", programName, authFile);
        exit(5);
    }
    if (!password && (password = getPassword()) == 0) {
        exit(1);
    }
    if (smatch(cipher, "md5")) {
        encodedPassword = mprGetMD5(sfmt("%s:%s:%s", username, realm, password));
    } else {
        /* This uses the more secure blowfish cipher */
        encodedPassword = mprMakePassword(sfmt("%s:%s:%s", username, realm, password), 16, 128);
    }
    httpRemoveUser(auth, username);
    if (httpAddUser(auth, username, encodedPassword, roles) == 0) {
        exit(7);
    }
    if (maWriteAuthFile(auth, authFile) < 0) {
        exit(6);
    }
    mprDestroy();
    return 0;
}


static char *getPassword()
{
    char    *password, *confirm;

    password = getpass("New password: ");
    confirm = getpass("Confirm password: ");
    if (smatch(password, confirm)) {
        return password;
    }
    mprError("%s: Error: Password not verified", programName);
    return 0;
}


#if WINCE
static char *getpass(char *prompt)
{
    return "NOT-SUPPORTED";
}

#elif ME_WIN_LIKE || VXWORKS
static char *getpass(char *prompt)
{
    static char password[MAX_PASS];
    int     c, i;

    fputs(prompt, stderr);
    for (i = 0; i < (int) sizeof(password) - 1; i++) {
#if VXWORKS
        c = getchar();
#else
        c = _getch();
#endif
        if (c == '\r' || c == EOF) {
            break;
        }
        if ((c == '\b' || c == 127) && i > 0) {
            password[--i] = '\0';
            fputs("\b \b", stderr);
            i--;
        } else if (c == 26) {           /* Control Z */
            c = EOF;
            break;
        } else if (c == 3) {            /* Control C */
            fputs("^C\n", stderr);
            exit(255);
        } else if (!iscntrl((uchar) c) && (i < (int) sizeof(password) - 1)) {
            password[i] = c;
            fputc('*', stderr);
        } else {
            fputc('', stderr);
            i--;
        }
    }
    if (c == EOF) {
        return "";
    }
    fputc('\n', stderr);
    password[i] = '\0';
    return sclone(password);
}

#endif /* ME_WIN_LIKE */
 
static void printUsage(cchar *programName)
{
    mprEprintf("usage: %s [--create] [--password password] authFile realm user roles...\n"
        "Options:\n"
        "    --cipher md5|blowfish Select the encryption cipher. Defaults to md5\n"
        "    --create              Create the password file\n"
        "    --passwordp password  Use the specified password\n"
        "\n", programName);
}


/*
    @copy   default

    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
