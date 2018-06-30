// This #include statement was automatically added by the Particle IDE.
#include <neopixel.h>

// This #include statement was automatically added by the Particle IDE.
#include <Ubidots.h>

STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));  // Use this line to enable the external WiFi Antenna
//STARTUP(WiFi.selectAntenna(ANT_INTERNAL));    // Use this line to enable the internal WiFi Antenna
STARTUP(System.enableFeature(FEATURE_RESET_INFO));  // Track why we reset
SYSTEM_THREAD(ENABLED);


/****************************************
 * Define Constants
 ****************************************/

#define TOKEN "0n0vYuJd8UmNCbnoBPtKhUz5hYScfM"  // Put here your Ubidots TOKEN
#define DATA_SOURCE_NAME "Victoria_Generator"

Ubidots ubidots(TOKEN);

//int led1 = D0; // Instead of writing D0 over and over again, we'll write led1
// You'll need to wire an LED to this one to see it blink.

int led2 = D7; // Instead of writing D7 over and over again, we'll write led2
// This one is the little blue LED on your board. On the Photon it is next to D7, and on the Core it is next to the USB jack.
unsigned long delayTransmit = 1000;     // minimum delay between data transmissions
unsigned long nextTransmit;             // minimum delay between data transmissions
unsigned long delayDatapoint = 10;      // time between regular data points
unsigned long delayTempReading = 600000; // take a temp reading ever 10 min
unsigned long nextTempReading;          // holds time for next temp reading
unsigned long nextDatapoint;            // timer for next data points
unsigned long delayBlinkOff = 300;      // how often to turn on the LED
unsigned long nextBlink;                // when to turn on the LED
unsigned long delayBlinkOn = 50;        // how long to turn on LED
unsigned long t = Time.now();           // calculates your actual timestamp in SECONDS
float VoltageLow;                   // holds the lowest measured voltage
float VoltageAvg;                   // moving average (10 samples at 100 Hz)
float VoltAvgLow;                   // lowest of averaged battery voltage samples
float newVoltage;
float voltage;                      // battery voltage reading
float temperature;                  // temperature reading

enum fsm_state {INITIALIZATION, IDLE, TRANSMIT, ALERT, RUNNING, INTERVAL, GEN_START, SEND_BUFF};
fsm_state fsm_state = INITIALIZATION;

enum LED_state {LED_on, LED_off, LED_disabled};
LED_state LED_state = LED_disabled;

String deviceID = "4d004e000c51353432383931";            // Unique to your Photon

char str [80];
Timer timer_10Hz(1000, Measure_10Hz);       // set up 10Hz timer

void setup() {
    Serial.begin(115200);
    //ubidots.setDebug(true); //Uncomment this line for printing debug messages
    //  pinMode(led1, OUTPUT);
    pinMode(led2, OUTPUT);
 //   timer_10Hz.start();     // start 10 Hz timer
}

void loop() {
    switch (fsm_state)
    {
        case INITIALIZATION:
            Particle.publish("State","Entered INIT");
            nextTransmit = delayTransmit + millis();   // wait delayTransmit before next transmit
            nextBlink = delayBlinkOff + millis();      // wait delayBlinkOff between LED blinks
            Particle.publish("State","Exit Init -> Idle");
            fsm_state = IDLE;
        break;
        
        case IDLE:
            //Particle.publish("State","Entered IDLE");
            //temperature = analogRead(A0);
            //temperature = (((temperature*3.3)/4095) *100) - 1;  // 10mV/deg F to XX.X F
            //temperature = int ((temperature*10) + 5);    // round to one decimal place
            //temperature = temperature/10;    // round to one decimal place
            // temperature = int (temperature +0.5);
            voltage = analogRead(A1);  // read Battery Voltage
            voltage = ((voltage*3.3)/4095) *5.77;  // Convert counts to Battery Voltage (1k/220 ohms)
            voltage = (round(voltage*100))/100;    // round to two decimal places
            // int value3 = analogRead(A1);
            // value3 = ((value3*3.3)/41) *5.77;  // Convert counts to Battery Voltage (1k/220 ohms)
            // float value2 = (round(value3))/100;    // round to two ne decimal places
            // float value3 = analogRead(A2);
            if (millis() > nextDatapoint)                       // time to take another reading
            {
                newVoltage = analogRead(A1);                    // read Battery Voltage
                newVoltage = ((newVoltage*3.3)/4095) *5.77;     // Convert counts to Battery Voltage (1k/220 ohms)
                if (VoltageAvg == 0) VoltageAvg = newVoltage;   // initialize Avg at newVoltage
                VoltageAvg = (((VoltageAvg*9) + newVoltage) + 0.01)/10;  // calculate moving average
                VoltageAvg = (round(VoltageAvg*100))/100;       // round to two decimal places
                if (VoltAvgLow == 0) VoltAvgLow = VoltageAvg;    // initalize after transmit
                if (VoltageAvg < VoltAvgLow) VoltAvgLow = VoltageAvg;   // keep the lowest averaged value
                nextDatapoint = millis() + delayDatapoint;      // wait for next data point
            }
            
            if (VoltageLow > 0)
            {
                if (voltage < VoltageLow) VoltageLow = voltage;     // keep the lowest voltage reading
            }
            
            if (millis() > nextTempReading)     // take a temperture reading
            {
                temperature = analogRead(A0);
                temperature = (((temperature*3.3)/4095) *100) - 1;  // 10mV/deg F to XX.X F
                temperature = int ((temperature*10) + 5);    // round to one decimal place
                temperature = temperature/10;    // round to one decimal place
                // temperature = int (temperature +0.5);
                t = Time.now(); // calculates your actual timestamp in SECONDS
                ubidots.add("BatteryTemp", temperature, NULL, t-0000);  // timestamp now
                nextTempReading = millis() + delayTempReading;  // set next temp reading
            }
            
            if (millis() > nextTransmit) fsm_state = TRANSMIT;
        break;
        
        case TRANSMIT:
            Particle.publish("State","Entered TRANSMIT");
            t = Time.now(); // calculates your actual timestamp in SECONDS
            //sprintf(str, "i", temperature);
            //round (temperature);
            //ubidots.add("BatteryTemp", temperature);  // Change for your variable name
            // ubidots.add("Voltage", 1800, NULL, t-5000);  // Sends a value with a custom timestamp
            // ubidots.add("Voltage", 1400, "", t-1000);  // Sends a value with a custom timestamp
            // ubidots.add("Voltage", 1300, NULL, t-0000);  // Sends a value with a custom timestamp
            //voltage = VoltageLow;      // set the votlage to transmitt to the lowest voltage during the period
            ubidots.add("BatteryVoltage", voltage);  // Add Battery Voltage to the packet
            ubidots.add("LowVoltage", VoltageLow);  // Add Battery Voltage to the packet
            ubidots.add("LowAvgVolt", VoltAvgLow);  // Add Battery Voltage to the packet
            ubidots.add("AverageVoltage", VoltageAvg);  // Send average voltage
            // ubidots.add("Current", value3);
            // Sends variables 'test-1' and 'test-2' with your actual timestamp,
            // variable 'test-2' will be send with its custom timestamp
            ubidots.sendAll(t);
            nextTransmit = millis() + delayTransmit;
            VoltAvgLow = 16;        // reset averaged low voltage
            VoltageLow = 16;        // reset low voltage
            Particle.publish("State","Exit Transmit -> Idle");
            fsm_state = IDLE;
        break;
        
        case INTERVAL:  // got her from an inteval timer interupt
            
            // check buffer to see if there is a datapoint newer than the current time
            // skip the Inteval datapoint if there is a newer datapoing
            // means the system overwrote this timeslot during a starting event
            // got back to sleep and wait for next interval
            // wait for time = 2 seconds before 10 minute point
            // collect 10 datapoints at 10 Hz and average them 
            // time stamp data points on the 10 minute window and write to buffer.
        break;
        
        case GEN_START: // got her because of a voltage change interupt
            // Record data at 100 Hz and aveage 10 points
            // Timestamp first datapoint at current time and add to buffer
            // Record new datapoint ever 0.1 sec until deltaV < 0.1 V 
        break;
        
        case SEND_BUFF: // run as seperate state machine that runs when awake
        break;
    }
    
    switch (LED_state)  // State Machine for onboard Blue LED
    {
        case (LED_off):
            if (millis() > nextBlink) 
            {
                digitalWrite(led2, HIGH);       // turn on the onboard LED
                nextBlink = delayBlinkOn + millis();
                LED_state = LED_on;
            }
            break;
            
        case (LED_on):
            if (millis() > nextBlink)
            {
                digitalWrite(led2, LOW);
                nextBlink = delayBlinkOff + millis();
                LED_state = LED_off;
            }
            break;
    }
}

void Measure_10Hz() // collect data at 10 Hz for 1 sec 
{   
     static int count = 0;
    //count = count++;
    digitalWrite(led2, !digitalRead(led2));   // Toggle led
    Serial.println(count++);
}
    

