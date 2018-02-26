/*
    Very large URI test (3MB)
 */ 
const HTTP: Uri = App.config.uris.http || "127.0.0.1:4100"

//  This writes a ~100K URI. LimitUri should be less than 100K for this unit test.

let data = "/"
for (i in 1000) {
    data += "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678\n"
}

/*
    Test LimitUri
 */
let s = new Socket
s.connect(HTTP.address)
let count = 0
try {
    count += s.write("GET ")
    count += s.write(data)
    count += s.write(" HTTP/1.1\r\n\r\n")
} catch {
    App.log.error("Write failed. Wrote  " + count + " of " + data.length + " bytes.")
    //  Check appweb.conf LimitRequestHeader. This must be sufficient to accept the write the header.
}

/* Server should just close the connection without a response */
response = new ByteArray
while ((n = s.read(response, -1)) != null ) {}
assert(response.toString() == '')
s.close()

//  Check server still up
if (Config.OS == 'windows') {
    App.sleep(3000)
}
http = new Http
http.get(HTTP + "/index.html")
assert(http.status == 200)
http.close()
