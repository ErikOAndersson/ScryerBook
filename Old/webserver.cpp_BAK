#include "webserver.h"

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
                
                // Static Image checkbox
                useStaticImage = (postData.indexOf("useStatic=on") >= 0);
                // Clock Mode checkbox
                useClockMode = (postData.indexOf("useClockMode=on") >= 0);
                // Save settings
                saveSettings();
                
                // Send success response
                webClient.println("HTTP/1.1 200 OK");
                webClient.println("Content-Type: text/html");
                webClient.println("Connection: close");
                webClient.println();
                webClient.println("<!DOCTYPE HTML><html><head><meta charset='UTF-8'><title>Settings Saved</title></head>");
                webClient.println("<body><h1>Settings Saved!</h1>");
                webClient.println("<p>Your settings have been saved successfully.</p>");
                webClient.println("<p><a href='/'>Back to Settings</a></p>");
                webClient.println("<p><em>Note: WiFi changes require a restart to take effect.</em></p>");
                webClient.println("</body></html>");
                break;
              }
            }
            
            // Send HTML form (GET request)
            webClient.println("HTTP/1.1 200 OK");
            webClient.println("Content-Type: text/html");
            webClient.println("Connection: close");
            webClient.println();
            
            // HTML form
            webClient.println("<!DOCTYPE HTML><html><head><meta charset='UTF-8'><title>ScryerBook Settings</title>");
            webClient.println("<style>");
            webClient.println("body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; background: #f0f0f0; }");
            webClient.println("h1 { color: #333; }");
            webClient.println("form { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }");
            webClient.println("label { display: block; margin-top: 15px; font-weight: bold; color: #555; }");
            webClient.println("input[type='text'], input[type='password'], input[type='number'] { width: 100%; padding: 8px; margin-top: 5px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }");
            webClient.println("input[type='checkbox'] { margin-top: 10px; }");
            webClient.println("input[type='submit'] { background: #4CAF50; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; margin-top: 20px; font-size: 16px; }");
            webClient.println("input[type='submit']:hover { background: #45a049; }");
            webClient.println(".info { background: #e7f3fe; padding: 10px; border-left: 4px solid #2196F3; margin: 20px 0; }");
            webClient.println("</style></head><body>");
            webClient.println("<h1>ScryerBook Settings</h1>");
            webClient.println("<div class='info'>Static IP: 192.168.1.61</div>");
            webClient.println("<form method='POST' action='/'>");
            
            webClient.println("<label>WiFi SSID:</label>");
            webClient.print("<input type='text' name='wifiSSID' value='");
            webClient.print(wifiSSID);
            webClient.println("'>");
            
            webClient.println("<label>WiFi Password:</label>");
            webClient.print("<input type='password' name='wifiPassword' value='");
            webClient.print(wifiPassword);
            webClient.println("'>");
            
            webClient.println("<label>Image Server Address:</label>");
            webClient.print("<input type='text' name='imageServer' value='");
            webClient.print(imageServerAddress);
            webClient.println("'>");
            
            webClient.println("<label>Image settings:</label>");
            webClient.print("<input type='text' name='imagePath' value='");
            webClient.print(imageServerPath);
            webClient.println("'>");
            
            webClient.println("<label>Fetch Interval (seconds):</label>");
            webClient.print("<input type='number' name='fetchInterval' value='");
            webClient.print(fetchInterval / 1000);
            webClient.println("' min='1'>");
            
            webClient.println("<label>");
            webClient.print("<input type='checkbox' name='useStatic'");
            if (useStaticImage) webClient.print(" checked");
            webClient.println("> Use Static Image (don't fetch from server)</label>");

            webClient.println("<label>");
            webClient.print("<input type='checkbox' name='useClockMode'");
            if (useClockMode) webClient.print(" checked");
            webClient.println("> Use Clock Mode</label>");

            webClient.println("<input type='submit' value='Save Settings'>");
            webClient.println("</form></body></html>");
            
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
