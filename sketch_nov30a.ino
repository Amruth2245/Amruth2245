#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>

// Define pins for modules and peripherals
#define RST_PIN 9      // RC522 Reset pin
#define SS_PIN 10      // RC522 SDA pin
#define BUZZER_PIN 2   // Buzzer pin
#define SWITCH_PIN 6   // Switch pin for SMS

// Initialize LCD, RFID, and SIM800L
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);
SoftwareSerial sim800l(7, 8); // RX, TX for SIM800L

// Define item structure
struct Item {
  String name;
  float price;
};

// RFID tags and associated items
String knownUIDs[] = {
  "43:72:f4:a3", "83:8d:b2:92", "43:57:f4:19",
  "73:a3:38:25", "63:3b:65:1a", "83:1f:81:1a",
  "23:bc:6b:1a"
};
Item items[] = {
  {"Milk", 20.5}, {"Horlicks", 150.75}, {"Brue", 27.25},
  {"Boost", 180.0}, {"Glucose", 80.75}, {"Honey", 50.25},
  {"Sugar", 40.5}
};

const int itemCount = sizeof(knownUIDs) / sizeof(knownUIDs[0]);

// Variables
float total = 0.0;         // Running total of cart
String lastUID = "";       // To prevent duplicate scans
String phoneNumber = "+918925373813"; // Replace with target phone number

void setup() {
  // Initialize Serial and SIM800L
  Serial.begin(9600);
  sim800l.begin(9600);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.print("SMART TROLLEY");
  delay(2000);

  lcd.clear();
  lcd.print("Initializing...");
  delay(3000);
  

  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();

  // Initialize peripherals
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP); // Internal pull-up resistor for switch

  lcd.clear();
  lcd.print("Ready to Scan!");
  
}

void loop() {
if(total){
 // Check if the switch is pressed to send SMS
  if (digitalRead(SWITCH_PIN) == LOW) { // Active LOW
    sendSMS();
    delay(1000); // Debounce
  }
}
  // Check for RFID card
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Retrieve UID from the RFID tag
  String uid = getUID();
  if (uid == lastUID) return; // Ignore duplicate scans
  lastUID = uid;

  // Match UID to known items
  int index = findItemIndex(uid);
  if (index != -1) {
    beepBuzzer();
    addItem(index);
  } else {
    beepBuzzer();
    handleUnknownItem();
  }

  rfid.PICC_HaltA(); // Halt RFID to allow next scan
  delay(2000);

}

// Get UID as a string
String getUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += ":";
  }
  uid.toLowerCase(); // Consistent comparison
  return uid;
}

// Find item index based on UID
int findItemIndex(String uid) {
  for (int i = 0; i < itemCount; i++) {
    if (uid == knownUIDs[i]) return i;
  }
  return -1; // Not found
}

// Add item to cart and update total
void addItem(int index) {
  total += items[index].price;

  lcd.clear();
  lcd.print("Added: ");
  lcd.print(items[index].name);
  lcd.setCursor(0, 1);
  lcd.print("Total: Rs ");
  lcd.print(total);

  Serial.print("Added: ");
  Serial.print(items[index].name);
  Serial.print(" | Price: Rs ");
  Serial.println(items[index].price);
}

// Handle unknown RFID tags
void handleUnknownItem() {
  lcd.clear();
  lcd.print("Unknown Item!");
  delay(2000);

  Serial.println("Unknown RFID tag scanned.");
}

// Play a beep using the buzzer
void beepBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
}

// Send SMS with total cost
void sendSMS() {
  String message = "Total cost: Rs " + String(total); // SMS content

  lcd.clear();
  lcd.print("Sending SMS...");
  //delay(1000);
sim800l.println("AT"); // Check communication
  delay(500);
  sim800l.println("AT+CMGF=1"); // Text mode
  delay(500);
  

  sim800l.println("AT+CMGS=\"+918925373813\"");
  delay(500);

  sim800l.print(message); // Message content
  delay(500);

  sim800l.write(26); // End of message (CTRL+Z)
  delay(5000);

  lcd.clear();
  lcd.print("SMS Sent!");
 delay(2000);

  Serial.println("SMS Sent: " + message);
}
