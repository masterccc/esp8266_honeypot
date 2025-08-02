// OLED config
#define SDA 14
#define SCL 12
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// WiFi config
#define WIFI_SSID "MYSSID"
#define WIFI_PASSWD "MY_WIFI_PASS"

// IP config (optionnal) - use nullptr
IPAddress localIP(192, 168, 0, 50);  // localIP = nullptr ;
IPAddress gateway(192, 168, 0, 254); // gateway = nullptr ;
IPAddress subnet(255, 255, 255, 0);  // subnet  = nullptr ;
IPAddress dns(192, 168, 0, 254);     // dns     = nullptr ;

// HTTP server
#define PORT_WEB_ADMIN 80
#define PORT_WEB_HONEYPOT 8080
#define HTTP_AUTH_USERNAME "mylogin"
#define HTTP_AUTH_PASSWD "mypass"
#define WEB_BLACKLIST {"phpmyadmin", "robots.txt", "/favicon."}

// Display
// Values : HOUR, STATE, NONE, LASTEVENT
#define DISPLAY_MODE LASTEVENT
#define DISPLAY_UPDATE_INTERVAL 5000

// Telnet config
#define PORT_TELNET 23
#define TELNET_KILL_CLIENT_DELAY 10000
#define TELNET_BANNER "root #"
#define TELNET_LOGIN_TXT "Login: "
#define TELNET_PASSWORD_TXT "Password: "
#define CMD_BLACKLIST {"rm", "reboot", "whoami"}

// NTP Config
#define NTP_POOL "pool.ntp.org"
#define NTP_SHIFT 7200
#define NTP_UPDATE_DELAY 3600000

// CSS/JS global
const char* globalCSS = R"rawliteral(
  <style>
    body { font-family: sans-serif; background: #f4f4f4; padding: 20px; color: #333; }
    h1 { color: #005a9c; }
    ul, table { background: #fff; padding: 10px; border-radius: 5px; }
    li { margin-bottom: 5px; }
    table { width: 100%; border-collapse: collapse; }
    th, td { border: 1px solid #ccc; padding: 8px; text-align: left; font-size: 14px; }
    th { background: #007BFF; color: white; }
    tr:nth-child(even) { background: #f9f9f9; }
    .button {
      display: inline-block;
      padding: 10px 15px;
      background: #007BFF;
      color: white;
      text-decoration: none;
      border-radius: 4px;
    }
    .button:hover { background: #0056b3; }
    pre { white-space: pre-wrap; word-wrap: break-word; max-width: 600px; }
  </style>

  <script>
function showToast(m){let t=document.createElement('div');t.textContent=m;Object.assign(t.style,{position:'fixed',bottom:'24px',left:'50%',transform:'translateX(-50%)',background:'#333',color:'#fff',padding:'8px 24px',borderRadius:'4px',zIndex:1e4,fontSize:'16px',opacity:'0.96'});document.body.appendChild(t);setTimeout(()=>t.remove(),1500);}

</script>
)rawliteral";
