import httpclient, strutils, base64, htmlparser, xmltree, json, os, times


# === Configuration ===
const
  baseUrl = "http://my_ip:port"
  username = "username"
  password = "pass"

let now = now()
let formatted = now.format("yyyyMMddHHmm")
let outputFile = formatted & "_logs.json"

# === Basic Auth Header ===
let credentials = username & ":" & password
let authValue = encode(cast[seq[byte]](credentials))
let authHeader = "Basic " & authValue

# === HTTP client avec auth ===
proc createHttpClient(): HttpClient =
  var client = newHttpClient()
  client.headers["Authorization"] = authHeader
  return client

# === Nettoyage HTML ===
proc removeNonPrintable(s: string): string =
  for c in s:
    if c in ' '..'~' or c in {'\n', '\r', '\t'}:
      result.add(c)

proc fixBrokenTags(s: string): string =
  result = s
  result = result.replace("</pre></code>", "</code></pre>")
  result = result.replace("<pre><code>", "<code><pre>")
  result = result.replace("</code></pre>", "</pre></code>")
  result = result.replace("</code>", "</code>")
  result = result.replace("</pre>", "</pre>")

# === Parser HTML Table ===
proc findTable(n: XmlNode): XmlNode =
  if n.kind == xnElement and n.tag == "table":
    return n
  if n.kind == xnElement:
    for child in n.items:
      let result = findTable(child)
      if not result.isNil:
        return result
  return nil

# === Extraction des logs génériques depuis une URL donnée ===
proc extractLogsFromUrl(client: HttpClient, path: string, kind: string): seq[JsonNode] =
  var logs: seq[JsonNode] = @[]
  let rawHtml = client.getContent(baseUrl & path)
  let cleanedHtml = fixBrokenTags(removeNonPrintable(rawHtml))
  let root = parseHtml(cleanedHtml)
  let table = findTable(root)

  if table.isNil:
    echo "❌ Table HTML introuvable pour ", path
    return logs

  var rowIndex = 0
  for tr in table.items:
    if tr.kind != xnElement or tr.tag != "tr":
      continue
    if rowIndex == 0:
      inc rowIndex
      continue  # skip header

    var fields: seq[string] = @[]
    for td in tr.items:
      if td.kind == xnElement and td.tag == "td":
        fields.add(td.innerText.strip)

    if kind == "logs":
      if fields.len >= 4 and fields[3].len > 0:
        logs.add(%*{
          "id": fields[0],
          "time": fields[1],
          "ip": fields[2],
          "command": fields[3]
        })
    elif kind == "web":
      if fields.len >= 5:
        logs.add(%*{
          "id": fields[0],
          "time": fields[1],
          "ip": fields[2],
          "url": fields[3],
          "post": fields[4]
        })

    inc rowIndex
  return logs

# === Exécution principale ===
let client = createHttpClient()
let logs = extractLogsFromUrl(client, "/logs", "logs")
let webLogs = extractLogsFromUrl(client, "/logweb", "web")

let outputJson = %*{
  "logs": logs,
  "webLogs": webLogs
}

writeFile(outputFile, pretty(outputJson))
echo "✅ Fichier généré : ", outputFile
