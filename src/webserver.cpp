#include "webserver.h"
#include "main.h"

extern unsigned long lastAlbumDisplayTime;

// ============================================================================
// HTML Templates (stored in flash memory)
// ============================================================================

const char HTML_STYLE[] PROGMEM = R"(
<style>
  body { 
    font-family: Arial, sans-serif; 
    max-width: 600px; 
    margin: 50px auto; 
    padding: 20px; 
    background: #f0f0f0; 
  }
  h1 { color: #333; }
  form { 
    background: white; 
    padding: 30px; 
    border-radius: 10px; 
    box-shadow: 0 2px 10px rgba(0,0,0,0.1); 
  }
  label { 
    display: block; 
    margin-top: 15px; 
    font-weight: bold; 
    color: #555; 
  }
  input[type='text'], 
  input[type='password'], 
  input[type='number'],
  select { 
    width: 100%; 
    padding: 8px; 
    margin-top: 5px; 
    border: 1px solid #ddd; 
    border-radius: 4px; 
    box-sizing: border-box; 
  }
  select {
    background: white;
    cursor: pointer;
  }
  input[type='checkbox'] { 
    margin-top: 10px; 
  }
  input[type='submit'] { 
    background: #4CAF50; 
    color: white; 
    padding: 12px 30px; 
    border: none; 
    border-radius: 4px; 
    cursor: pointer; 
    margin-top: 20px; 
    font-size: 16px; 
  }
  input[type='submit']:hover { 
    background: #45a049; 
  }
  .info { 
    background: #e7f3fe; 
    padding: 10px; 
    border-left: 4px solid #2196F3; 
    margin: 20px 0; 
  }
  .success {
    background: #d4edda;
    padding: 15px;
    border-left: 4px solid #28a745;
    margin: 20px 0;
    border-radius: 4px;
  }
</style>
)";

const char HTML_SETTINGS_PAGE[] PROGMEM = R"(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset='UTF-8'>
  <title>ScryerBook Settings</title>
  %STYLE%
</head>
<body>
  <h1>ScryerBook Settings</h1>
  <div class='info'>
    <strong>Device Info:</strong><br>
    IP Address: %IPADDR%<br>
    Version: %VERSION%
  </div>
  <form method='POST' action='/'>
    <label>WiFi SSID:</label>
    <input type='text' name='wifiSSID' value='%SSID%'>
    
    <label>WiFi Password:</label>
    <input type='password' name='wifiPassword' value='%PASSWORD%'>
    
    <label>Image Server Address:</label>
    <input type='text' name='imageServer' value='%IMGSERVER%'>
    
    <label>Image Path/Query:</label>
    <input type='text' name='imagePath' value='%IMGPATH%'>
    
    <label>Fetch Interval (seconds):</label>
    <input type='number' name='fetchInterval' value='%INTERVAL%' min='1'>
    
    <label>Display Mode:</label>
    <select name='displayMode'>
      <option value='FETCH'%FETCH_SELECTED%>Fetch (Dynamic Image)</option>
      <option value='CLOCK'%CLOCK_SELECTED%>Clock</option>
      <option value='ALBUM'%ALBUM_SELECTED%>Album (Static Image)</option>
      <option value='QUOTE'%QUOTE_SELECTED%>Quote</option>
      <option value='STATUSINFO'%STATUSINFO_SELECTED%>Status Info</option>
      <option value='NETWORK'%NETWORK_SELECTED%>Network Picker</option>
    </select>
    
    <label>Album Mode:</label>
    <select name='albumMode'>
      <option value='nt'%ALBUM_NT_SELECTED%>Nature</option>
      <option value='ar'%ALBUM_AR_SELECTED%>Synthwave</option>
    </select>

    <input type='submit' value='Save Settings'>
  </form>
</body>
</html>
)";

const char HTML_SUCCESS_PAGE[] PROGMEM = R"(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset='UTF-8'>
  <title>Settings Saved</title>
  %STYLE%
</head>
<body>
  <h1>Settings Saved!</h1>
  <div class='success'>
    <p>Your settings have been saved successfully.</p>
  </div>
  <p><a href='/'>Back to Settings</a></p>
  <p><em>Note: WiFi changes require a restart to take effect.</em></p>
</body>
</html>
)";

// ============================================================================
// Helper Functions
// ============================================================================

// Helper for URL decode
unsigned char h2int(char c) {
  if (c >= '0' && c <= '9') {
    return((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return((unsigned char)c - 'A' + 10);
  }
  return(0);
}

// URL decode helper function
String urlDecode(String str) {
  String decoded = "";
  char c;
  char code0;
  char code1;
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%') {
      i++;
      code0 = str.charAt(i);
      i++;
      code1 = str.charAt(i);
      c = (h2int(code0) << 4) | h2int(code1);
      decoded += c;
    } else {
      decoded += c;
    }
  }
  return decoded;
}

// Handle web server requests
void handleWebServer() {
  WiFiClient webClient = webServer.available();
  
  if (webClient) {
    Serial.println("New web client");
    String currentLine = "";
    String request = "";
    bool isPost = false;
    String postData = "";
    
    while (webClient.connected()) {
      if (webClient.available()) {
        char c = webClient.read();
        request += c;
        
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // End of headers, check if POST
            if (isPost && postData.length() == 0) {
              // Read POST data
              while (webClient.available()) {
                postData += (char)webClient.read();
              }
              
              // Parse POST data and update settings
              if (postData.length() > 0) {
                // Parse form data
                int idx;
                
                // WiFi SSID
                idx = postData.indexOf("wifiSSID=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  wifiSSID = urlDecode(postData.substring(idx + 9, endIdx));
                }
                
                // WiFi Password
                idx = postData.indexOf("wifiPassword=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  wifiPassword = urlDecode(postData.substring(idx + 13, endIdx));
                }
                
                // Image Server
                idx = postData.indexOf("imageServer=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  imageServerAddress = urlDecode(postData.substring(idx + 12, endIdx));
                }
                
                // Image Path
                idx = postData.indexOf("imagePath=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  imageServerPath = urlDecode(postData.substring(idx + 10, endIdx));
                }
                
                // Fetch Interval
                idx = postData.indexOf("fetchInterval=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  String intervalStr = postData.substring(idx + 14, endIdx);
                  fetchInterval = intervalStr.toInt() * 1000; // Convert seconds to milliseconds
                }
                
                // Display Mode dropdown
                idx = postData.indexOf("displayMode=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  String modeStr = urlDecode(postData.substring(idx + 12, endIdx));
                  
                  // Convert string to MODE enum
                  if (modeStr == "FETCH") _mode = FETCH;
                  else if (modeStr == "CLOCK") _mode = CLOCK;
                  else if (modeStr == "ALBUM") _mode = ALBUM;
                  else if (modeStr == "QUOTE") _mode = QUOTE;
                  else if (modeStr == "STATUSINFO") _mode = STATUSINFO;
                  else if (modeStr == "NETWORK") _mode = NETWORK;
                  
                  // Update legacy flags for backward compatibility
                  useStaticImage = (_mode == ALBUM);
                  useClockMode = (_mode == CLOCK);
                }
                
                // Album Mode dropdown
                idx = postData.indexOf("albumMode=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  String albumModeStr = urlDecode(postData.substring(idx + 10, endIdx));
                  if (albumModeStr == "nt") {
                    albumMode = NATURE;
                    lastAlbumDisplayTime = 99999999; // Force immediate update
                  } else if (albumModeStr == "ar") {
                    albumMode = SYNTHWAVE;
                    lastAlbumDisplayTime = 99999999; // Force immediate update
                  }
                }
                
                // Save settings
                saveSettings();
                
                // Send success response
                webClient.println("HTTP/1.1 200 OK");
                webClient.println("Content-Type: text/html");
                webClient.println("Connection: close");
                webClient.println();
                
                // Build success page with placeholders
                String html = String(HTML_SUCCESS_PAGE);
                html.replace("%STYLE%", HTML_STYLE);
                
                webClient.print(html);
                break;
              }
            }
            
            // Send HTML form (GET request)
            webClient.println("HTTP/1.1 200 OK");
            webClient.println("Content-Type: text/html");
            webClient.println("Connection: close");
            webClient.println();
            
            // Build settings page with placeholders
            String html = String(HTML_SETTINGS_PAGE);
            html.replace("%STYLE%", HTML_STYLE);
            html.replace("%IPADDR%", WiFi.localIP().toString());
            html.replace("%VERSION%", VERSION);
            html.replace("%SSID%", wifiSSID);
            html.replace("%PASSWORD%", wifiPassword);
            html.replace("%IMGSERVER%", imageServerAddress);
            html.replace("%IMGPATH%", imageServerPath);
            html.replace("%INTERVAL%", String(fetchInterval / 1000));
            
            // Set selected option for display mode dropdown
            html.replace("%FETCH_SELECTED%", _mode == FETCH ? " selected" : "");
            html.replace("%CLOCK_SELECTED%", _mode == CLOCK ? " selected" : "");
            html.replace("%ALBUM_SELECTED%", _mode == ALBUM ? " selected" : "");
            html.replace("%QUOTE_SELECTED%", _mode == QUOTE ? " selected" : "");
            html.replace("%STATUSINFO_SELECTED%", _mode == STATUSINFO ? " selected" : "");
            html.replace("%NETWORK_SELECTED%", _mode == NETWORK ? " selected" : "");
            
            // Set selected option for album mode dropdown
            html.replace("%ALBUM_NT_SELECTED%", albumMode == NATURE ? " selected" : "");
            html.replace("%ALBUM_AR_SELECTED%", albumMode == SYNTHWAVE ? " selected" : "");
            
            webClient.print(html);
            break;
          } else {
            // Check for POST request
            if (currentLine.startsWith("POST")) {
              isPost = true;
            }
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    
    delay(1);
    webClient.stop();
    Serial.println("Web client disconnected");
  }
}
