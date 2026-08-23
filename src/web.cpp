#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <globals.h>
#include <timeData.h>
#include "SPIFFS.h"

String getIndexHTML();
String templateProcessor(const String& var);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setupServer(){
    initWebSocket();

    // Android captive-portal detection: a plain 200 doesn't reliably trigger the
    // sign-in notification, a redirect to the portal does.
    server.on("/generate_204", HTTP_GET, [] (AsyncWebServerRequest *request) {
        request->redirect("http://" + WiFi.softAPIP().toString() + "/");
    }).setFilter(ON_AP_FILTER);
    server.on("/gen_204", HTTP_GET, [] (AsyncWebServerRequest *request) {
        request->redirect("http://" + WiFi.softAPIP().toString() + "/");
    }).setFilter(ON_AP_FILTER);
    // Apple/Windows probes: any non-204/expected response is enough to make them
    // offer the captive portal; the CaptiveRequestHandler catch-all below already
    // covers these, these routes just make the intent explicit.
    server.on("/hotspot-detect.html", HTTP_GET, [] (AsyncWebServerRequest *request) {
        request->send(SPIFFS,"/wortuhr.html","text/html",false,templateProcessor);
    }).setFilter(ON_AP_FILTER);
    server.on("/library/test/success.html", HTTP_GET, [] (AsyncWebServerRequest *request) {
        request->send(SPIFFS,"/wortuhr.html","text/html",false,templateProcessor);
    }).setFilter(ON_AP_FILTER);
    server.on("/ncsi.txt", HTTP_GET, [] (AsyncWebServerRequest *request) {
        request->send(SPIFFS,"/wortuhr.html","text/html",false,templateProcessor);
    }).setFilter(ON_AP_FILTER);
    server.on("/connecttest.txt", HTTP_GET, [] (AsyncWebServerRequest *request) {
        request->send(SPIFFS,"/wortuhr.html","text/html",false,templateProcessor);
    }).setFilter(ON_AP_FILTER);
    server.on("/redirect", HTTP_GET, [] (AsyncWebServerRequest *request) {
        request->send(SPIFFS,"/wortuhr.html","text/html",false,templateProcessor);
    }).setFilter(ON_AP_FILTER);

    server.serveStatic("/",SPIFFS,"/");
    server.onNotFound([](AsyncWebServerRequest *request){
        if (request->url() == "/"){
            request->send(SPIFFS,"/wortuhr.html","text/html");
        }else{
            request->send(404);
        } 
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS,"/wortuhr.html","text/html");
    });

    server.on("/api/config", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String config = "{";
        config += "\"time\":"+ String(myTimeData.hour*3600+ myTimeData.minute*60+myTimeData.second) + ",";
        config += "\"baseColor\":{\"r\":" + String(baseColorDay.r) + ",\"g\":" + String(baseColorDay.g) + ",\"b\":" + String(baseColorDay.b) + ",\"mode\":\"" + designDay + "\",\"brightness\":" + String(brightnessDay) + "},";
        if(nightModeActive){
            config += "\"nightMode\":{\"enabled\":true,";
        }else{
            config += "\"nightMode\":{\"enabled\":false,";
        }
        config += "\"startH\":" + String(nightModeBeginHour) + ",\"startM\":" + String(nightModeBeginMinute) + ",\"endH\":" + String(nightModeEndHour) + ",\"endM\":" + String(nightModeEndMinute) + ",";
        config += "\"baseColor\":{\"r\":" + String(baseColorNight.r) + ",\"g\":" + String(baseColorNight.g) + ",\"b\":" + String(baseColorNight.b) + ",\"mode\":\"" + designNight + "\",\"brightness\":" + String(brightnessNight) + "}},";        
        config += "\"WiFi\":\"" + ssid + "\",\"password\":\"" + password + "\",\"modes\":[" + designOptions + "],\"wiFiSSIDs\":[" + KnownSSIDsList + "],\"language\":\"" + language + "\",\"timezone\":\"" + timezone + "\",\"apMode\":" + (APMode ? "true" : "false") + "}";
        request->send_P(200, "application/json", config.c_str()); 
    });

    server.on("/api/dayColor",HTTP_POST, [] (AsyncWebServerRequest * request){
        String inputMessage;
        String newDesign = designDay;
        CRGB newColor = baseColorDay;
        uint8_t newBrightness = brightnessDay;

        if (request->hasParam("mode", true)) {
            inputMessage = request->getParam("mode", true)->value();
            if(designOptions.indexOf(inputMessage) == -1){
                request->send_P(400, "text/html","mode not available");
                return;
            }
            newDesign = inputMessage;
            Serial.println(inputMessage);
        }
        if (request->hasParam("r", true)) {
            inputMessage = request->getParam("r", true)->value();
            Serial.print("R: ");
            Serial.println(inputMessage);
            newColor.r = atoi(inputMessage.c_str());
        }
        if (request->hasParam("g", true)) {
            inputMessage = request->getParam("g", true)->value();
            Serial.print("G: ");
            Serial.println(inputMessage);
            newColor.g = atoi(inputMessage.c_str());
        }
        if (request->hasParam("b", true)) {
            inputMessage = request->getParam("b", true)->value();
            Serial.print("B: ");
            Serial.println(inputMessage);
            newColor.b = atoi(inputMessage.c_str());
        }
        if (request->hasParam("brightness", true)) {
            inputMessage = request->getParam("brightness", true)->value();
            Serial.print("brightness: ");
            Serial.println(inputMessage);
            newBrightness = atoi(inputMessage.c_str());
        }

        preferences.begin("wortuhr",false);
        preferences.putString("design",newDesign);
        preferences.putInt("baseColorR",newColor.r);
        preferences.putInt("baseColorG",newColor.g);
        preferences.putInt("baseColorB",newColor.b);
        preferences.putInt("brightness",newBrightness);
        preferences.end();
        designDay = newDesign;
        baseColorDay = newColor;
        brightnessDay = newBrightness;
        if(!NightMode){
            design = newDesign;
            baseColor = newColor;
            brightness = newBrightness;
            FastLED.setBrightness(brightness);
        }
        myTimeData.updateColor();

        request->send_P(200, "text/html","ok");
    });

    server.on("/api/wifi", HTTP_POST, [] (AsyncWebServerRequest *request) {
        String inputMessage;
        String inputParam;
        String newSsid;
        String newPassword;

        if (request->hasParam("ssid", true)) {
            inputMessage = request->getParam("ssid", true)->value();
            inputParam = "ssid";
            newSsid = inputMessage;
            Serial.println(inputMessage);
        }else{
            request->send_P(400, "text/html","No SSID given");
            return;
        }

        if (request->hasParam("password", true)) {
            inputMessage = request->getParam("password", true)->value();
            inputParam = "password";
            newPassword = inputMessage;
            Serial.println(inputMessage);
        }else{
            request->send_P(400, "text/html","No Password given");
            return;
        }

        ssid = newSsid;
        password = newPassword;
        preferences.begin("wortuhr",false);
        preferences.putString("ssid",newSsid);
        preferences.putString("password",newPassword);
        preferences.end();

        request->send_P(200, "text/html","ok");
        delay(500);
        ESP.restart();
    });

    server.on("/color", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String inputMessage;
        String newDesign = designDay;
        CRGB newColor = baseColorDay;
        uint8_t newBrightness = brightnessDay;

        if (request->hasParam("design")) {
            inputMessage = request->getParam("design")->value();
            newDesign = inputMessage;
            Serial.println(inputMessage);
        }

        if (request->hasParam("colorR")) {
            inputMessage = request->getParam("colorR")->value();
            Serial.print("R: ");
            Serial.println(inputMessage);
            newColor.r = atoi(inputMessage.c_str());
        }
        if (request->hasParam("colorG")) {
            inputMessage = request->getParam("colorG")->value();
            Serial.print("G: ");
            Serial.println(inputMessage);
            newColor.g = atoi(inputMessage.c_str());
        }
        if (request->hasParam("colorB")) {
            inputMessage = request->getParam("colorB")->value();
            Serial.print("B: ");
            Serial.println(inputMessage);
            newColor.b = atoi(inputMessage.c_str());
        }
        if (request->hasParam("brightness")) {
            inputMessage = request->getParam("brightness")->value();
            Serial.print("brightness: ");
            Serial.println(inputMessage);
            newBrightness = atoi(inputMessage.c_str());
        }
        
        preferences.begin("wortuhr",false);
        preferences.putString("design",newDesign);
        preferences.putInt("baseColorR",newColor.r);
        preferences.putInt("baseColorG",newColor.g);
        preferences.putInt("baseColorB",newColor.b);
        preferences.putInt("brightness",newBrightness);
        preferences.end();
        designDay = newDesign;
        baseColorDay = newColor;
        brightnessDay = newBrightness;
        if(!NightMode){
            design = newDesign;
            baseColor = newColor;
            brightness = newBrightness;
            FastLED.setBrightness(brightness);
        }
        myTimeData.updateColor();
        request->redirect("/");
    });

    // night
    server.on("/colorNight", HTTP_POST, [] (AsyncWebServerRequest *request) {
        String inputMessage;
        String newDesign = designNight;
        CRGB newColor = baseColorNight;
        uint8_t newBrightness = brightnessNight;
        bool active = nightModeActive;
        uint8_t newBeginHour = nightModeBeginHour;
        uint8_t newBeginMinute = nightModeBeginMinute;
        uint8_t newEndHour = nightModeEndHour;
        uint8_t newEndMinute = nightModeEndMinute;

        if (request->hasParam("nightModeActiveInt", true)) {
            inputMessage = request->getParam("nightModeActiveInt", true)->value();
            Serial.print("active: ");
            Serial.println(inputMessage);
            int activeInt = atoi(inputMessage.c_str());
            if(activeInt == 1){
                active = true;
            }else{
                active = false;
            }
        }
        if (request->hasParam("nightModeBeginHour", true)) {
            inputMessage = request->getParam("nightModeBeginHour", true)->value();
            Serial.print("BeginH: ");
            Serial.println(inputMessage);
            newBeginHour = atoi(inputMessage.c_str());
        }
        if (request->hasParam("nightModeBeginMinute", true)) {
            inputMessage = request->getParam("nightModeBeginMinute", true)->value();
            Serial.print("BeginM: ");
            Serial.println(inputMessage);
            newBeginMinute = atoi(inputMessage.c_str());
        }
        if (request->hasParam("nightModeEndHour", true)) {
            inputMessage = request->getParam("nightModeEndHour", true)->value();
            Serial.print("EndH: ");
            Serial.println(inputMessage);
            newEndHour = atoi(inputMessage.c_str());
        }
        if (request->hasParam("nightModeEndMinute", true)) {
            inputMessage = request->getParam("nightModeEndMinute", true)->value();
            Serial.print("EndM: ");
            Serial.println(inputMessage);
            newEndMinute = atoi(inputMessage.c_str());
        }

        if (request->hasParam("designNight", true)) {
            inputMessage = request->getParam("designNight", true)->value();
            if(designOptions.indexOf(inputMessage) == -1){
                request->send_P(400, "text/html","mode not available");
                return;
            }
            newDesign = inputMessage;
            Serial.println(inputMessage);
        }

        if (request->hasParam("colorNightR", true)) {
            inputMessage = request->getParam("colorNightR", true)->value();
            Serial.print("RNight: ");
            Serial.println(inputMessage);
            newColor.r = atoi(inputMessage.c_str());
        }
        if (request->hasParam("colorNightG", true)) {
            inputMessage = request->getParam("colorNightG", true)->value();
            Serial.print("GNight: ");
            Serial.println(inputMessage);
            newColor.g = atoi(inputMessage.c_str());
        }
        if (request->hasParam("colorNightB", true)) {
            inputMessage = request->getParam("colorNightB", true)->value();
            Serial.print("BNight: ");
            Serial.println(inputMessage);
            newColor.b = atoi(inputMessage.c_str());
        }
        if (request->hasParam("brightnessNight", true)) {
            inputMessage = request->getParam("brightnessNight", true)->value();
            Serial.print("brightnessNight: ");
            Serial.println(inputMessage);
            newBrightness = atoi(inputMessage.c_str());
        }

        preferences.begin("wortuhr",false);
        preferences.putInt("nightModeBeginH",newBeginHour);
        nightModeBeginHour = newBeginHour;
        preferences.putInt("nightModeBeginM",newBeginMinute);
        nightModeBeginMinute = newBeginMinute;
        preferences.putInt("nightModeEndH",newEndHour);
        nightModeEndHour = newEndHour;
        preferences.putInt("nightModeEndM",newEndMinute);
        nightModeEndMinute = newEndMinute;

        preferences.putBool("nightModeActive",active);
        preferences.putString("designNight",newDesign);
        preferences.putInt("baseColorNightR",newColor.r);
        preferences.putInt("baseColorNightG",newColor.g);
        preferences.putInt("baseColorNightB",newColor.b);
        preferences.putInt("brightnessNight",newBrightness);
        preferences.end();
        nightModeActive = active;
        designNight = newDesign;
        baseColorNight = newColor;
        brightnessNight = newBrightness;
        if(NightMode){
            design = newDesign;
            baseColor = newColor;
            brightness = newBrightness;
            FastLED.setBrightness(brightness);
        }
        myTimeData.checkNightMode();
        myTimeData.updateColor();
        request->send_P(200, "text/html","ok");
    });

    server.on("/api/language", HTTP_POST, [] (AsyncWebServerRequest *request) {
        if (!request->hasParam("language", true)) {
            request->send_P(400, "text/html","No language given");
            return;
        }
        String newLanguage = request->getParam("language", true)->value();
        if (newLanguage != "de" && newLanguage != "en") {
            request->send_P(400, "text/html","language not available");
            return;
        }
        language = newLanguage;
        preferences.begin("wortuhr",false);
        preferences.putString("language", newLanguage);
        preferences.end();
        request->send_P(200, "text/html","ok");
    });

    server.on("/api/timezone", HTTP_POST, [] (AsyncWebServerRequest *request) {
        if (!request->hasParam("timezone", true)) {
            request->send_P(400, "text/html","No timezone given");
            return;
        }
        String newTimezone = request->getParam("timezone", true)->value();
        if (newTimezone == "" || newTimezone.length() > 63) {
            request->send_P(400, "text/html","timezone invalid");
            return;
        }
        timezone = newTimezone;
        preferences.begin("wortuhr",false);
        preferences.putString("timezone", newTimezone);
        preferences.end();
        setTimezone(timezone);
        myTimeData.syncTime();
        myTimeData.forceRedisplay();
        request->send_P(200, "text/html","ok");
    });

    server.on("/time",[] (AsyncWebServerRequest *request){
        int timeUnix;
        if (request->hasParam("timeUnix")) {
            timeUnix = atoi(request->getParam("timeUnix")->value().c_str());
            Serial.println(timeUnix);
            DateTime dt = timeUnix;
            if (RTCAvailable){
                rtc.adjust(dt);
            }
            myTimeData.syncTime(); // force clock to syschronise from rtc if in AP mode
        }
        request->redirect("/");
    });
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String s = String((char *)data, len);
    Serial.println(s);
    // Live preview while a user drags a color/brightness/mode control, before Save.
    // Protocol: "preview:<day|night>:r,g,b,brightness,mode"
    if(s.startsWith("preview:")){
        s = s.substring(8);
        int scopeSep = s.indexOf(':');
        if (scopeSep == -1) return;
        String scope = s.substring(0, scopeSep);
        Serial.println("preview scope: " + scope);
        s = s.substring(scopeSep + 1);

        String fields[5];
        int lastIndex = 0;
        for (int i = 0; i < 5; i++) {
            int index = s.indexOf(',', lastIndex);
            if (index == -1) index = s.length();
            fields[i] = s.substring(lastIndex, index);
            lastIndex = index + 1;
        }

        String newMode = fields[4];
        if (designOptions.indexOf(newMode) == -1) return;

        baseColor = CRGB(fields[0].toInt(), fields[1].toInt(), fields[2].toInt());
        brightness = fields[3].toInt();
        design = newMode;
        FastLED.setBrightness(brightness);
        myTimeData.previewColor();

        previewMode = true;
        previewColorTriggerTimestamp = millis();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

String templateProcessor(const String& var){
    if(var == "ssidValue"){return ssid;}
    if (var == "knownSSIDs"){
        String ret = "<option></option>";
        for(int i=0;i<10;i++){
            if (KnownSSIDs[i] != ""){
                ret += "<option>" + KnownSSIDs[i] + "</option>";
            }
        }
        return ret;
    }
    if(var == "passwordValue"){return password;}
    // day
    if(var == "colorRValue"){return String(baseColorDay.r);}
    if(var == "colorGValue"){return String(baseColorDay.g);}
    if(var == "colorBValue"){return String(baseColorDay.b);}
    if(var == "designValue"){return designDay;}
    if(var == "brightnessValue"){return String(brightnessDay);}
    // night
    if(var == "nightModeActive"){
        if(nightModeActive){
            return "true";
        }else{
            return "false";
        }
    }
    if(var=="nightModeActiveInt"){
        if(nightModeActive){
            return "1";
        }else{
            return "0";
        }
    }
    if(var == "nightModeBeginHourValue"){return String(nightModeBeginHour);}
    if(var == "nightModeBeginMinuteValue"){return String(nightModeBeginMinute);}
    if(var == "nightModeEndHourValue"){return String(nightModeEndHour);}
    if(var == "nightModeEndMinuteValue"){return String(nightModeEndMinute);}
    if(var == "colorNightRValue"){return String(baseColorNight.r);}
    if(var == "colorNightGValue"){return String(baseColorNight.g);}
    if(var == "colorNightBValue"){return String(baseColorNight.b);}
    if(var == "designNightValue"){return designNight;}
    if(var == "brightnessNightValue"){return String(brightnessNight);}
    Serial.println(var + " not found");
    return "";
}