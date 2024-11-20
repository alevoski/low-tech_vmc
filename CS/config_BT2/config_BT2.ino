// Working to change name and password !
// https://forum.arduino.cc/t/hc-05-at-pswd-doesnt-save/965871/7

#include <SoftwareSerial.h>
SoftwareSerial Bluetooth(13,12);

char c=' ';
void setup() 
{
  Serial.begin(9600);
  Serial.println("ENTER AT COMMAND");
  Bluetooth.begin(38400);
}

void loop() 
{
  if(Bluetooth.available())
  {
    c=Bluetooth.read();
    Serial.write(c);
  }
  if(Serial.available())
  {
    c=Serial.read();
    Bluetooth.write(c);
  }
}
