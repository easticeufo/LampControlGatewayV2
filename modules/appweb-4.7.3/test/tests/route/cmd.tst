/*
    cmd.tst - Test Update cmd
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http

//  Run a command that creates ../../route-update-cmd.tmp
  
if (Config.OS != "VXWORKS" && Config.OS != "WIN") { 
    let path = Path("../../route-update-cmd.tmp")
    path.remove()
    assert(!path.exists)

    http.get(HTTP + "/route/update/cmd")
    assert(http.status == 200)
    assert(http.response == "UPDATED")
    assert(path.exists)
    assert(path.remove())
    http.close()
}
