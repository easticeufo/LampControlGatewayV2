/*
    CGI Denial of service testing
 */

const HTTP: Uri = App.config.uris.http || "127.0.0.1:4100"

// Depths:    0  1  2  3   4   5   6   7   8   9 
let sizes = [ 1, 2, 4, 8, 12, 16, 24, 32, 40, 64 ]
let depth = global.test ? test.depth : 1
count = sizes[depth] * 20

if (Config.OS != 'windows') {
    //  Check server available
    http = new Http
    http.get(HTTP + "/index.html")
    assert(http.status == 200)
    http.close()

    //  Try to crash with DOS attack
    for (i in count) {
        let http = new Http
        http.get(HTTP + '/cgi-bin/cgiProgram')
        http.close()
    }

    //  Check server still there
    App.sleep(1000)
    http = new Http
    http.get(HTTP + "/index.html")
    assert(http.status == 200)
    http.close()
}
