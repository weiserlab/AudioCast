/* #############################################################################################
* Authors     : C. Rajashekar Reddy, Dhairya Shah
* Emails      : rajashekar.c@u.nus.edu, dhairya@u.nus.edu
################################################################################################
* Baseband Audio Generator for AudioCast Transmitter*

* Description : This sketch encodes sensor / fixed data in the form of audio signals which serves 
as a baseband signal for AudioCast transmitter. 
* Additional  : It builds on the ggwave library by ggerganov: https://github.com/ggerganov/ggwave/tree/master/examples/arduino-tx 
* Framework   : Platform IO
* Build Target: This sketch currently comples on ESP32-WROOM-32 (Devkit and NodeMCU), Check "platform.ini".
    - However, it works with other platformio compatible platforms like - Arduino Uno R3, Arduino Nano RP2040 Connect, etc.

* Receiver    : AudioCast uses a spectrum analyser to listen to the data encoded audio broadcasted by AudioCast.
                Further, the freely available "waver" app can be used to decode the data by listening to these transmissions
    - Web:     https://waver.ggerganov.com
    - Android: https://play.google.com/store/apps/details?id=com.ggerganov.Waver
    - iOS:     https://apps.apple.com/us/app/waver-data-over-sound/id1543607865
* Make sure to enable the "Fixed-length" option in "Waver"'s settings and set the number of bytes to 
be equal to "payloadLength" used in the sketch.
################################################################################################*/

#include <Arduino.h>
#include <ggwave.h>
#include <Wire.h>
#include <iostream>
#include <cstring>
#include <string>

// Uncomment the below if you want to transmit LUX data from DFRobot VEML7700 sensor. Additionally you may connect any other sensor and modify
// #define LUX_sensor_data 

String packet_data = "ABCDEF123456" ; //"ABCD"; //each character is 1 byte -> max can send is 15 bytes 16th byte is ending byte
char txt[64]; 
#ifdef LUX_sensor_data
    #include "DFRobot_VEML7700.h"
    DFRobot_VEML7700 als;  
#else
    // memset(txt, 0xff, sizeof(txt));
#endif

/*
 * Instantiate an object to drive the sensor
 */
// Pin configuration
const int kPinLed0    = 32;
const int kPinSpeaker = 33;

const int samplesPerFrame = 128;
const int sampleRate      = 6000;

// Global GGwave instance
GGWave ggwave;

#define P(str) (strcpy_P(txt, PSTR(str)), txt)

// Helper function to output the generated GGWave waveform via a buzzer
void send_text(GGWave & ggwave, uint8_t pin, const char * text, GGWave::TxProtocolId protocolId) {
    Serial.print(F("Sending text: "));
    Serial.println(text);

    ggwave.init(text, protocolId);
    ggwave.encode();

    const auto & protocol = GGWave::Protocols::tx()[protocolId];
    const auto tones = ggwave.txTones();
    const auto duration_ms = protocol.txDuration_ms(ggwave.samplesPerFrame(), ggwave.sampleRateOut());
    for (auto & curTone : tones) {
        const auto freq_hz = (protocol.freqStart + curTone)*ggwave.hzPerSample();
        tone(pin, freq_hz);
        delay(duration_ms);
    }

    noTone(pin);
    digitalWrite(pin, LOW);
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // pinMode(kPinLed0,    OUTPUT);
    pinMode(kPinSpeaker, OUTPUT);

    #ifdef LUX_sensor_data
        als.begin();   // Init
    #else
        // memset(txt, 0xff, sizeof(txt));
    #endif

    // Initialize "ggwave"
    {
        Serial.println(F("Trying to initialize the ggwave instance"));

        ggwave.setLogFile(nullptr);

        auto p = GGWave::getDefaultParameters();

        // Adjust the "ggwave" parameters to your needs.
        // Make sure that the "payloadLength" parameter matches the one used on the transmitting side.
        p.payloadLength   = 16;
        p.sampleRateInp   = sampleRate;
        p.sampleRateOut   = sampleRate;
        p.sampleRate      = sampleRate;
        p.samplesPerFrame = samplesPerFrame;
        p.sampleFormatInp = GGWAVE_SAMPLE_FORMAT_I16;
        p.sampleFormatOut = GGWAVE_SAMPLE_FORMAT_U8;
        p.operatingMode   = GGWAVE_OPERATING_MODE_TX | GGWAVE_OPERATING_MODE_TX_ONLY_TONES | GGWAVE_OPERATING_MODE_USE_DSS;

        // Protocols to use for TX
        GGWave::Protocols::tx().only(GGWAVE_PROTOCOL_MT_FASTEST);

        // Sometimes, the speaker might not be able to produce frequencies in the Mono-tone range of 1-2 kHz.
        // In such cases, you can shift the base frequency up by changing the "freqStart" parameter of the protocol.
        // Make sure that in the receiver (for example, the "Waver" app) the base frequency is shifted by the same amount.
        // Here we shift the frequency by +48 bins. Each bin is equal to 48000/1024 = 46.875 Hz.
        // So the base frequency is shifted by +2250 Hz.
        //GGWave::Protocols::tx()[GGWAVE_PROTOCOL_MT_FASTEST].freqStart += 48;

        // Initialize the ggwave instance and print the memory usage
        ggwave.prepare(p);
        Serial.println(ggwave.heapSize());

        Serial.println(F("Instance initialized successfully!"));
    }
}


void loop() {
    float lux;
    // delay(1000);

    send_text(ggwave, kPinSpeaker, P("Hello!"), GGWAVE_PROTOCOL_MT_FASTEST);

    delay(1000);

    Serial.println(F("Starting main loop"));
    int counter;
    String temp = "";
    // char* s = ""
    while (true) {
        counter = 1;
        // digitalWrite(kPinLed0, HIGH);
        #ifdef LUX_sensor_data
            als.getALSLux(lux);   // Get the measured ambient light value
            Serial.println(lux);
            snprintf(txt, 16, "Lux: %f lx", lux);
            send_text(ggwave, kPinSpeaker, txt, GGWAVE_PROTOCOL_MT_FASTEST);
            digitalWrite(kPinLed0, LOW);
            delay(1000);
        #else
            // Send 200 data packets
            for(counter = 1; counter <= 200; counter ++){
                temp = "";
                if(counter < 10) temp+="00";
                else if(counter < 100) temp +="0";
                temp += String(counter) + packet_data;
                // txt = &temp[0];
                // snprintf(txt, 16, temp);
                strcpy(txt, temp.c_str());
                send_text(ggwave, kPinSpeaker, txt, GGWAVE_PROTOCOL_MT_FASTEST);
                    delay(20);
            }
        #endif
    }  
}
