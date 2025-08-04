#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "esp_config.h"

#define NEED_AUTH()                                                                                \
    if (!server.authenticate(HTTP_AUTH_USERNAME, HTTP_AUTH_PASSWD)) {                              \
        return server.requestAuthentication();                                                     \
    }

struct CommandeLog {
    String       date;
    IPAddress    ip;
    String       commande;
    CommandeLog* next;
};
CommandeLog* commandesHead = nullptr;

struct LogEntry {
    IPAddress ip;
    String    timestamp;
    String    url;
    String    postData;
    LogEntry* next;
};
LogEntry* logsHead = nullptr;

unsigned long bootTime, previousDisplayUpdate = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Telnet server
WiFiServer    telnetServer(PORT_TELNET);
WiFiClient    telnetClient;
bool          passwordEntered = false, authDone = false;
String        telnetBuffer       = "", loginInput, passwordInput;
const char*   blacklist[]        = CMD_BLACKLIST;
const int     blacklistSize      = sizeof(blacklist) / sizeof(blacklist[0]);
unsigned long telnetLastActive   = 0;
unsigned long telnetSessionStart = 0;
enum TelnetAuthState { TELNET_LOGIN, TELNET_PASSWORD, TELNET_AUTHED };
TelnetAuthState telnetState = TELNET_LOGIN;

// Web servers
ESP8266WebServer server(PORT_WEB_ADMIN);
ESP8266WebServer fake_webserver(PORT_WEB_HONEYPOT);
const char*      web_blacklist[]   = WEB_BLACKLIST;
const int        web_blacklistSize = sizeof(web_blacklist) / sizeof(web_blacklist[0]);

// const char*      web_exact_blacklist[]   = WEB_EXACT_BLACKLIST;
// const int        web_exact_blacklistSize = sizeof(web_exact_blacklist) /
// sizeof(web_exact_blacklist[0]);

// NTP
WiFiUDP   ntpUDP;
NTPClient timeClient(ntpUDP, NTP_POOL, NTP_SHIFT, NTP_UPDATE_DELAY);

enum e_display { NONE, STATS, HOUR, LASTEVENT };
enum e_display screen_display = DISPLAY_MODE;

void checkWiFi() {
    static unsigned long lastAttemptTime = 0;
    const unsigned long  retryInterval   = 30000;
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastAttemptTime >= retryInterval) {
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWD);
            lastAttemptTime = now;
        }
    }
}

String generate_header(const String& title) {
    String header;
    header.reserve(512);
    header = "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"UTF-8\">\n";
    header += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    header += "<title>" + title + "</title>\n";
    header += globalCSS;
    header += "\n</head>\n<body>\n";
    return header;
}

// Telnet logs management
void addCommandeLog(const String& date, const IPAddress& ip, const String& commande) {
    CommandeLog* newNode = new CommandeLog{date, ip, commande, commandesHead};
    commandesHead        = newNode;
}

void deleteAllCommandeLogs() {
    while (commandesHead) {
        CommandeLog* tmp = commandesHead;
        commandesHead    = commandesHead->next;
        delete tmp;
    }
}

int countCommandeLogs() {
    int count = 0;
    for (CommandeLog* ptr = commandesHead; ptr; ptr = ptr->next)
        count++;
    return count;
}

int countUniqueIPs() {
    IPAddress uniqueIPs[100];
    int       uniqueCount = 0;
    for (CommandeLog* ptr = commandesHead; ptr; ptr = ptr->next) {
        bool found = false;
        for (int j = 0; j < uniqueCount; j++) {
            if (ptr->ip == uniqueIPs[j]) {
                found = true;
                break;
            }
        }
        if (!found && uniqueCount < 100) uniqueIPs[uniqueCount++] = ptr->ip;
    }
    return uniqueCount;
}

void printCommandeLogs(ESP8266WebServer& server) {
    int idx = 1;
    for (CommandeLog* ptr = commandesHead; ptr; ptr = ptr->next, idx++) {
        String row;
        row.reserve(256);
        row = "<tr>";
        row += "<td>" + String(idx) + "</td>";
        row += "<td>" + ptr->date + "</td>";
        row += "<td>" + ptr->ip.toString() + "</td>";
        row += "<td><pre><code>" + ptr->commande + "</pre></code></td>";
        row += "</tr>";
        server.sendContent(row);
        delay(1);
    }
}

// Web logs management
void addLog(IPAddress ip, const String& url, const String& postData) {
    LogEntry* newNode = new LogEntry{ip, timeClient.getFormattedTime(), url, postData, logsHead};
    logsHead          = newNode;
}

void deleteAllLogs() {
    while (logsHead) {
        LogEntry* tmp = logsHead;
        logsHead      = logsHead->next;
        delete tmp;
    }
}

int countWebLogs() {
    int count = 0;
    for (LogEntry* ptr = logsHead; ptr; ptr = ptr->next)
        count++;
    return count;
}

void printWebLogs(ESP8266WebServer& server) {
    int idx = 1;
    for (LogEntry* ptr = logsHead; ptr; ptr = ptr->next, idx++) {
        String row;
        row.reserve(256);
        row = "<tr>";
        row += "<td>" + String(idx) + "</td>";
        row += "<td>" + ptr->timestamp + "</td>";
        row += "<td>" + ptr->ip.toString() + "</td>";
        row += "<td>" + ptr->url + "</td>";
        row += "<td><pre><code>" + ptr->postData + "</pre></code></td>";
        row += "</tr>";
        server.sendContent(row);
        delay(1);
    }
}
// End of log management

void updateDisplay() {
    display.setCursor(0, 0);
    switch (screen_display) {
        case NONE:
            display.clearDisplay();
            break;
        case HOUR:
            display.clearDisplay();
            display.setTextSize(2);
            display.setCursor(10, 20);
            display.println(timeClient.getFormattedTime());
            break;
        case LASTEVENT:
            break;
        case STATS:
            display.clearDisplay();
            display.setTextSize(1);
            display.println(timeClient.getFormattedTime());
            display.println("Free RAM:" + String(ESP.getFreeHeap()) + " bytes");
            display.println(WiFi.localIP());
            display.println(WiFi.macAddress());
            display.println("# of commands : " + String(countCommandeLogs()));
            break;
    }
    display.display();
}

bool isBlacklisted(String haystack) {
    for (int i = 0; i < blacklistSize; i++) {
        if (haystack.indexOf(blacklist[i]) >= 0) return true;
    }
    return false;
}

void sanitize(String& str) {
    str.replace("&", "&amp;");
    str.replace("<", "&lt;");
    str.replace(">", "&gt;");
}

String httpMethodToString(HTTPMethod method) {
    switch (method) {
        case HTTP_GET:
            return "GET";
        case HTTP_POST:
            return "POST";
        case HTTP_DELETE:
            return "DELETE";
        case HTTP_PUT:
            return "PUT";
        case HTTP_PATCH:
            return "PATCH";
        default:
            return "UNKNOWN";
    }
}

// Honeypot web server
void setupWebServer() {
    fake_webserver.onNotFound([]() {
        String url = fake_webserver.uri();
        for (int i = 0; i < web_blacklistSize; i++) {
            if (url.indexOf(web_blacklist[i]) != -1) return;
        }

        IPAddress clientIP = fake_webserver.client().remoteIP();
        String    postData = fake_webserver.hasArg("plain") ? fake_webserver.arg("plain") : "";

        // for (int i = 0; i < web_blacklistSize; i++) {
        //    if (url == web_exact_blacklist[i]) return;
        //}
        sanitize(postData);
        sanitize(url);
        addLog(clientIP, url, postData);
        fake_webserver.send(200, "text/plain", "OK");

        if (screen_display == LASTEVENT) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.setTextSize(1);
            display.println("Last connection:");
            display.println("From: " + clientIP.toString());
            display.println("At  : " + timeClient.getFormattedTime());
            display.println(httpMethodToString(fake_webserver.method()) + " " + url);
            display.display();
        }
    });

    // Admin web server
    server.on("/", HTTP_GET, []() {
        NEED_AUTH();
        int    totalTelnetLogs = countCommandeLogs(), uniqueTelnetIPs = countUniqueIPs();
        String lastTelnetCmd = "";
        if (commandesHead) lastTelnetCmd = commandesHead->commande;
        int totalWebLogs = countWebLogs();

        // Count unique Web IPs
        IPAddress uniqueWebIPs[100];
        int       uniqueWebIPsCount = 0;
        for (LogEntry* ptr = logsHead; ptr; ptr = ptr->next) {
            bool found = false;
            for (int j = 0; j < uniqueWebIPsCount; j++) {
                if (ptr->ip == uniqueWebIPs[j]) {
                    found = true;
                    break;
                }
            }
            if (!found && uniqueWebIPsCount < 100) uniqueWebIPs[uniqueWebIPsCount++] = ptr->ip;
        }

        String lastWebURL    = logsHead ? logsHead->url : "";
        int    rssi          = WiFi.RSSI();
        String signalQuality = (rssi > -60) ? "Strong" : (rssi > -75) ? "Medium" : "Weak";
        String bootTimeStr   = timeClient.getFormattedTime();

        // Calcul des pourcentages
        int totalRAM       = 42000;
        int freeRAM        = ESP.getFreeHeap();
        int ramUsed        = totalRAM - freeRAM;
        int ramUsedPercent = (ramUsed * 100) / totalRAM;

        int wifiQualityPercent = constrain(2 * (rssi + 100), 0, 100);

        String html;
        html.reserve(4096);
        html = generate_header("ESP8266 - Stats");
        html += R"rawliteral(

    <h1>ESP8266 - Usage Stats</h1>
      <div class="circle-wrapper">
        <div class="circle-block">
          <div class="circle" style="--p: )rawliteral" +
                String(ramUsedPercent) + R"rawliteral(%;">
            <span>)rawliteral" +
                String(ramUsedPercent) + R"rawliteral(%</span>
          </div>
          <div class="circle-label">RAM Used</div>
        </div>
        <div class="circle-block">
          <div class="circle" style="--p: )rawliteral" +
                String(wifiQualityPercent) + R"rawliteral(%;">
            <span>)rawliteral" +
                String(wifiQualityPercent) + R"rawliteral(%</span>
          </div>
          <div class="circle-label">WiFi Signal</div>
        </div>
        <div class="circle-block">
          <div class="circle" style="--p: 100%;">
            <span>)rawliteral" +
                String(totalTelnetLogs) + R"rawliteral(</span>
          </div>
          <div class="circle-label">Telnet Cmds</div>
        </div>
        <div class="circle-block">
          <div class="circle" style="--p: 100%;">
            <span>)rawliteral" +
                String(totalWebLogs) + R"rawliteral(</span>
          </div>
          <div class="circle-label">Web Reqs</div>
        </div>
      </div>
    <div class="container">
      <ul>
        <li>Firmware version: )rawliteral" +
                String(ESP.getSdkVersion()) + R"rawliteral(</li>
        <li>Boot time (UTC+1): )rawliteral" +
                bootTimeStr + R"rawliteral(</li>
      </ul>

      <h2>Telnet Stats</h2>
      <ul>
        <li>Total Telnet commands: )rawliteral" +
                String(totalTelnetLogs) + R"rawliteral(</li>
        <li>Unique Telnet IPs: )rawliteral" +
                String(uniqueTelnetIPs) + R"rawliteral(</li>
        <li>Last Telnet command: )rawliteral" +
                lastTelnetCmd + R"rawliteral(</li>
      </ul>
      <a class='button' href='/logs'>Open</a>
      <a class='button button-danger' href='#' onclick="fetch('/delete').then(() => showToast('Sent'))">Delete</a>

      <h2>Web Stats</h2>
      <ul>
        <li>Total Web logs: )rawliteral" +
                String(totalWebLogs) + R"rawliteral(</li>
        <li>Unique Web IPs: )rawliteral" +
                String(uniqueWebIPsCount) + R"rawliteral(</li>
        <li>Last Web URL: )rawliteral" +
                lastWebURL + R"rawliteral(</li>
      </ul>
      <a class='button' href='/logweb'>Open</a>
      <a class='button button-danger' href='#' onclick="fetch('/delete_logweb').then(() => showToast('Sent'))">Delete</a>

      <h2>General</h2>
      <a class='button button-danger' href='#' onclick="fetch('/reboot').then(() => showToast('Sent'))">Reboot</a>
      <a class='button' href='#' onclick="fetch('/display/hour').then(() => showToast('Sent'))">Display hour</a>
      <a class='button' href='#' onclick="fetch('/display/state').then(() => showToast('Sent'))">Display state</a>
      <a class='button' href='#' onclick="fetch('/display/none').then(() => showToast('Sent'))">Clear screen</a>
      </div>
    </body>
    </html>
    )rawliteral";

        server.send(200, "text/html", html);
    });

    server.on("/display/hour", HTTP_GET, []() {
        NEED_AUTH();
        screen_display = HOUR;
        updateDisplay();
        server.send(200, "text/plain", "ok");
    });

    server.on("/display/state", HTTP_GET, []() {
        NEED_AUTH();
        screen_display = STATS;
        updateDisplay();
        server.send(200, "text/plain", "ok");
    });

    server.on("/display/none", HTTP_GET, []() {
        NEED_AUTH();
        screen_display = NONE;
        updateDisplay();
        server.send(200, "text/plain", "ok");
    });

    server.on("/display/event", HTTP_GET, []() {
        NEED_AUTH();
        screen_display = LASTEVENT;
        updateDisplay();
        server.send(200, "text/plain", "ok");
    });

    server.on("/reboot", HTTP_GET, []() {
        NEED_AUTH();
        server.send(200, "text/plain", "Redémarrage...");
        delay(300);
        ESP.restart();
    });

    server.on("/delete", HTTP_GET, []() {
        NEED_AUTH();
        deleteAllCommandeLogs();
        server.send(200, "text/html", "ok");
    });

    server.on("/logs", HTTP_GET, []() {
        NEED_AUTH();
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "text/html", "");
        server.sendContent(generate_header("ESP8266 - Logs"));
        //  server.sendContent(globalCSS);
        server.sendContent(R"rawliteral(
      </head>
      <body>
        <h1>Command Logs</h1>
    )rawliteral");

        if (!commandesHead) {
            server.sendContent("<p>No data.</p>");
        } else {
            server.sendContent(
                "<table><tr><th>#</th><th>Date</th><th>IP</th><th>Command</th></tr>");
            printCommandeLogs(server);
            server.sendContent("</table>");
        }

        server.sendContent("<a class='button' href='/'>Go back</a>");
        server.sendContent(" <a class='button' href='#' onclick=\"fetch('/delete').then(() => "
                           "showToast('Sent'));\">Delete logs</a>");
        server.sendContent("</body></html>");
        server.sendContent("");
    });

    server.on("/delete_logweb", HTTP_GET, []() {
        NEED_AUTH();
        deleteAllLogs();
        server.send(200, "text/html", "ok");
    });

    server.on("/logweb", HTTP_GET, []() {
        NEED_AUTH();
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "text/html", "");
        server.sendContent(R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>ESP8266 - Web Logs</title>
    )rawliteral");
        server.sendContent(globalCSS);
        server.sendContent(R"rawliteral(
      </head>
      <body>
        <h1>Web Logs</h1>
    )rawliteral");

        if (!logsHead) {
            server.sendContent("<p>No data.</p>");
        } else {
            server.sendContent(
                "<table><tr><th>#</th><th>Date</th><th>IP</th><th>URL</th><th>Post Data</th></tr>");
            printWebLogs(server);
            server.sendContent("</table>");
        }

        server.sendContent("<a class='button' href='/'>Go back</a>");
        server.sendContent(" <a class='button' href='#' onclick=\"fetch('/delete_logweb').then(() "
                           "=> showToast('Sent'));\">Delete Logs</a>");
        server.sendContent("</body></html>");
        server.sendContent("");
    });

    server.begin();
    fake_webserver.begin();
}

void setup() {
    Wire.begin(SDA, SCL);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("Connecting to wifi");
    display.display();

    WiFi.config(localIP, gateway, subnet, dns);
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(200);
        display.print(".");
        display.display();
    }

    timeClient.begin();
    timeClient.update();
    bootTime = timeClient.getEpochTime();

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("Connected!");
    display.println(WiFi.localIP());
    display.println(WiFi.macAddress());
    display.println(timeClient.getFormattedTime());
    display.println("Start listening...");
    display.display();

    telnetServer.begin();
    telnetServer.setNoDelay(true);
    setupWebServer();
}

void loop() {
    checkWiFi();
    server.handleClient();
    fake_webserver.handleClient();
    timeClient.update();

    // Gestion expiration session Telnet (>10s)
    if (telnetServer.hasClient()) {
        unsigned long now = millis();
        if (!telnetClient || !telnetClient.connected()) {
            telnetClient       = telnetServer.available();
            telnetSessionStart = now;
            telnetLastActive   = now;
            telnetState        = TELNET_LOGIN;
            telnetBuffer       = "";
            telnetClient.print(TELNET_LOGIN_TXT);
        } else {
            // Ancien client >10s : on drop et on prend le nouveau
            if (now - telnetSessionStart > TELNET_KILL_CLIENT_DELAY) {
                telnetClient.stop();
                telnetClient       = telnetServer.available();
                telnetSessionStart = now;
                telnetLastActive   = now;
                telnetState        = TELNET_LOGIN;
                telnetBuffer       = "";
                telnetClient.print(TELNET_LOGIN_TXT);
            } else {
                // Session trop récente : on refuse le client entrant
                WiFiClient newClient = telnetServer.available();
                newClient.stop();
            }
        }
    }

    // Lecture Telnet
    if (telnetClient && telnetClient.connected() && telnetClient.available()) {
        char c           = telnetClient.read();
        telnetLastActive = millis();

        // Attention \r\n ou seul \n ou \r selon client telnet
        if (c == '\n' || c == '\r') {
            telnetBuffer.trim();
            if (telnetState == TELNET_LOGIN) {
                loginInput   = telnetBuffer;
                telnetBuffer = "";
                telnetState  = TELNET_PASSWORD;
                telnetClient.print(TELNET_PASSWORD_TXT);
            } else if (telnetState == TELNET_PASSWORD) {
                passwordInput = telnetBuffer;
                telnetBuffer  = "";
                telnetState   = TELNET_AUTHED;
            } else if (telnetState == TELNET_AUTHED) {
                String cmd   = telnetBuffer;
                telnetBuffer = "";
                if (!isBlacklisted(cmd) && cmd.length() > 0) {
                    sanitize(cmd);
                    addCommandeLog(timeClient.getFormattedTime(), telnetClient.remoteIP(), cmd);
                    if (screen_display == LASTEVENT) {
                        display.clearDisplay();
                        display.setCursor(0, 0);
                        display.println("Last connection:");
                        display.println("From : " + telnetClient.remoteIP().toString());
                        display.println("At   : " + timeClient.getFormattedTime());
                        display.println("User : " + loginInput);
                        display.println("Run  : " + cmd);
                        display.display();
                    }
                }
                telnetClient.print(TELNET_BANNER);
            }
        } else {
            telnetBuffer += c;
        }
    }

    unsigned long currentMillis = millis();
    if (currentMillis - previousDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        previousDisplayUpdate = currentMillis;
        updateDisplay();
    }
}