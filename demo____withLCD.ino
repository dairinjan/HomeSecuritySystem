#include <SoftwareSerial.h>
#include <String.h>
SoftwareSerial gprsSerial(2,3);
#include <LiquidCrystal_I2C.h>          // library for LCD with I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);     // Set the LCD address (0x27) and size of lcd (16 chars and 2 line display)

//Sensor pin numbers:
int fire_pin = A1;
int gas_pin  = A0;

//buzzer pin:
int buzzer_pin = 13; 

int fire_reading = 0;
int gas_reading  = 0;

//average reading:
int average_measurement_fire = 115;
int average_measurement_gas  = 115; 

//maximum reading level to send an alert:
int alert_level_fire = 90;
int alert_level_gas  = 500; 

//const unsigned long eventInterval = 1800000;//30 minutes
//const unsigned long eventInterval = 120000;//2 minutes
//const unsigned long eventInterval = 300000;//5 minutes 
const unsigned long eventInterval = 600000;//10 minutes 
unsigned long previousTime = 0;

void setup() {
  Serial.begin(9600); //Handshaking
  lcd.init();         // initialize the LCD
  lcd.backlight();    // Turn on the blacklight
  
  gprsSerial.begin(9600); //Handshaking for Sim900
  pinMode(fire_pin, INPUT);
  pinMode(gas_pin, INPUT);
  pinMode(buzzer_pin, OUTPUT);
  delay(1000);
}

void loop() {
  fire_reading = analogRead(fire_pin) - 800;  
  gas_reading  = analogRead(gas_pin );
  Serial.print("Fire = ");
  Serial.print(fire_reading);
  Serial.print("....Gas = ");
  Serial.println(gas_reading);
  //getting the moving average: 
  average_measurement_fire   = (average_measurement_fire   * 99 + fire_reading  ) / 100;
  average_measurement_gas   = (average_measurement_gas   * 99 + gas_reading  ) / 100;
  
  Serial.print("Ave Fire = ");
  Serial.print(average_measurement_fire);
  Serial.print("....Ave Gas = ");
  Serial.println(average_measurement_gas);
  

  //display the fire reading:
  lcd.clear();                       // clear the LCD
  lcd.setCursor (0, 0);              // Set the cursor
  lcd.print("Fire Level:" );         // print the message
  lcd.setCursor (12, 0);             // Set the cursor
  lcd.print(average_measurement_fire);           // print the message

  //display the gas reading:  
  lcd.setCursor (0, 1);             // Set the cursor
  lcd.print("Gas Level:" );       // print the message
  lcd.setCursor (12, 1);             // Set the cursor
  lcd.print(average_measurement_gas);       // print the message
  
  if (millis() - previousTime >= eventInterval) {
    
    Serial.println("Writing into the ThinkSpeak");
    sendReading(average_measurement_fire, average_measurement_gas);
    
    previousTime = millis();
  }
  //triggerCall();
  if(average_measurement_fire < alert_level_fire)
  {
    tone(buzzer_pin, 2400, 3000); 
    sendSMS(fire_reading,"fire");
    triggerCall();
    average_measurement_fire = 115;
  }
  else
  {
    noTone(buzzer_pin);
  }
  if(average_measurement_gas > alert_level_gas)
  {
    tone(buzzer_pin, 2400, 3000);
    sendSMS(gas_reading,"gas");
    average_measurement_gas = 115;
  }
  else
  {
    noTone(buzzer_pin);
  }
  delay(2000);
}
void SIM900power()
{
  pinMode(9, OUTPUT); 
  digitalWrite(9,LOW);
  delay(1000);
  digitalWrite(9,HIGH);
  delay(2000);
  digitalWrite(9,LOW);
  delay(3000);
}
void ShowSerialData()
{
  while(gprsSerial.available()!=0)
  Serial.write(gprsSerial.read());
  delay(1000); 
}

void triggerCall()
{
  gprsSerial.println("AT"); //Handshaking with SIM900
  ShowSerialData(); 
  gprsSerial.println("ATD+ +639152411363;"); 
  ShowSerialData();
  delay(20000); // wait for 20 seconds...
  gprsSerial.println("ATH"); //hang up
  ShowSerialData();
}
void sendSMS(int reading, String type)
{
  gprsSerial.println("AT+CSCS=\"GSM\""); //Handshaking with SIM900
  ShowSerialData(); 
  delay(2000);
  gprsSerial.println("AT+CMGS=\"+639152411363\"");
  ShowSerialData(); 
  delay(2000);
  // REPLACE WITH YOUR OWN SMS MESSAGE CONTENT
  gprsSerial.println("****************"); 
  gprsSerial.println("SMART HOME SECURITY SYSTEM"); 
  if(type == "gas")
  {
    gprsSerial.println("Gas Leak has been detected!"); 
  }
  else
  {
    gprsSerial.println("Fire has been detected!"); 
  }
  gprsSerial.println("Intensity: ");
  gprsSerial.println(reading);
  gprsSerial.println("****************");
  delay(2000);
  gprsSerial.println((char)26); // End AT command with a ^Z, ASCII code 26
  delay(5000);
  gprsSerial.println();
  // Give module time to send SMS
  delay(5000); 
}
void sendReading(int fire_reading, int gas_reading)
{
  if (gprsSerial.available())
    Serial.write(gprsSerial.read());
 
  gprsSerial.println("AT");
  delay(1000);
 
  gprsSerial.println("AT+CPIN?");
  delay(1000);
 
  gprsSerial.println("AT+CREG?");
  delay(1000);
 
  gprsSerial.println("AT+CGATT?");
  delay(1000);
 
  gprsSerial.println("AT+CIPSHUT");
  delay(1000);
 
  gprsSerial.println("AT+CIPSTATUS");
  delay(1000);
 
  gprsSerial.println("AT+CIPMUX=0");
  delay(1000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CSTT=\"internet\"");//start task and setting the APN,
  delay(2000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIICR");//bring up wireless connection
  delay(1000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIFSR");//get local IP adress
  delay(2000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIPSPRT=0");
  delay(1000);
 
  ShowSerialData();
  
  gprsSerial.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");//start up the connection
  delay(3000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIPSEND");//begin send data to remote server
  delay(2000);
  ShowSerialData();

  String str="GET https://api.thingspeak.com/update?api_key=JEN13SUU3O3A7Q7H&field1=" + String(fire_reading) +"&field2="+String(gas_reading);
  Serial.println(str);
  gprsSerial.println(str);//begin send data to remote server
  
  delay(2000);
  ShowSerialData();
 
  gprsSerial.println((char)26);//sending
  delay(3000);//waitting for reply, important! the time is base on the condition of internet 
  gprsSerial.println();
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIPSHUT");//close the connection
  delay(100);
  ShowSerialData();
}
