#pragma once

#include "wled.h"
#ifndef WLED_ENABLE_MQTT
#error "This user mod requires MQTT to be enabled."
#endif

#ifdef ESP8266
  #include "WiFiClientSecure.h"
#else // ESP32
#endif

// This is a Metar map user mod allow user to use individual leds to present airports metar fly category.
class MetarMapUsermod : public Usermod {
  private:
    // strings to reduce flash memory usage (used more than twice)
    static const char _name[];

    unsigned long lastUpdateTime = 0;
    // max delay if no update from mqtt
    unsigned long maxRefreshDelaySec = 3600;

    uint16_t ledTotalLen = 0;
    
    bool pendingColorUpdate = false;

    bool mqttInitialized = false;

    // This config should be pulled from readFromConfig()
    const char* airports[15] = {
      "KFRR","KOKV","KMRB","KFDK","KGAI",
      "KJYO","KIAD","KDCA","KCGS","KBWI",
      "KMTN","KDMW","KHGR","KTHV","KLNS"
    };

    uint32_t flight_catergory_color[15] = {};

    String mqttFlightCategoryTopic = "";
    String mqttWindSpeedTopic = "";
    String mqttVisibilityTopic = "";
    String mqttSkyConditionTopic = "";
    String mqttObservationTimeTopic = "";

    /*    
       green - VFR (Visual Flight Rules)
       blue - MVFR (Marginal Visual Flight Rules)
       red - IFR (Instrument Flight Rules)
       purple - LIFR (Low Instrument Flight Rules)
    */
    uint32_t VFR_COLOR = GREEN;
    uint32_t MVFR_COLOR = BLUE;
    uint32_t IFR_COLOR = RED;
    uint32_t LIFR_COLOR = PURPLE;
    uint32_t ERR = YELLOW;

    void updateStrip() {
      if (strip.isUpdating()) return;
      
      strip.setBrightness(scaledBri(bri), true);
      // freeze and init to black
      Segment& seg = strip.getSegment(0);
      if (!seg.freeze) {
        seg.freeze = true;
        seg.fill(BLACK);
      } 
      // use transition
      seg.setOption(SEG_OPTION_ON, true); 
      for (uint8_t i = 0; i < sizeof(flight_catergory_color)/sizeof(flight_catergory_color[0]); i ++) {
        strip.setPixelColor(i, flight_catergory_color[i]);
      }
      pendingColorUpdate = false;
      Serial.println("Strip color updated");
    }

    // fill strip with passed in color
    void fillArrayWithColor(uint32_t color_arry[], uint32_t color) {
      for (uint8_t i = 0; i < sizeof(flight_catergory_color)/sizeof(flight_catergory_color[0]); i ++) {
        flight_catergory_color[i] = color;
      }
    }

    void mqttInitialize()
    {
      mqttFlightCategoryTopic = String(mqttDeviceTopic) + "/+/flight_category";
      mqttWindSpeedTopic = String(mqttDeviceTopic) + "/+/wind_speed_kt";
      mqttVisibilityTopic = String(mqttDeviceTopic) + "/+/visibility_statute_mi";
      // mqttSkyConditionTopic = String(mqttDeviceTopic) + "/+/sky_condition"; // [TODO]fix parsing bug
      mqttObservationTimeTopic = String(mqttDeviceTopic) + "/+/observation_time";
      
      mqtt->subscribe(mqttFlightCategoryTopic.c_str(), 0);
      mqtt->subscribe(mqttWindSpeedTopic.c_str(), 0);
      mqtt->subscribe(mqttVisibilityTopic.c_str(), 0);
      // mqtt->subscribe(mqttSkyConditionTopic.c_str(), 0); // [TODO]fix parsing bug
      mqtt->subscribe(mqttObservationTimeTopic.c_str(),0);

      Serial.println("MQTT topic subscribed");
    }
    
  public:

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() {
      Serial.println("Hello from MetarMap UserMod!");
      ledTotalLen = strip.getLengthTotal();
      // Flash light to indicate power on
    }

    /**
     * subscribe to MQTT topic for controlling usermod
     */
    void onMqttConnect(bool sessionPresent) {
      //(re)subscribe to required topics
      Serial.println("MQTT Connected");
      if (!mqttInitialized){
        mqttInitialize();
        mqttInitialized = true;
      }
    }

    /**
     * handling of MQTT message
     * topic only contains mapped airport
     */
    bool onMqttMessage(char* topic, char* payload) {
      const char delim[] = "/";
      char *airportCode = NULL;
      char *wxField = NULL;

      airportCode = strtok(topic, delim);
      wxField = strtok(NULL, delim);

      int airportIndex = getIndexByAirport(airportCode);
      if (airportIndex < 0) {
        return false;
      }

      Serial.print(airportCode);
      Serial.print(" - ");
      Serial.print(wxField);
      Serial.print(" : ");
      Serial.println(payload);

      if (strcmp(wxField, "flight_category") == 0) {
        if(strcmp(payload, "VFR") == 0) {
          flight_catergory_color[airportIndex] = VFR_COLOR;
        } else if (strcmp(payload, "MVFR") == 0) {
          flight_catergory_color[airportIndex] = MVFR_COLOR;
        } else if (strcmp(payload, "IFR") == 0) {
          flight_catergory_color[airportIndex] = IFR_COLOR;
        } else if (strcmp(payload, "LIFR") == 0) {
          flight_catergory_color[airportIndex] = LIFR_COLOR;
        } else {
          flight_catergory_color[airportIndex] = ERR;
        }
      }

      pendingColorUpdate = true;
      lastUpdateTime = millis();
      return true;
    }

    int getIndexByAirport(char* airportCode) {
      for (int i = 0; i < (int)(sizeof(airports)/sizeof(airports[0])); i++) {
        if (strcmp(airportCode, airports[i]) == 0){
          return i;
        }
      }
      return -1;
    }

    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() {
      Serial.print("Local IP: ");
      Serial.println(WiFi.localIP());
    }

    /*
     * handleOverlayDraw() is called just before every show() (LED strip update frame) after effects have set the colors.
     * Use this to blank out some LEDs or set them to a different color regardless of the set effect mode.
     * Commonly used for custom clocks (Cronixie, 7 segment)
     */
    void handleOverlayDraw()
    {
        if (pendingColorUpdate) {
          updateStrip();
        }
    }

    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     * 
     */
    void loop() {
      if (millis() - lastUpdateTime > maxRefreshDelaySec*1000 ) {
        // lastUpdateTime is updated on MQTT message
        // If no MQTT message is received over maxRefreshDelaySec time, force all lights into white
        fillArrayWithColor(flight_catergory_color, WHITE);
        pendingColorUpdate = true;
      }
    }

    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * If you want to force saving the current state, use serializeConfig() in your loop().
     * 
     */
    void addToConfig(JsonObject& root)
    {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      JsonArray airportCodeArr = top.createNestedArray(F("ICAOS")); 
      uint16_t airportListLength = sizeof(airports)/sizeof(airports[0]);

      if (ledTotalLen > 0 && ledTotalLen == airportListLength) {
        for (int i = 0; i < ledTotalLen; i++) {
          airportCodeArr.add(airports[i]);
        }
      }
    }

    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens immediately after boot, or after saving on the Usermod Settings page)
     * 
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     * 
     * Return true in case the config values returned from Usermod Settings were complete, or false if you'd like WLED to save your defaults to disk (so any missing values are editable in Usermod Settings)
     * 
     * getJsonValue() returns false if the value is missing, or copies the value into the variable provided and returns true if the value is present
     * The configComplete variable is true only if the "exampleUsermod" object and all values are present.  If any values are missing, WLED will know to call addToConfig() to save them
     * 
     * This function is guaranteed to be called on boot, but could also be called every time settings are updated
     */
    bool readFromConfig(JsonObject& root)
    {
      JsonObject top = root[FPSTR(_name)];
      if (top.isNull()){
        return false;
      }
      bool configComplete = !top.isNull();

      const char* airportList[ledTotalLen];

      if(!top[F("ICAOS")].isNull()) {
        Serial.print("read from config:"); // [TODO:] reading config to airportList is not working
        for (int i = 0; i < ledTotalLen; i++) {
          configComplete &= getJsonValue(top[F("ICAOS")][i], airportList[i], airports[i]);
          Serial.println(airportList[i]);
        }
      }

      return configComplete;
    }


    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId()
    {
      return USERMOD_ID_METAR_MAP;
    }
};

// strings to reduce flash memory usage (used more than twice)
const char MetarMapUsermod::_name[] PROGMEM = "MetarMapUserMod";