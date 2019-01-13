#include <SoftwareSerial.h>

String Arsp, Grsp;
SoftwareSerial SIM800L(7, 8); // RX, TX

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  Serial.println("Testing GSM SIM800L");
  SIM800L.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:

  if(SIM800L.available())
  {
    Grsp = SIM800L.readString();
    Serial.println(Grsp);
  }

  if(Serial.available())
  {
    Arsp = Serial.readString();
    SIM800L.println(Arsp);
  }
}
