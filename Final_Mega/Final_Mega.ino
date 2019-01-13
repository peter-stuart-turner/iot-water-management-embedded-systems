// Program written for arduino Mega
#include "MQTT_GC.h"                  //Adapted GSM_MQTT
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
#define S_BACKWASH        0x03         // Backwash the hair filter
#define S_CIRCULATE_G     0x04         // Circulate grey water
#define S_CIRCULATE_R     0x05         // Circulate rain water
#define S_DRAIN_G         0x06         // Drain Grey Tank
#define S_DRAIN_R         0x07         // Drain Rain Tank

// Definitions
#define MAX_SAMPLES       50           // Max number of samples before sending (less than 256)
#define PUB_INTERVAL      2000         // How often to send Tank levels in [ms]
#define READ_INTERVAL     40           // =PUB_INTERVAL*2/MAX_SAMPLES [ms]
#define B_MAX_LEVEL       30           // max level of biotank before pumping
#define BKWASH_TMOUT      10000        // time in [ms] to clear hair filters
#define R_CYC_TMOUT       60000        // Rain cycle time [ms]
#define G_CYC_TMOUT       30000        // Grey cycle time [ms]
#define CLIENT_MQTT       "Garth"
#define USER_MQTT         "pbocagsx"
#define PASSWORD_MQTT     "8pmpGdeQ2bcz"


// MQTT host address and port
String MQTT_HOST = "m21.cloudmqtt.com";
String MQTT_PORT = "11124";
char CLIENT_ID[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}; // Unique client ID

// Subscribe Topics
#define COMMAND           "GR/C"
#define SETTING           "GR/S"

// Publish Topics
#define G_TANK_LEVEL      "GR/GT"
#define R_TANK_LEVEL      "GR/RT"

//char SETTING[9] = {CLIENT_ID, '/', 'S',0};
//char MQTT_SETTING[8] = {CLIENT_ID, '/', 'M',0};  // To change MQTT_HOST and MQTT_PORT

// Sensor Values
unsigned long publishTime = 0;          // Store time of last publishing tank data
unsigned long readTime = 0;             // Store time of last publishing tank data
unsigned long backwashTime = 0;         // Store time of backwash for stopping
unsigned long cycleTime = 0;            // Store time of cycling water
byte samples = 0;                       // Number of samples
RunningAverage GreyAvg(MAX_SAMPLES);    // Object for measurement values of grey water
RunningAverage RainAvg(MAX_SAMPLES);    // Object for measurement values of rain water
RunningAverage BioAvg(MAX_SAMPLES);     // Object for measurement values of bio filter

//Global variables
byte state = 0;                         // Current state
byte B_TankHeight_70 = 17;              // Height in cm of bio tank (57*0.3) 70%
byte G_TankHeight_10 = 162;             // Height in cm of grey water tank (180*0.9) 10%
byte R_TankHeight_10 = 162;             // Height in cm of rain water tank (180*0.9) 10%
boolean irrigate_flag = 0;
boolean bio_empty_flag = 0;
boolean backwash_flag = 0;
boolean cycle_grey_flag = 0;
boolean cycle_rain_flag = 0;
boolean r_drain_flag = 0;
boolean g_drain_flag = 0;
boolean connect_flag = 0;
boolean tank_select = 0;                // select grey = 0  or rain = 1 alternates tanks for sending data
boolean allowMunicipal = 0;

//SoftwareSerial Serial(SM_TX, SM_RX); // Arduino RX, Arduino TX
MQTT_GC MQTT(0);
// parameter is the keepalive duration in seconds (0 turns the functionality off)

// ***********************************************************
//        SETUP
// ***********************************************************

void setup()
{
  // initialize mqtt:
  // GSM modem should be connected to SoftwareSerial
  MQTT.begin();
  Serial.println("Connecting");
  while (!connect_flag)
  {
    MQTT.processing();
    MQTT.serialEvent2();
    if (millis() > (publishTime + PUB_INTERVAL))
    {
      Serial.print(".");
      publishTime = millis();
    }
  }
  Serial.print("\n");
  Serial.println("###Started MQTT###");

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

  // State machine for grey water and rain water system
  switch (state) {
    case S_IDLE:
      // Check if it's time to send tank data else get tank data
      TankProcessing();
      //DRAINING LOGIC TO STOP DRAINING WHEN EMPTY
      if (r_drain_flag)
      {
        if (RainAvg.lastAvg > R_TankHeight_10)
        {
          digitalWrite(R_TANK_OUT, LOW);
          digitalWrite(DRAIN, LOW);
        }
      }
      if (g_drain_flag)
      {
        if (GreyAvg.lastAvg > G_TankHeight_10)
        {
          digitalWrite(G_TANK_OUT, LOW);
          digitalWrite(DRAIN, LOW);
        }
      }
      break;
    case S_IRRIGATE:
      // operate solenoids based on water levels to irrigate
      state = S_IDLE;
      if (irrigate_flag == 1)
      {
        StopIrrigate();
      }
      else
      {
        IrrigateSelect();
        irrigate_flag = 1;
      }
      break;
    case S_EMPTY_BIO_G:
      // Empty the Bio filter to grey water tank
      state = S_IDLE;
      if (bio_empty_flag)
      {
        digitalWrite(BIO_OUT, LOW);
        digitalWrite(G_TANK_IN, LOW);
        bio_empty_flag = 0;
      }
      else
      {
        digitalWrite(BIO_OUT, HIGH);
        digitalWrite(G_TANK_IN, HIGH);
        bio_empty_flag = 1;
      }
      break;
    case S_BACKWASH:
      // Backwash the hair filter
      state = S_IDLE;
      if (backwash_flag)
      {
        digitalWrite(BALL_B_WASH, LOW);
        digitalWrite(MUNI_B_WASH, LOW);
        backwash_flag = 0;
      }
      else
      {
        digitalWrite(BALL_B_WASH, HIGH);
        digitalWrite(MUNI_B_WASH, HIGH);
        backwash_flag = 1;
        backwashTime = millis() + BKWASH_TMOUT;
      }
      break;
    case S_CIRCULATE_G:
      // Circulate grey water
      state = S_IDLE;
      if (cycle_grey_flag)
      {
        digitalWrite(G_TANK_OUT, LOW);
        digitalWrite(G_TANK_IN, LOW);
        cycle_grey_flag = 0;
      }
      else
      {
        digitalWrite(G_TANK_OUT, HIGH);
        digitalWrite(G_TANK_IN, HIGH);
        cycle_grey_flag = 1;
        cycleTime = millis() + G_CYC_TMOUT;
      }
      break;
    case S_CIRCULATE_R:
      // Circulate rain water
      state = S_IDLE;
      if (cycle_rain_flag)
      {
        digitalWrite(R_TANK_OUT, LOW);
        digitalWrite(R_TANK_IN, LOW);
        cycle_rain_flag = 0;
      }
      else
      {
        digitalWrite(R_TANK_OUT, HIGH);
        digitalWrite(R_TANK_IN, HIGH);
        cycle_rain_flag = 1;
        cycleTime = millis() + R_CYC_TMOUT;
      }
      break;
    case S_DRAIN_G:
      state = S_IDLE;
      digitalWrite(G_TANK_OUT, HIGH);
      digitalWrite(DRAIN, HIGH);
      g_drain_flag = 1;
      break;
    case S_DRAIN_R:
      state = S_IDLE;
      digitalWrite(R_TANK_OUT, HIGH);
      digitalWrite(DRAIN, HIGH);
      r_drain_flag = 1;
      break;
    default:
      MQTT.publish(0, 0, 0, MQTT.generateMessageID(), COMMAND, "S_Err");
      break;
  }
  MQTT.processing();
  MQTT.serialEvent2();
}

// ***********************************************************
//        FUNCTIONS
// ***********************************************************
void TankProcessing(void)
{
  if (millis() >= publishTime)
  {
    if (MQTT.available())
    {
      if (tank_select == 0)
      {
        sendTankLevel(GreyAvg, 'G');
        tank_select = 1;
      }
      else
      {
        sendTankLevel(RainAvg, 'R');
        tank_select = 0;
      }
      publishTime = millis() + PUB_INTERVAL;
      samples = 0;
    }
    // Check biotank level and empty if full
    BioAvg.lastAvg = BioAvg.getAverage();
    if (!bio_empty_flag)
    {
      if (BioAvg.lastAvg <= B_MAX_LEVEL)
      {
        state = S_EMPTY_BIO_G;
      }
    }
    else
    {
      if (BioAvg.lastAvg > B_MAX_LEVEL)
      {
        state = S_EMPTY_BIO_G;
      }
    }
  }
  else if ((millis() > readTime) && (samples < MAX_SAMPLES))
  {
    // Get tank values
    samples++;
    RainAvg.addValue(GetTankLevel(R_SENS_TRIG, R_SENS_ECHO));
    GreyAvg.addValue(GetTankLevel(G_SENS_TRIG, G_SENS_ECHO));
    BioAvg.addValue(GetTankLevel(G_SENS_TRIG, G_SENS_ECHO));
    readTime = millis() + READ_INTERVAL;
  }
}

void sendTankLevel(RunningAverage &myRA, char subTopic)
{
  char tankLevel = myRA.getAverage();
  myRA.lastAvg = tankLevel;
  myRA.clr();
  if (tankLevel == 0)
  {
    tankLevel = 'A';
  }
  switch (subTopic)
  {
    case 'G':
      MQTT.publish(0, 0, 0, MQTT.generateMessageID(), G_TANK_LEVEL, &tankLevel);
      Serial.println("Sent tank level");
      break;
    case 'R':
      MQTT.publish(0, 0, 0, MQTT.generateMessageID(), R_TANK_LEVEL, &tankLevel);
      Serial.println("Sent tank level");
      break;
  }
}

byte GetTankLevel(int trigPin, int echoPin)
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
  }
  else
  {
    distance = 0;
  }
  return distance;
}

void IrrigateSelect(void)
{
  if ((GreyAvg.lastAvg <= G_TankHeight_10) && (RainAvg.lastAvg <= R_TankHeight_10)) // both more than 10% remaining
  {
    digitalWrite(G_TANK_OUT, HIGH);
    digitalWrite(R_TANK_OUT, HIGH);
    digitalWrite(GWRW_IRRIGATE, HIGH);
  }
  else if (GreyAvg.lastAvg <= G_TankHeight_10) // more than 10% remaining
  {
    digitalWrite(G_TANK_OUT, HIGH);
    digitalWrite(GWRW_IRRIGATE, HIGH);
  }
  else if (RainAvg.lastAvg <= R_TankHeight_10) // more than 10% remaining
  {
    digitalWrite(R_TANK_OUT, HIGH);
    digitalWrite(GWRW_IRRIGATE, HIGH);
  }
  else if (allowMunicipal) // am i allowed to use municipal water
  {
    digitalWrite(MUNI_IRRIGATE, HIGH);
    if (!irrigate_flag)// only on first entry
    {
      MQTT.publish(0, 0, 0, MQTT.generateMessageID(), COMMAND, "MI!");
    }
  }
  else
  {
    if (!irrigate_flag)// only on first entry
    {
      MQTT.publish(0, 0, 0, MQTT.generateMessageID(), COMMAND, "MI?");
    }
  }
}

void StopIrrigate(void)
{
  irrigate_flag = 0;
  digitalWrite(G_TANK_OUT, LOW);
  digitalWrite(R_TANK_OUT, LOW);
  digitalWrite(GWRW_IRRIGATE, LOW);
}

void MQTT_GC::AutoConnect(void)
{
  connect(CLIENT_MQTT, 1, 1, USER_MQTT, PASSWORD_MQTT, 1, 0, 0, 0, "", "");
  //   void connect(char *ClientIdentifier, char UserNameFlag, char PasswordFlag, char *UserName, char *Password, char CleanSession, char WillFlag, char WillQoS, char WillRetain, char *WillTopic, char *WillMessage);
  Serial.println("MQTT broker connect");
  Serial.println(MQTT.available());
  Serial.println(MQTT.TCP_Flag);
  MQTT.serialEvent2();
}

void MQTT_GC::OnConnect(void)
{
  //Serial.println("***************************");
  subscribe(0, generateMessageID(), COMMAND, 1);
  //    void subscribe(char DUP, unsigned int MessageID, char *SubTopic, char SubQoS);
  subscribe(0, generateMessageID(), SETTING, 1);
  publish(0, 0, 0, generateMessageID(), "GR/I", "S");
  //  void publish(char DUP, char Qos, char RETAIN, unsigned int MessageID, char *Topic, char *Message);
  Serial.println("***************************");
  if (!connect_flag)
  {
    connect_flag = 1;
  }
}

void MQTT_GC::OnMessage(char* Topic, int TopicLength, char* Message, int MessageLength)
{
  /*
    Topic        :Name of the topic from which message is coming
    TopicLength  :Number of characters in topic name
    Message      :The containing array
    MessageLength:Number of characters in message
  */
  Serial.println("###MESSAGE RECEIVED!###");
  if (Topic == COMMAND) {
    char cmd = Message[1];
    switch (cmd) {
      case 'i': // Irrigate
        state = S_IRRIGATE;
        Serial.println("###Irrigate###");
        break;
      case 'b':
        // Backwash
        state = S_BACKWASH;
        Serial.println("###Backwash###");
        break;
      case 'g':
        // cycle grey
        state = S_CIRCULATE_G;
        Serial.println("###Cycle Grey###");
        break;
      case 'r':
        // cycle rain
        state = S_CIRCULATE_R;
        Serial.println("###Cycle Rain###");
        break;
      case 's':
        // stop all
        Serial.println("###Stop All###");
        StopAll();
        state = S_IDLE;
        break;
      case 'd':
        state = S_DRAIN_G;
        Serial.println("###Drain grey tank###");
        break;
      case 'x':
        state = S_DRAIN_R;
        Serial.println("###Drain rain tank###");
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
  //  else if (Topic == MQTT_SETTING) {
  //
  //  }
  else { //error message just return sent message
    //MQTT.publish(0, 0, 0, MQTT.generateMessageID(), COMMAND, Message);
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
  if (r_drain_flag == 1)
  {
    r_drain_flag = 0;
    //stopDrain();
  }
    if (g_drain_flag == 1)
  {
    g_drain_flag = 0;
    //stopDrain();
  }
}


