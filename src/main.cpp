#include <TinyGPSPlus.h>
#include <ACAN_T4.h>

// The TinyGPSPlus object
TinyGPSPlus gps;
CANMessage can_frame;

void displayInfo() {
    Serial.print(F("Location: "));
    if (gps.location.isValid()) {
        Serial.print(gps.location.lat(), 6);
        Serial.print(F(","));
        Serial.print(gps.location.lng(), 6);
    } else {
        Serial.print(F("INVALID"));
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200);
    Serial.println(F("A simple demonstration of TinyGPSPlus with an attached GPS module"));
    Serial.println();

    ACAN_T4_Settings settings(250 * 1000); // 250 kbit/s
    settings.mLoopBackMode = true;
    settings.mSelfReceptionMode = true;
    const uint32_t errorCode = ACAN_T4::can1.begin(settings);
    if (0 != errorCode) { Serial.println ("Invalid setting"); }
}

//https://www.hemispheregnss.com/wp-content/uploads/2020/08/875-0436-10_a1-nmea-2000-standard-messages-reference-manual-2.pdf
//129025  - Position Rapid update       - 10 Hz
//129540  - GNSS Sats in View           - 1 Hz
//126993  - Heartbeat                   - 1 Hz
//127250  - HEading, magnetic north     - 10 Hz

CANMessage generate_gps_message(uint32_t pgn, double latitude, double longitude){
    uint8_t priority = 3;
    uint8_t sa = 255;
    uint8_t data[8];
    memcpy(data, &latitude, sizeof(latitude));
    memcpy(data + sizeof(latitude), &longitude, sizeof(longitude));
    can_frame.id = ((priority & 0x07) << 26) | ((pgn & 0x3FFFF) << 8) | (sa & 0xFF);
    can_frame.len = sizeof(data);
    can_frame.ext = true;
    memcpy(can_frame.data, data, sizeof(data));
    return can_frame;
}

void loop() {
    while (Serial2.available() > 0)
        if (gps.encode(Serial2.read())) {
            displayInfo();
            auto new_message = generate_gps_message(129025, gps.location.lat(), gps.location.lng());
            // const bool cansend_ok = 
            ACAN_T4::can1.tryToSend(new_message);
        }
        

    if (millis() > 5000 && gps.charsProcessed() < 10) {
        Serial.println(F("No GPS detected: check wiring."));
        while (true)
        ;
    }
}