/*
    server.c - Embed the AppWeb server in a multi-threaded C language application.
 */
 
#include    "appweb.h"

/*
    MAIN is used to compile on platforms like VxWorks that have different calling conventions
 */
MAIN(server, int argc, char **argv, char **envp)
{
    /*
        This will create and run the web server described by the appweb.conf configuration file.
     */
    return maRunWebServer("appweb.conf");
}
