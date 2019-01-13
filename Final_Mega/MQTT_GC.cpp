// *********************************************************************************
// GSM_MQTT.cpp adjusted for application in Greenchain engineering MQTT applications
// *********************************************************************************

#include "MQTT_GC.h"
#include "Arduino.h"
#include <avr/pgmspace.h>
//extern uint8_t GSM_Response;
extern String MQTT_HOST;
extern String MQTT_PORT;
extern MQTT_GC MQTT;

byte GSM_Response = 0;
unsigned long previousMillis = 0;
//char inputString[UART_BUFFER_LENGTH];         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
void serialEvent2();

MQTT_GC::MQTT_GC(unsigned long KeepAlive)
{
  _KeepAliveTimeOut = KeepAlive;
}

void MQTT_GC::begin(void)
{
  Serial.begin(38400);
  Serial2.begin(9600);
  Serial2.write("AT\r\n");
  delay(1000);
  _tcpInit();
}

char MQTT_GC::_sendAT(char *command, unsigned long waitms)
{

  unsigned long PrevMillis = millis();
  strcpy(reply, "none");
  GSM_Response = 0;
  Serial2.write(command);
  unsigned long currentMillis = millis();
  //  Serial.println(PrevMillis);
  //  Serial.println(currentMillis);
  while ( (GSM_Response == 0) && ((currentMillis - PrevMillis) < waitms) )
  {
    //    delay(1);
    serialEvent2();
    currentMillis = millis();
  }
  return GSM_Response;
}
char MQTT_GC::sendATreply(char *command, char *replystr, unsigned long waitms)
{
  strcpy(reply, replystr);
  unsigned long PrevMillis = millis();
  GSM_ReplyFlag = 0;
  Serial2.write(command);
  unsigned long currentMillis = millis();

  //  Serial.println(PrevMillis);
  //  Serial.println(currentMillis);
  while ( (GSM_ReplyFlag == 0) && ((currentMillis - PrevMillis) < waitms) )
  {
    //    delay(1);
    serialEvent2();
    currentMillis = millis();
  }
  return GSM_ReplyFlag;
}
void MQTT_GC::_tcpInit(void)
{
  switch (modemStatus)
  {
    case 0:
      {
        delay(1000);
        Serial2.print("+++");
        delay(500);
        if (_sendAT("AT\r\n", 5000) == 1)
        {
          modemStatus = 1;
        }
        else
        {
          modemStatus = 0;
          break;
        }
      }
    case 1:
      {
        if (_sendAT("ATE1\r\n", 2000) == 1)
        {
          modemStatus = 2;
        }
        else
        {
          modemStatus = 1;
          break;
        }
      }
    case 2:
      {
        if (sendATreply("AT+CREG?\r\n", "0,1", 5000) == 1)
        {
          _sendAT("AT+CIPMUX=0\r\n", 2000);
          _sendAT("AT+CIPMODE=1\r\n", 2000);
          if (sendATreply("AT+CGATT?\r\n", ": 1", 4000) != 1)
          {
            _sendAT("AT+CGATT=1\r\n", 2000);
          }
          modemStatus = 3;
          _tcpStatus = 2;
        }
        else
        {
          modemStatus = 2;
          break;
        }
      }
    case 3:
      {
        if (GSM_ReplyFlag != 7)
        {
          _tcpStatus = sendATreply("AT+CIPSTATUS\r\n", "STATE", 4000);
          if (_tcpStatusPrev == _tcpStatus)
          {
            tcpATerrorcount++;
            if (tcpATerrorcount >= 10)
            {
              tcpATerrorcount = 0;
              _tcpStatus = 7;
            }

          }
          else
          {
            _tcpStatusPrev = _tcpStatus;
            tcpATerrorcount = 0;
          }
        }
        _tcpStatusPrev = _tcpStatus;
        Serial.print(_tcpStatus);
        switch (_tcpStatus)
        {
          case 2:
            {
              _sendAT("AT+CSTT=\"internet\"\r\n", 5000);
              break;
            }
          case 3:
            {
              _sendAT("AT+CIICR\r\n", 5000)  ;
              break;
            }
          case 4:
            {
              sendATreply("AT+CIFSR\r\n", ".", 4000) ;
              break;
            }
          case 5:
            {
              Serial2.print("AT+CIPSTART=\"TCP\",\"");
              Serial2.print(MQTT_HOST);
              Serial2.print("\",\"");
              Serial2.print(MQTT_PORT);
              if (_sendAT("\"\r\n", 5000) == 1)
              {
                unsigned long PrevMillis = millis();
                unsigned long currentMillis = millis();
                while ( (GSM_Response != 4) && ((currentMillis - PrevMillis) < 20000) )
                {
                  //    delay(1);
                  serialEvent2();
                  currentMillis = millis();
                }
              }
              break;
            }
          case 6:
            {
              unsigned long PrevMillis = millis();
              unsigned long currentMillis = millis();
              while ( (GSM_Response != 4) && ((currentMillis - PrevMillis) < 20000) )
              {
                //    delay(1);
                serialEvent2();
                currentMillis = millis();
              }
              break;
            }
          case 7:
            {
              sendATreply("AT+CIPSHUT\r\n", "OK", 4000) ;
              modemStatus = 0;
              _tcpStatus = 2;
              break;
            }
        }
      }
  }

}

void MQTT_GC::_ping(void)
{
  if (pingFlag == true)
  {

    unsigned long currentMillis = millis();
    if (_KeepAliveTimeOut == 0)
    {
    }
    else if ((currentMillis - _PingPrevMillis ) >= _KeepAliveTimeOut * 1000)
    {
      // save the last time you blinked the LED
      _PingPrevMillis = currentMillis;
      Serial2.print(char(PINGREQ * 16));
      _sendLength(0);
    }
  }
}

void MQTT_GC::_sendUTFString(char *string)
{
  int localLength = strlen(string);
  Serial2.print(char(localLength / 256));
  Serial2.print(char(localLength % 256));
  Serial2.print(string);
}

void MQTT_GC::_sendLength(int len)
{
  bool  length_flag = false;
  while (length_flag == false)
  {
    if ((len / 128) > 0)
    {
      Serial2.print(char(len % 128 + 128));
      len /= 128;
    }
    else
    {
      length_flag = true;
      Serial2.print(char(len));
    }
  }
}
void MQTT_GC::connect(char *ClientIdentifier, char UserNameFlag, char PasswordFlag, char *UserName, char *Password, char CleanSession, char WillFlag, char WillQoS, char WillRetain, char *WillTopic, char *WillMessage)
{
  ConnectionAcknowledgement = NO_ACKNOWLEDGEMENT ;
  Serial2.print(char(CONNECT * 16 ));
  char ProtocolName[7] = "MQIsdp";
  int localLength = (2 + strlen(ProtocolName)) + 1 + 3 + (2 + strlen(ClientIdentifier));
  if (WillFlag != 0)
  {
    localLength = localLength + 2 + strlen(WillTopic) + 2 + strlen(WillMessage);
  }
  if (UserNameFlag != 0)
  {
    localLength = localLength + 2 + strlen(UserName);

    if (PasswordFlag != 0)
    {
      localLength = localLength + 2 + strlen(Password);
    }
  }
  _sendLength(localLength);
  _sendUTFString(ProtocolName);
  Serial2.print(char(_ProtocolVersion));
  Serial2.print(char(UserNameFlag * User_Name_Flag_Mask + PasswordFlag * Password_Flag_Mask + WillRetain * Will_Retain_Mask + WillQoS * Will_QoS_Scale + WillFlag * Will_Flag_Mask + CleanSession * Clean_Session_Mask));
  Serial2.print(char(_KeepAliveTimeOut / 256));
  Serial2.print(char(_KeepAliveTimeOut % 256));
  _sendUTFString(ClientIdentifier);
  if (WillFlag != 0)
  {
    _sendUTFString(WillTopic);
    _sendUTFString(WillMessage);
  }
  if (UserNameFlag != 0)
  {
    _sendUTFString(UserName);
    if (PasswordFlag != 0)
    {
      _sendUTFString(Password);
    }
  }
}
void MQTT_GC::publish(char DUP, char Qos, char RETAIN, unsigned int MessageID, char *Topic, char *Message)
{
  Serial2.print(char(PUBLISH * 16 + DUP * DUP_Mask + Qos * QoS_Scale + RETAIN));
  int localLength = (2 + strlen(Topic));
  if (Qos > 0)
  {
    localLength += 2;
  }
  localLength += strlen(Message);
  _sendLength(localLength);
  _sendUTFString(Topic);
  if (Qos > 0)
  {
    Serial2.print(char(MessageID / 256));
    Serial2.print(char(MessageID % 256));
  }
  Serial2.print(Message);
}
void MQTT_GC::publishACK(unsigned int MessageID)
{
  Serial2.print(char(PUBACK * 16));
  _sendLength(2);
  Serial2.print(char(MessageID / 256));
  Serial2.print(char(MessageID % 256));
}
void MQTT_GC::publishREC(unsigned int MessageID)
{
  Serial2.print(char(PUBREC * 16));
  _sendLength(2);
  Serial2.print(char(MessageID / 256));
  Serial2.print(char(MessageID % 256));
}
void MQTT_GC::publishREL(char DUP, unsigned int MessageID)
{
  Serial2.print(char(PUBREL * 16 + DUP * DUP_Mask + 1 * QoS_Scale));
  _sendLength(2);
  Serial2.print(char(MessageID / 256));
  Serial2.print(char(MessageID % 256));
}

void MQTT_GC::publishCOMP(unsigned int MessageID)
{
  Serial2.print(char(PUBCOMP * 16));
  _sendLength(2);
  Serial2.print(char(MessageID / 256));
  Serial2.print(char(MessageID % 256));
}
void MQTT_GC::subscribe(char DUP, unsigned int MessageID, char *SubTopic, char SubQoS)
{
  Serial2.print(char(SUBSCRIBE * 16 + DUP * DUP_Mask + 1 * QoS_Scale));
  int localLength = 2 + (2 + strlen(SubTopic)) + 1;
  _sendLength(localLength);
  Serial2.print(char(MessageID / 256));
  Serial2.print(char(MessageID % 256));
  _sendUTFString(SubTopic);
  Serial2.print(SubQoS);

}
void MQTT_GC::unsubscribe(char DUP, unsigned int MessageID, char *SubTopic)
{
  Serial2.print(char(UNSUBSCRIBE * 16 + DUP * DUP_Mask + 1 * QoS_Scale));
  int localLength = (2 + strlen(SubTopic)) + 2;
  _sendLength(localLength);

  Serial2.print(char(MessageID / 256));
  Serial2.print(char(MessageID % 256));

  _sendUTFString(SubTopic);
}
void MQTT_GC::disconnect(void)
{
  Serial2.print(char(DISCONNECT * 16));
  _sendLength(0);
  pingFlag = false;
}
//Messages
const char CONNECTMessage[] PROGMEM  = {"Client request to connect to Server\r\n"};
const char CONNACKMessage[] PROGMEM  = {"Connect Acknowledgment\r\n"};
const char PUBLISHMessage[] PROGMEM  = {"Publish message\r\n"};
const char PUBACKMessage[] PROGMEM  = {"Publish Acknowledgment\r\n"};
const char PUBRECMessage[] PROGMEM  = {"Publish Received (assured delivery part 1)\r\n"};
const char PUBRELMessage[] PROGMEM  = {"Publish Release (assured delivery part 2)\r\n"};
const char PUBCOMPMessage[] PROGMEM  = {"Publish Complete (assured delivery part 3)\r\n"};
const char SUBSCRIBEMessage[] PROGMEM  = {"Client Subscribe request\r\n"};
const char SUBACKMessage[] PROGMEM  = {"Subscribe Acknowledgment\r\n"};
const char UNSUBSCRIBEMessage[] PROGMEM  = {"Client Unsubscribe request\r\n"};
const char UNSUBACKMessage[] PROGMEM  = {"Unsubscribe Acknowledgment\r\n"};
const char PINGREQMessage[] PROGMEM  = {"PING Request\r\n"};
const char PINGRESPMessage[] PROGMEM  = {"PING Response\r\n"};
const char DISCONNECTMessage[] PROGMEM  = {"Client is Disconnecting\r\n"};

void MQTT_GC::printMessageType(uint8_t Message)
{
  switch (Message)
  {
    case CONNECT:
      {
        int k, len = strlen_P(CONNECTMessage);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(CONNECTMessage + k);
          Serial.print(myChar);
        }
        break;
      }
    case CONNACK:
      {
        int k, len = strlen_P(CONNACKMessage);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(CONNACKMessage + k);
          Serial.print(myChar);
        }
        break;
      }
    case PUBLISH:
      {
        int k, len = strlen_P(PUBLISHMessage);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(PUBLISHMessage + k);
          Serial.print(myChar);
        }
        break;
      }
    case PUBACK:
      {
        int k, len = strlen_P(PUBACKMessage);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(PUBACKMessage + k);
          Serial.print(myChar);
        }
        break;
      }
    case  PUBREC:
      {
        int k, len = strlen_P(PUBRECMessage);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(PUBRECMessage + k);
          Serial.print(myChar);
        }
        break;
      }
    case PUBREL:
      {
        int k, len = strlen_P(PUBRELMessage);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(PUBRELMessage + k);
          Serial.print(myChar);
        }
        break;
      }
    case PUBCOMP:
      {
        int k, len = strlen_P(PUBCOMPMessage );
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(PUBCOMPMessage  + k);
          Serial.print(myChar);
        }
        break;
      }
    case SUBSCRIBE:
      {
        int k, len = strlen_P(SUBSCRIBEMessage );
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(SUBSCRIBEMessage  + k);
          Serial.print(myChar);
        }
        break;
      }
    case SUBACK:
      {
        int k, len = strlen_P(SUBACKMessage );
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(SUBACKMessage  + k);
          Serial.print(myChar);
        }
        break;
      }
    case UNSUBSCRIBE:
      {
        int k, len = strlen_P(UNSUBSCRIBEMessage );
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(UNSUBSCRIBEMessage  + k);
          Serial.print(myChar);
        }
        break;
      }
    case UNSUBACK:
      {
        int k, len = strlen_P(UNSUBACKMessage );
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(UNSUBACKMessage  + k);
          Serial.print(myChar);
        }
        break;
      }
    case PINGREQ:
      {
        int k, len = strlen_P(PINGREQMessage);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(PINGREQMessage + k);
          Serial.print(myChar);
        }
        break;
      }
    case PINGRESP:
      {
        int k, len = strlen_P(PINGRESPMessage);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(PINGRESPMessage + k);
          Serial.print(myChar);
        }
        break;
      }
    case DISCONNECT:
      {
        int k, len = strlen_P(DISCONNECTMessage);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(DISCONNECTMessage + k);
          Serial.print(myChar);
        }
        break;
      }
  }
}

//Connect Ack
const char ConnectAck0[] PROGMEM  = {"Connection Accepted\r\n"};
const char ConnectAck1[] PROGMEM  = {"Connection Refused: unacceptable protocol version\r\n"};
const char ConnectAck2[] PROGMEM  = {"Connection Refused: identifier rejected\r\n"};
const char ConnectAck3[] PROGMEM  = {"Connection Refused: server unavailable\r\n"};
const char ConnectAck4[] PROGMEM  = {"Connection Refused: bad user name or password\r\n"};
const char ConnectAck5[] PROGMEM  = {"Connection Refused: not authorized\r\n"};
void MQTT_GC::printConnectAck(uint8_t Ack)
{
  switch (Ack)
  {
    case 0:
      {
        int k, len = strlen_P(ConnectAck0);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(ConnectAck0 + k);
          Serial.print(myChar);
        }
        break;
      }
    case 1:
      {
        int k, len = strlen_P(ConnectAck1);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(ConnectAck1 + k);
          Serial.print(myChar);
        }
        break;
      }
    case 2:
      {
        int k, len = strlen_P(ConnectAck2);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(ConnectAck2 + k);
          Serial.print(myChar);
        }
        break;
      }
    case 3:
      {
        int k, len = strlen_P(ConnectAck3);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(ConnectAck3 + k);
          Serial.print(myChar);
        }
        break;
      }
    case 4:
      {
        int k, len = strlen_P(ConnectAck4);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(ConnectAck4 + k);
          Serial.print(myChar);
        }
        break;
      }
    case 5:
      {
        int k, len = strlen_P(ConnectAck5);
        char myChar;
        for (k = 0; k < len; k++)
        {
          myChar =  pgm_read_byte_near(ConnectAck5 + k);
          Serial.print(myChar);
        }
        break;
      }
  }
}

unsigned int MQTT_GC::generateMessageID(void)
{
  if (_LastMessaseID < 65535)
  {
    return ++_LastMessaseID;
  }
  else
  {
    _LastMessaseID = 0;
    return _LastMessaseID;
  }
}

void MQTT_GC::processing(void)
{
  if (TCP_Flag == false)
  {
    MQTT_Flag = false;
    _tcpInit();
  }
  _ping();
}

bool MQTT_GC::available(void)
{
  return MQTT_Flag;
}

void MQTT_GC::serialEvent2(void)
{


  while (Serial2.available())
  {
    if (strstr(MQTT.inputString, " CONNECT OK"))
      Serial.println(" CONNECT OK");

    char inChar = (char)Serial2.read();
    if (MQTT.TCP_Flag == false)
    {
      if (MQTT.index < 200)
      {
        MQTT.inputString[MQTT.index++] = inChar;
      }
      //Serial.println("debug_serialEvent2");
      if (inChar == '\n')
      {
        MQTT.inputString[MQTT.index] = 0;
        stringComplete = true;
        if (strstr(MQTT.inputString, MQTT.reply) != NULL)
        {
          MQTT.GSM_ReplyFlag = 1;
          if (strstr(MQTT.inputString, " INITIAL") != 0)
          {
            MQTT.GSM_ReplyFlag = 2; //
          }
          else if (strstr(MQTT.inputString, " START") != 0)
          {
            MQTT.GSM_ReplyFlag = 3; //
          }
          else if (strstr(MQTT.inputString, " IP CONFIG") != 0)
          {
            _delay_us(10);
            MQTT.GSM_ReplyFlag = 4;
          }
          else if (strstr(MQTT.inputString, " GPRSACT") != 0)
          {
            MQTT.GSM_ReplyFlag = 4; //
          }
          else if ((strstr(MQTT.inputString, " STATUS") != 0) || (strstr(MQTT.inputString, "TCP CLOSED") != 0))
          {
            MQTT.GSM_ReplyFlag = 5; //
          }
          else if (strstr(MQTT.inputString, " TCP CONNECTING") != 0)
          {
            MQTT.GSM_ReplyFlag = 6; //
          }
          else if ((strstr(MQTT.inputString, " CONNECT OK") != 0) || (strstr(MQTT.inputString, " CONNECT FAIL") != NULL) || (strstr(MQTT.inputString, " PDP DEACT") != 0))
          {
            MQTT.GSM_ReplyFlag = 7;
          }
          Serial.print("MQTT.inputString: ");
          Serial.println(MQTT.inputString);
          Serial.print("MQTT.ireply flagg: ");
          Serial.print(MQTT.GSM_ReplyFlag, DEC);
          Serial.println(" ");
        }
        else if (strstr(MQTT.inputString, "OK") != NULL)
        {
          GSM_Response = 1;
        }
        else if (strstr(MQTT.inputString, "ERROR") != NULL)
        {
          GSM_Response = 2;
        }
        else if (strstr(MQTT.inputString, ".") != NULL)
        {
          GSM_Response = 3;
        }
        else if (strstr(MQTT.inputString, "CONNECT FAIL") != NULL)
        {
          GSM_Response = 5;
        }
        else if (strstr(MQTT.inputString, "CONNECT") != NULL)
        {
          GSM_Response = 4;
          MQTT.TCP_Flag = true;
          Serial.println("MQTT.TCP_Flag = True");
          MQTT.AutoConnect();
          MQTT.pingFlag = true;
          Serial.println("MQTT.Ping_Flag = True");
          MQTT.tcpATerrorcount = 0;
        }
        else if (strstr(MQTT.inputString, "CLOSED") != NULL)
        {
          GSM_Response = 4;
          MQTT.TCP_Flag = false;
          MQTT.MQTT_Flag = false;
          Serial.println("MQTT.MQTT_Flag = false;");
        }
        //        Serial.print("GSM_Response: ");
        //        Serial.print(GSM_Response);
        MQTT.index = 0;
        MQTT.inputString[0] = 0;
      }
    }
    else
    {
      uint8_t ReceivedMessageType = (inChar / 16) & 0x0F;
      uint8_t DUP = (inChar & DUP_Mask) / DUP_Mask;
      uint8_t QoS = (inChar & QoS_Mask) / QoS_Scale;
      uint8_t RETAIN = (inChar & RETAIN_Mask);
      if ((ReceivedMessageType >= CONNECT) && (ReceivedMessageType <= DISCONNECT))
      {
        bool NextLengthByte = true;
        MQTT.length = 0;
        MQTT.lengthLocal = 0;
        uint32_t multiplier = 1;
        delay(2);
        char Cchar = inChar;
        while ( (NextLengthByte == true) && (MQTT.TCP_Flag == true))
        {
          if (Serial2.available())
          {
            inChar = (char)Serial2.read();
            //            Serial.print("InChar: ");
            //            Serial.println(Cchar, DEC);
            //            Serial.println(inChar, DEC);
            if ((((Cchar & 0xFF) == 'C') && ((inChar & 0xFF) == 'L') && (MQTT.length == 0)) || (((Cchar & 0xFF) == '+') && ((inChar & 0xFF) == 'P') && (MQTT.length == 0)))
            {
              MQTT.index = 0;
              MQTT.inputString[MQTT.index++] = Cchar;
              MQTT.inputString[MQTT.index++] = inChar;
              MQTT.TCP_Flag = false;
              MQTT.MQTT_Flag = false;
              MQTT.pingFlag = false;
              Serial.println("Disconnecting");
            }
            else
            {
              if ((inChar & 128) == 128)
              {
                MQTT.length += (inChar & 127) *  multiplier;
                multiplier *= 128;
                Serial.println("More");
              }
              else
              {
                NextLengthByte = false;
                MQTT.length += (inChar & 127) *  multiplier;
                multiplier *= 128;
              }
            }
          }
        }
        MQTT.lengthLocal = MQTT.length;
        if (MQTT.TCP_Flag == true)
        {

          MQTT.printMessageType(ReceivedMessageType);
          MQTT.index = 0L;
          uint32_t a = 0;
          while ((MQTT.length-- > 0) && (Serial2.available()))
          {
            MQTT.inputString[uint32_t(MQTT.index++)] = (char)Serial2.read();

            delay(1);

          }
//          Serial.print("received message type: ");
//          Serial.println(ReceivedMessageType, DEC);
          if (ReceivedMessageType == CONNACK)
          {
            Serial.println("connack");
            MQTT.ConnectionAcknowledgement = MQTT.inputString[0] * 256 + MQTT.inputString[1];
            if (MQTT.ConnectionAcknowledgement == 0)
            {

              MQTT.MQTT_Flag = true;
              MQTT.OnConnect();

            }

            MQTT.printConnectAck(MQTT.ConnectionAcknowledgement);
            // MQTT.OnConnect();
          }
          else if (ReceivedMessageType == PUBLISH)
          {
            uint32_t TopicLength = (MQTT.inputString[0]) * 256 + (MQTT.inputString[1]);
            Serial.print("Topic : '");
            MQTT.PublishIndex = 0;
            for (uint32_t iter = 2; iter < TopicLength + 2; iter++)
            {
              Serial.print(MQTT.inputString[iter]);
              MQTT.Topic[MQTT.PublishIndex++] = MQTT.inputString[iter];
            }
            MQTT.Topic[MQTT.PublishIndex] = 0;
            Serial.print("' Message :'");
            MQTT.TopicLength = MQTT.PublishIndex;

            MQTT.PublishIndex = 0;
            uint32_t MessageSTART = TopicLength + 2UL;
            int MessageID = 0;
            if (QoS != 0)
            {
              MessageSTART += 2;
              MessageID = MQTT.inputString[TopicLength + 2UL] * 256 + MQTT.inputString[TopicLength + 3UL];
            }
            for (uint32_t iter = (MessageSTART); iter < (MQTT.lengthLocal); iter++)
            {
              Serial.print(MQTT.inputString[iter]);
              MQTT.Message[MQTT.PublishIndex++] = MQTT.inputString[iter];
            }
            MQTT.Message[MQTT.PublishIndex] = 0;
            Serial.println("'");
            MQTT.MessageLength = MQTT.PublishIndex;
            if (QoS == 1)
            {
              MQTT.publishACK(MessageID);
            }
            else if (QoS == 2)
            {
              MQTT.publishREC(MessageID);
            }
            MQTT.OnMessage(MQTT.Topic, MQTT.TopicLength, MQTT.Message, MQTT.MessageLength);
            MQTT.MessageFlag = true;
          }
          else if (ReceivedMessageType == PUBREC)
          {
            Serial.print("Message ID :");
            MQTT.publishREL(0, MQTT.inputString[0] * 256 + MQTT.inputString[1]);
            Serial.println(MQTT.inputString[0] * 256 + MQTT.inputString[1]) ;

          }
          else if (ReceivedMessageType == PUBREL)
          {
            Serial.print("Message ID :");
            MQTT.publishCOMP(MQTT.inputString[0] * 256 + MQTT.inputString[1]) ;
            Serial.println(MQTT.inputString[0] * 256 + MQTT.inputString[1]) ;

          }
          else if ((ReceivedMessageType == PUBACK) || (ReceivedMessageType == PUBCOMP) || (ReceivedMessageType == SUBACK) || (ReceivedMessageType == UNSUBACK))
          {
            Serial.print("Message ID :");
            Serial.println(MQTT.inputString[0] * 256 + MQTT.inputString[1]) ;
          }
          else if (ReceivedMessageType == PINGREQ)
          {
            MQTT.TCP_Flag = false;
            MQTT.pingFlag = false;
            Serial.println("Disconnecting1");
            MQTT.sendATreply("AT+CIPSHUT\r\n", ".", 4000) ;
            MQTT.modemStatus = 0;
          }
        }
      }
      else if ((inChar = 13) || (inChar == 10))
      {
      }
      else
      {
        Serial.print("Received :Unknown Message Type :");
        Serial.println(inChar);
      }
    }
  }
}
