/*
    Uri validation
 */

const HTTP: Uri = App.config.uris.http || "127.0.0.1:4100"

function get(uri): String {
    let s = new Socket
    s.connect(HTTP.address)
    let count = 0
    try {
        count += s.write("GET " + uri + " HTTP/1.0\r\n\r\n")
    } catch {
        App.log.error("Write failed. Wrote  " + count + " of " + data.length + " bytes.")
    }
    let response = new ByteArray
    while ((n = s.read(response, -1)) != null ) {}
    s.close()
    return response.toString()
}

let response
response = get('index.html')
assert(response.toString().contains('Bad Request'))

response = get('/\x01index.html')
assert(response.toString().contains('Bad Request'))

response = get('\\index.html')
assert(response.toString().contains('Bad Request'))
