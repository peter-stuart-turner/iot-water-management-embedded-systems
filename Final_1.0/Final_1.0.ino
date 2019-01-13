#include "MQTT_GC.h"                  //Adapted GSM_MQTT
#include <SoftwareSerial.h>
#include "RunningAverage.h"

//Pin Assignments
// 1 and 2 reserved for serial
#define SM_TX             2 //10
#define SM_RX             3 //11
#define B_SENS_TRIG       4
#define B_SENS_ECHO       5
#define G_SENS_TRIG       6
#define G_SENS_ECHO       7
#define R_SENS_TRIG       8
#define R_SENS_ECHO       9
#define G_TANK_IN         10
#define G_TANK_OUT        11
#define R_TANK_IN         12
#define R_TANK_OUT        13
#define BIO_OUT           A0
#define BALL_B_WASH       A1
#define MUNI_B_WASH       A2
#define GWRW_IRRIGATE     A3
#define MUNI_IRRIGATE     A4
#define DRAIN             A5

// States
#define S_IDLE            0x00         // IDLE state used for receiving messages and sending sensor data
#define S_IRRIGATE        0x01         // Irrigation on
#define S_EMPTY_BIO_G     0x02         // Empty the bio tank to grey water tank
#define S_EMPTY_BIO_S     0x03         // Empty the bio tank to sewage
#define S_BACKWASH        0x04         // Backwash the hair filter
#define S_CIRCULATE_G     0x05         // Circulate grey water
#define S_CIRCULATE_R     0x06         // Circulate rain water
#define S_DRAIN_G         0x07         // Drain Grey Tank
#define S_DRAIN_R         0x08         // Drain Rain Tank

// Definitions
#define MAX_SAMPLES       50           // Max number of samples before sending (less than 256)
#define PUB_INTERVAL      2000         // How often to send Tank levels in [ms]
#define READ_INTERVAL     40           // =PUB_INTERVAL/MAX_SAMPLES [ms]
#define B_MAX_LEVEL       30           // max level of biotank before pumping

// MQTT host address and port
String MQTT_HOST = "m20.cloudmqtt.com";
String MQTT_PORT = "10775";
char CLIENT_ID[6] = {0, 0, 0, 0, 0, 0x01}; // Unique client ID

// Subscribe Topics
char COMMAND[8] = {CLIENT_ID, '/', 'C'};
char SETTING[8] = {CLIENT_ID, '/', 'S'};
char MQTT_SETTING[8] = {CLIENT_ID, '/', 'M'};  // To change MQTT_HOST and MQTT_PORT

// Publish Topics
char G_TANK[8] = {CLIENT_ID, '/', 'G'};
char R_TANK[8] = {CLIENT_ID, '/', 'R'};

// Sensor Values
unsigned long publishTime = 0;          // Store time of last publishing tank data
unsigned long readTime = 0;          // Store time of last publishing tank data
byte samples = 0;                       // Number of samples
RunningAverage GreyAvg(MAX_SAMPLES);    // Object for measurement values of grey water
RunningAverage RainAvg(MAX_SAMPLES);    // Object for measurement values of rain water
RunningAverage BioAvg(MAX_SAMPLES);     // Object for measurement values of bio filter

//Global variables
byte state = 0;                     // Current state
boolean irrigate_flag = 0;
boolean bio_empty_flag = 0;
boolean backwash_flag = 0;
boolean cycle_grey_flag = 0;
boolean cycle_rain_flag = 0;
boolean drain_flag = 0;

SoftwareSerial SoftSerial(SM_TX, SM_RX); // Arduino RX, Arduino TX
MQTT_GC MQTT(2000);
//   20 is the keepalive duration in seconds

// ***********************************************************
//        SETUP
// ***********************************************************

void setup()
{
  // initialize mqtt:
  // GSM modem should be connected to SoftwareSerial
  MQTT.begin();
  SoftSerial.println("Started MQTT");

  //Initialise pins
  pinMode(B_SENS_TRIG, OUTPUT);
  pinMode(B_SENS_ECHO, INPUT);
  pinMode(G_SENS_TRIG, OUTPUT);
  pinMode(G_SENS_ECHO, INPUT);
  pinMode(R_SENS_TRIG, OUTPUT);
  pinMode(R_SENS_ECHO, INPUT);
  pinMode(G_TANK_IN, OUTPUT);
  pinMode(G_TANK_OUT, OUTPUT);
  pinMode(R_TANK_IN, OUTPUT);
  pinMode(R_TANK_OUT, OUTPUT);
  pinMode(BIO_OUT, OUTPUT);
  pinMode(BALL_B_WASH, OUTPUT);
  pinMode(MUNI_B_WASH, OUTPUT);
  pinMode(GWRW_IRRIGATE, OUTPUT);
  pinMode(MUNI_IRRIGATE, OUTPUT);
  pinMode(DRAIN, OUTPUT);

  //Initialise buffers for tank levels
  GreyAvg.clr();
  RainAvg.clr();
  BioAvg.clr();

  //Initialise sensors
  digitalWrite(B_SENS_TRIG, LOW);
  digitalWrite(G_SENS_TRIG, LOW);
  digitalWrite(R_SENS_TRIG, LOW);
}

// ***********************************************************
//        MAIN
// ***********************************************************

void loop()
{
  
      if (MQTT.available())
      {
    
      }

      // State machine for grey water and rain water system
      switch (state) {
        case S_IDLE:
          // Check if the Gsm module is doing anything
          if (MQTT.available())
      {
    
      }
          // Check if it's time to send tank data else get tank data
          if (millis() > (publishTime + PUB_INTERVAL))
          {
            sendTankLevel(GreyAvg);
            sendTankLevel(RainAvg);
            publishTime = millis();
            samples = 0;
            SoftSerial.println("1");
          }
          else if ((millis() > (readTime + READ_INTERVAL)) && (samples < MAX_SAMPLES))
          {
            // Get tank values
            samples++;
            GetTankLevel(R_SENS_TRIG, R_SENS_ECHO, RainAvg);
            GetTankLevel(G_SENS_TRIG, G_SENS_ECHO, GreyAvg);
            GetTankLevel(B_SENS_TRIG, B_SENS_ECHO, BioAvg);
            readTime = millis();
          }
          // Check biotank level and empty if full
          if (BioAvg.lastAvg < B_MAX_LEVEL)
          {
    
          }
    
          break;
        case S_IRRIGATE:
          // operate solenoids based on water levels to irrigate
          digitalWrite(GWRW_IRRIGATE, HIGH);
          digitalWrite(MUNI_IRRIGATE, LOW);
          irrigate_flag = 1;
          state = S_IDLE;
          break;
        case S_EMPTY_BIO_G:
          // Empty the Bio filter to grey water tank
          digitalWrite(BIO_OUT, HIGH);
          digitalWrite(G_TANK_IN, HIGH);
          irrigate_flag = 1;
          state = S_IDLE;
          break;
        case S_EMPTY_BIO_S:
          // Empty the Bio filter to grey water tank
          break;
        case S_BACKWASH:
          // Backwash the hair filter
          break;
        case S_CIRCULATE_G:
          // Circulate grey water
          break;
        case S_CIRCULATE_R:
          // Circulate rain water
          break;
        default:
          MQTT.publish(0, 0, 0, MQTT.generateMessageID(), COMMAND, "S_Err");
          break;
      }
    MQTT.processing();
}

// ***********************************************************
//        FUNCTIONS
// ***********************************************************

void sendTankLevel(RunningAverage &myRA)
{
  byte tankLevel = myRA.getAverage();
  myRA.lastAvg = tankLevel;
  myRA.clr();
  MQTT.publish(0, 0, 0, MQTT.generateMessageID(), COMMAND, &tankLevel);
}

byte GetTankLevel(int trigPin, int echoPin, RunningAverage &myRA)
{
  byte distance = 0;
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);    //required 10 us
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  unsigned int duration = pulseIn(echoPin, HIGH, 32000);
  if (duration > 0)
  {
    distance = (byte)(duration * 0.0175); //output distance in cm
    myRA.addValue(distance);
  }
  else
  {
    distance = 0;
  }
  return distance;
}

void MQTT_GC::AutoConnect(void)
{
  connect("Garth", 1, 1, "rdjghvoh", "w7Ex0VTqZViw", 1, 0, 0, 0, "", "");
  //   void connect(char *ClientIdentifier, char UserNameFlag, char PasswordFlag, char *UserName, char *Password, char CleanSession, char WillFlag, char WillQoS, char WillRetain, char *WillTopic, char *WillMessage);
}

void MQTT_GC::OnConnect(void)
{
  SoftSerial.println("***************************");
  subscribe(0, generateMessageID(), "GR/C", 1);
  //    void subscribe(char DUP, unsigned int MessageID, char *SubTopic, char SubQoS);
  subscribe(0, generateMessageID(), SETTING, 1);
  publish(0, 0, 0, generateMessageID(), "GRStart", "HI");
  SoftSerial.println("***************************");
  //  void publish(char DUP, char Qos, char RETAIN, unsigned int MessageID, char *Topic, char *Message);
}

void MQTT_GC::OnMessage(char* Topic, int TopicLength, char* Message, int MessageLength)
{
  /*
     Topic        :Name of the topic from which message is coming
     TopicLength  :Number of characters in topic name
     Message      :The containing array
     MessageLength:Number of characters in message
  */
  if (Topic == COMMAND) {
    char cmd = Message[1];
    switch (cmd) {
      case 'i': // Irrigate
        if (irrigate_flag == 1) {
          //stopIrrigate();
        }
        else {
          // Check source RW/GW with flow diagram logic
          state = S_IRRIGATE;
          // Open irrigation solenoid
        }
        break;
      case 'b':
        // Backwash
        // Open ball valve
        //
        break;
      case 'g':
        // cycle grey

        break;
      case 'r':
        // cycle rain

        break;
      case 's':
        // stop all
        StopAll();
        break;
      case 'd':
        break;
      default:
        MQTT.publish(0, 0, 0, MQTT.generateMessageID(), COMMAND, Message);
        break;
    }
  }
  else if (Topic == SETTING) {
    char set = Message[1];
    switch (set)
    {
      case 'i':
        // Irrigation mode and time
        break;
      case 'b':
        // Backwash time
        break;
      case 'g':
        // grey tank specs
        break;
      case 'r':
        // rain tank specs
        break;
      case 's':
        // stop all
        break;
    }
  }
  else if (Topic == MQTT_SETTING) {

  }
  else { //error message just return sent message
    MQTT.publish(0, 0, 0, MQTT.generateMessageID(), COMMAND, Message);
  }
}

void StopAll(void)
{
  if (irrigate_flag == 1)
  {
    irrigate_flag = 0;
    //stopIrrigate();
  }
  if (backwash_flag == 1)
  {
    backwash_flag = 0;
    //stopBackwash();
  }
  if (cycle_grey_flag == 1)
  {
    cycle_grey_flag = 0;
    //stopCycleGrey();
  }
  if (cycle_rain_flag == 1)
  {
    cycle_rain_flag = 0;
    //stopCycleRain();
  }
  if (drain_flag == 1)
  {
    drain_flag = 0;
    //stopDrain();
  }
}


