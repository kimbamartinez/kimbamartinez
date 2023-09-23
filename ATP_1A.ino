#define BLYNK_TEMPLATE_ID "TMPLgzEh6kA9"
#define BLYNK_TEMPLATE_NAME "NodeMCU"
#define BLYNK_AUTH_TOKEN "0EZ5HIou7Sr0s3FeRCJnRW6R-j6ddrYk"
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <DHT.h>
#include <HX711.h>
#include <Servo.h>
#include <BlynkSimpleEsp8266.h>
// #include <Blynk.h>
// #include <TimeLib.h>
#include <ezTime.h>
Timezone myTZ;
#define DHTPIN D3      // DHT11 sensor pin
#define DHTTYPE DHT11 // DHT11 sensor type
#define RELAY1_PIN D2  // Relay1 pin
#define RELAY2_PIN D1    // Relay2 pin


DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
Servo servoMotor;
HX711 scale;

const int LOADCELL_DOUT_PIN = D7;
const int LOADCELL_SCK_PIN = D8;
float calibration_factor = -103500; //- 86670worked for my 440lb max scale setup

int servoPin = D4;       // Change this to the pin where you connected the servo motor
int feedingHour1 = 0;    // Initialize feeding hour for the first feeding time to 0
int feedingMinute1 = 0;  // Initialize feeding minute for the first feeding time to 0
int feedingHour2 = 0;    // Initialize feeding hour for the second feeding time to 0
int feedingMinute2 = 0;  // Initialize feeding minute for the second feeding time to 0
int warn;
int prev_warn = -1;
float temp;
float humidity;
float weight;
float m;
unsigned long interval = 2000l;  // Check if it's time to feed the poultry for the first feeding time

int currentHour = 0;
int currentMinute = 0;
int previousMinute1 = -1;
int previousMinute2 = -1;
  static bool prev_tempCold = false;
  static bool prev_tempHot = false;
  bool tempCold = (temp <= 29);
  bool tempHot = (temp >= 31);

char auth[] = "0EZ5HIou7Sr0s3FeRCJnRW6R-j6ddrYk";
char ssid[] = "realme 8 5G";
char pass[] = "Paloadka";

void weights()
{
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  weight = scale.get_units(), 3;
  Serial.print("Weight: ");
  Serial.print(weight, 3);
  Serial.print(" kg " );

  if(weight <=.05)
  {
    warn = 1;
  }
  else
  {
    warn = 0;
  }

  if(warn != prev_warn)
  {
    prev_warn = warn;

    if(warn == 1)
    {
      Blynk.logEvent("warning", "Store Feeds");
    }
  }

  Blynk.virtualWrite(V8, weight);  // Display weight on virtual pin 0
  Blynk.virtualWrite(V9, warn);    // Display warning on virtual pin 1
}

void openFoodStorage()
{
  servoMotor.write(140);
  delay(1000);
  servoMotor.write(0);
  delay(3000);

}

void closeFoodStorage()
{
  servoMotor.write(140);
  delay(100);


}


void feedPoultry()
{

  // Check if it's time to feed the poultry for the first feeding time
  if(currentHour == feedingHour1 && currentMinute == feedingMinute1 && currentMinute != previousMinute1)
  {
    // make sure it run only once
    previousMinute1 = currentMinute;
    if(warn == 0)
    {
      openFoodStorage();
      // Notify in Blynk that feeding is done
      Blynk.virtualWrite(V9, "Feeding 1 done");
      closeFoodStorage();
      Serial.println("Feeding 1 done");
      Blynk.logEvent("notification", "Done first feeding");
    }
    else
    {
      Serial.println("cannot feed");
      Blynk.logEvent("alert", "Cannot proceed schedule 1");
    }
  }

  // Check if it's time to feed the poultry for the second feeding time
  if(currentHour == feedingHour2 && currentMinute == feedingMinute2 && currentMinute != previousMinute2)
  {
    // make sure it run only once
    previousMinute2 = currentMinute;
    if(warn == 0)
    {
      openFoodStorage();
      // Notify in Blynk that feeding is done
      Blynk.virtualWrite(V9, "Feeding 2 done");
      closeFoodStorage();

      Serial.println("Feeding 2 done");
      Blynk.logEvent("notification", "Done second feeding");
    }
    else
    {
      Serial.println("cannot feed");
      Blynk.logEvent("alert", "Cannot proceed schedule 2");
    }
  }
}


void sendSensorData()
{
  temp = dht.readTemperature();  // Read temperature data from DHT11 sensor
  float hum = dht.readHumidity();      // Read humidity data from DHT11 sensor
  if(isnan(temp) || isnan(hum))
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" C | Humidity: ");
  Serial.print(hum);
  Serial.println(" %");
if (temp <=29) {
    digitalWrite(RELAY1_PIN, HIGH);  // Turn on Relay 1
    digitalWrite(RELAY2_PIN, LOW); // Turn off Relay 2
    
  }
  else if (temp >=31) {
    digitalWrite(RELAY1_PIN, LOW); // Turn off Relay 1
    digitalWrite(RELAY2_PIN, HIGH);  // Turn on Relay 2
  }
  else {
    digitalWrite(RELAY1_PIN, LOW);  // Turn off Relay 1
    digitalWrite(RELAY2_PIN, LOW);  // Turn off Relay 2
  }
if (tempCold != prev_tempCold) {
    prev_tempCold = tempCold;
    if (tempCold) {
      Blynk.logEvent("alert", "Fans On, Temperature is Hot");
    }
  }

  if (tempHot != prev_tempHot) {
    prev_tempHot = tempHot;
    if (tempHot) {
      Blynk.logEvent("alert", "Lights On! Temperature is Cold");
    }
  }

  Blynk.virtualWrite(V5, temp);  // Send temperature and humidity data to Blynk app
  Blynk.virtualWrite(V6, hum);
}

void printCurrentTime()
{
  currentHour = hour();
  currentMinute = minute();

  currentHour += 8;
  if(currentHour >= 24)
  {
    currentHour -= 24;
  }
  Serial.print("Time: ");
  Serial.print(currentHour);
  Serial.print(":");
  Serial.println(currentMinute);
}

// returns military time
void convertSecondsToHoursMinutes(int seconds_, int& hours_, int& minutes_)
{
  // Calculate total minutes
  int totalMinutes = seconds_ / 60;

  // Calculate hours and minutes separately
  hours_ = totalMinutes / 60;
  minutes_ = totalMinutes % 60;
}
BLYNK_CONNECTED()
{

  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
}

BLYNK_WRITE(V1)
{
  // Get the feeding hour for the first feeding time from Blynk
  // Get the feeding minute for the first feeding time from Blynk

  convertSecondsToHoursMinutes(param[0].asInt(), feedingHour1, feedingMinute1);
  Serial.print("Feeding Time 1: ");
  Serial.print(feedingHour1);
  Serial.print(":");
  Serial.println(feedingMinute1);
}

// This function is called whenever the second feeding time is updated in Blynk
BLYNK_WRITE(V2)
{
  // Get the feeding hour for the second feeding time from Blynk
  // Get the feeding minute for the second feeding time from Blynk

  convertSecondsToHoursMinutes(param[0].asInt(), feedingHour2, feedingMinute2);
  Serial.print("Feeding Time 2: ");
  Serial.print(feedingHour2);
  Serial.print(":");
  Serial.println(feedingMinute2);
}

void setup()
{

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  delay(3000);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);

  Serial.begin(115200);
  servoMotor.attach(servoPin, 500, 2400);
  servoMotor.write(140);
  dht.begin();

  Blynk.begin(auth, ssid, pass);
  Serial.println("Connected to Blynk.");

  timer.setInterval(interval, sendSensorData);
  timer.setInterval(interval, weights);
  timer.setInterval(interval, feedPoultry);
  timer.setInterval(interval, printCurrentTime);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  while(!scale.is_ready())
  {
    Serial.println("Waiting load cell...");
    delay(1000);
  }
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  scale.tare();
  float zero_factor = scale.read_average();  //Get a baseline reading
  Serial.print("Zero factor: ");            //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
  while(!waitForSync())
  {
    Serial.println("Waiting for sync");
    delay(1000);
  }
}

void loop()
{

  Blynk.run();
  timer.run();
  
  // This function is called whenever the first feeding time is updated in Blynk
}
