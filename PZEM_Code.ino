#define BLYNK_TEMPLATE_ID "TMPL6f6PYVCL5"
#define BLYNK_TEMPLATE_NAME "Power Socket"
#define BLYNK_AUTH_TOKEN "mQtwp0xZuhfzP_W9WBB2jjfnPwaujLYw"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PZEM004Tv30.h>

// Initialize LCD with I2C address 0x27, 16 columns, and 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char ssid[] = "Galaxy M02s6830";
const char pass[] = "endq8847";

// Timer for regular updates
BlynkTimer timer;

#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 17
#define PZEM_TX_PIN 16
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif

#if defined(ESP32)
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);
#elif defined(ESP8266)
// Uncomment the appropriate PZEM initialization for ESP8266
// PZEM004Tv30 pzem(Serial1);
#else
PZEM004Tv30 pzem(PZEM_SERIAL);
#endif

// Variables for energy calculation
float kWh = 0.0;
float cost = 0.0;
unsigned long lastMillis = millis();

// Update the Google Script ID here
String GOOGLE_SCRIPT_ID = "AKfycbznBD5PnadwWmyrcBBWr-XwtWMm9Ejo2dK8O7xvKDd8ajCpjVAMEkKQjY88Hit5Sjxk"; // replace YOUR_NEW_SCRIPT_ID with the actual script ID

void setup() {
    // Debugging Serial port
    Serial.begin(115200);
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    // Initialize I2C bus
    Wire.begin();

    // Initialize the LCD
    lcd.begin(16, 2); 
    lcd.backlight();

    // Start the scrolling text
    scrollText("SMART PLUG SOCKET by VeloSys Technologies", 150);
}

void loop() {
    // Ensure WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        // Display "SMART ENERGY METER" on LCD when WiFi is not connected
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("SMART PLUG");
        lcd.setCursor(0, 1);
        lcd.print("SOCKET by VeloSys");

        Serial.println("Reconnecting to WiFi...");
        WiFi.begin(ssid, pass);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("Reconnected to WiFi.");
        delay(2000); // Small delay to ensure stable connection before proceeding
    } else {
        // Read the data from the sensor
        float voltage = pzem.voltage();
        float current = pzem.current();
        float power = pzem.power();
        float energy = pzem.energy();
        float frequency = pzem.frequency();
        float pf = pzem.pf();

        // Calculate energy consumed in kWh
        unsigned long currentMillis = millis();
        kWh += power * (currentMillis - lastMillis) / 3600000000.0;
        lastMillis = currentMillis;

        // Calculate cost based on the provided tariff
        if (kWh <= 30) {
            cost = kWh * 12;
        } else if (kWh <= 60) {
            cost = (30 * 12) + ((kWh - 30) * 30);
        } else if (kWh <= 90) {
            cost = (30 * 12) + (30 * 30) + ((kWh - 60) * 38);
        } else if (kWh <= 120) {
            cost = (30 * 12) + (30 * 30) + (30 * 38) + ((kWh - 90) * 41);
        } else if (kWh <= 180) {
            cost = (30 * 12) + (30 * 30) + (30 * 38) + (30 * 41) + ((kWh - 120) * 59);
        } else {
            cost = (30 * 12) + (30 * 30) + (30 * 38) + (30 * 41) + (60 * 59) + ((kWh - 180) * 89);
        }

        // Check if the data is valid
        if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy) || isnan(frequency) || isnan(pf)) {
            Serial.println("Error reading from PZEM sensor");
        } else {
            // Print the values to the Serial console
            Serial.print("Voltage: "); Serial.print(voltage); Serial.println("V");
            Serial.print("Current: "); Serial.print(current); Serial.println("A");
            Serial.print("Power: "); Serial.print(power); Serial.println("W");
            Serial.print("Energy: "); Serial.print(kWh, 4); Serial.println("kWh");
            Serial.print("Frequency: "); Serial.print(frequency, 1); Serial.println("Hz");
            Serial.print("PF: "); Serial.println(pf);
            Serial.print("Cost: "); Serial.print("Rs"); Serial.println(cost, 2);
        }

        // Construct the URL with all parameters
        String urlFinal = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?"
                        + "voltage=" + String(voltage) 
                        + "&current=" + String(current) 
                        + "&units=" + String(kWh)   // Add units (kWh) to URL
                        + "&price=" + String(cost); // Add cost to URL

        // Print the URL for debugging
        Serial.print("POST data to spreadsheet: ");
        Serial.println(urlFinal);

        // Send HTTP GET request to Google Sheets script
        HTTPClient http;
        http.begin(urlFinal.c_str());
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET();
        Serial.print("HTTP Status Code: ");
        Serial.println(httpCode);

        // Get the response from the server
        String payload;
        if (httpCode > 0) {
            payload = http.getString();
            Serial.println("Payload: " + payload);
        }

        http.end();
        delay(1000);

        // Display voltage, current, power, energy, power factor, and frequency on the LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Voltage: ");
        lcd.print(voltage, 2);
        lcd.print("V");

        lcd.setCursor(0, 1);
        lcd.print("Current: ");
        lcd.print(current, 2);
        lcd.print(" A");

        delay(2000);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Power: ");
        lcd.print(power, 2);
        lcd.print(" W");

        lcd.setCursor(0, 1);
        lcd.print("Energy: ");
        lcd.print(kWh, 3);
        lcd.print("kWh");

        delay(2000);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Freq: ");
        lcd.print(frequency, 2);
        lcd.print(" Hz");

        lcd.setCursor(0, 1);
        lcd.print("PF: ");
        lcd.print(pf, 2);

        delay(2000);

        // Send data to Blynk
        Blynk.virtualWrite(V0, voltage);
        Blynk.virtualWrite(V1, current);
        Blynk.virtualWrite(V2, power);
        Blynk.virtualWrite(V3, kWh);
        Blynk.virtualWrite(V4, pf);
        Blynk.virtualWrite(V5, frequency);
        Blynk.virtualWrite(V6, kWh);   // Units Consumed (kWh) on Virtual Pin V6
        Blynk.virtualWrite(V7, cost);  // Cost on Virtual Pin V7

        // Run Blynk and timer
        Blynk.run();
        timer.run();
    }
}

void scrollText(String text, int delayTime) {
    int textLength = text.length();
    for (int i = 0; i < textLength + 16; i++) { // +16 for complete visibility at the end
        lcd.clear();
        lcd.setCursor(0, 0);

        if (i < 16) {
            // Display leading empty characters
            int spaces = 16 - i;
            String displayText = text.substring(0, i);
            lcd.print(repeatString(" ", spaces) + displayText);
        } else if (i < textLength) {
            // Display substring of text
            lcd.print(text.substring(i - 16, i));
        } else {
            // Ending spaces to move text out of display area
            int endIndex = i - 16 < textLength ? textLength : i - 16;
            lcd.print(text.substring(endIndex, textLength));
        }

        delay(delayTime); // Control speed of scrolling
    }
}

String repeatString(String s, int n) {
    String result;
    for (int i = 0; i < n; i++) {
        result += s;
    }
    return result;
}
