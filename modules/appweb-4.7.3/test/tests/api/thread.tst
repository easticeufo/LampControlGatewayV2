/*
    thread.tst - Multithreaded test of the Appweb
 */

/*
    TODO - not working reliably
let testAppweb = test.bin.join("testAppweb").portable
let command = testAppweb + " --host " + App.config.uris.http + " --name mpr.api.c --iterations 5 " + 
    test.mapVerbosity(-2)

for each (threadCount in [2, 4, 8, 16]) {
    print(command + "--threads " + threadCount)
    Cmd.sh(command + "--threads " + threadCount)
}
*/
