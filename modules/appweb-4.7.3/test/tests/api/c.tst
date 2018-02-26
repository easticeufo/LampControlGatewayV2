/*
    c.tst - Test the Appweb C API
 */

const HOST = App.config.uris.http || "127.0.0.1:4100"

let testAppweb = test.bin.join("testAppweb").portable
let command = testAppweb + " --host " + HOST + " --name appweb.api.c " + test.mapVerbosity(-3)
Cmd.sh(command)
