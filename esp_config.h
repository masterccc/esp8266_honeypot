// OLED config
#define SDA 14
#define SCL 12
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// WiFi config
#define WIFI_SSID "MYSSID"
#define WIFI_PASSWD "MYWIFIPASS"

// IP config (optionnal) - use nullptr
IPAddress localIP(192, 168, 1, 26);  // localIP = nullptr ;
IPAddress gateway(192, 168, 1, 254); // gateway = nullptr ;
IPAddress subnet(255, 255, 255, 0);  // subnet  = nullptr ;
IPAddress dns(192, 168, 1, 254);     // dns     = nullptr ;

// HTTP server
#define PORT_WEB_ADMIN 80
#define PORT_WEB_HONEYPOT 8080
#define HTTP_AUTH_USERNAME "mylogin"
#define HTTP_AUTH_PASSWD "mypass"
#define WEB_BLACKLIST                                                                              \
    {"phpmyadmin", "robots.txt", "/favicon.", "eval-stdin.php", "echo(md5", "admin/formLogin"}
#define WEB_EXACT_BLACKLIST {"/"}

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
#define CMD_BLACKLIST {"rm", "reboot", "whoami", "enable", "shell", "system", "mount"}

// NTP Config
#define NTP_POOL "pool.ntp.org"
#define NTP_SHIFT 7200
#define NTP_UPDATE_DELAY 3600000

// CSS/JS global
const char* globalCSS = R"rawliteral(
  <style>
   :root {
      --primary: #4CAF50;
      --secondary: #f4f4f4;
      --text-color: #333;
      --radius: 12px;
    }

    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      margin: 0;
      padding: 0;
      background: #f9f9f9;
      color: var(--text-color);
    }

    .container {
      max-width: 900px;
      margin: auto;
       padding-right: 0rem;
       padding-right: 2rem;
      padding-bottom: 2rem;
       padding-left: 2rem;
    }

    h1, h2 {
      font-weight: 600;
      margin-bottom: 1rem;
      color: #222;
    }

    ul {
  list-style-type: none;
    padding: 0;

    background: white;
    border-radius: var(--radius);
    box-shadow: 0 2px 10px rgba(0,0,0,0.05);
    padding: 0.5rem;
    }

    ul li {
      padding: 0.5rem 0;
      border-bottom: 1px solid #eee;
    }

    ul li:last-child {
      border-bottom: none;
    }

    .button {
      display: inline-block;
      margin: 0.5rem 0.5rem 0.5rem 0;
      padding: 0.6rem 1.2rem;
      background: var(--primary);
      color: white;
      border: none;
      border-radius: var(--radius);
      text-decoration: none;
      cursor: pointer;
      transition: background 0.3s;
    }

    .button:hover {
      background: #388e3c;
    }

    .button-danger {
      background: #f44336; /* rouge moderne */
    }

    .circle-wrapper {
      display: flex;
      justify-content: center;
      gap: 2rem;
      margin: 2rem 0;
      flex-wrap: wrap;
    }

    .circle {
      --size: 70px;
      --p: 65%;
      width: var(--size);
      height: var(--size);
      border-radius: 50%;
      background:
        radial-gradient(closest-side, white 79%, transparent 80% 100%),
        conic-gradient(var(--primary) var(--p), #ddd 0);
      display: flex;
      align-items: center;
      justify-content: center;
      font-weight: bold;
      font-size: 1.2rem;
      color: var(--text-color);
      position: relative;
    }

    .circle span {
      position: absolute;
    }

    .circle-label {
      text-align: center;
      margin-top: 0.5rem;
      font-size: 0.95rem;
      color: #555;
    }

    .circle-block {
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    .section {
      margin-bottom: 3rem;
    }

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
    
  </style>

  <script>
function showToast(m){let t=document.createElement('div');t.textContent=m;Object.assign(t.style,{position:'fixed',bottom:'24px',left:'50%',transform:'translateX(-50%)',background:'#333',color:'#fff',padding:'8px 24px',borderRadius:'4px',zIndex:1e4,fontSize:'16px',opacity:'0.96'});document.body.appendChild(t);setTimeout(()=>t.remove(),1500);}

</script>
)rawliteral";
