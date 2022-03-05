/* 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 * More pin layouts for other boards can be found here: https://github.com/miguelbalboa/rfid#pin-layout
 * 
 * Using https://github.com/daknuett/cryptosuite2 for hashing the UID.
 */

#include <SPI.h>
#include <MFRC522.h>
#include <sha256.h>
#include <string.h>

#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_PIN          10         // Configurable, see typical pin layout above
#define RELAY_PIN       2

MFRC522::MIFARE_Key key = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; // use the one you have written to the tag

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
 // Init array that will store new NUID 
byte nuidPICC[4];
const char hexMap[] PROGMEM = "0123456789abcdef";
String hashedUID = "28dae7c8bde2f3ca608f86d0e16a214dee74c74bee011cdfdd46bc04b655bc14";

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() { 
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
}

void loop() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
 
  String hash = getHash(rfid.uid.uidByte, rfid.uid.size);
  if(isSameKey(hash)) {
    Serial.println(F("Same!"));
    closeRelay();
  } else {
    Serial.println(F("Different Card!"));
  }

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

String getHash(byte *buffer, byte bufferSize) {
  Sha256.init();
  Sha256.print(*buffer);
  uint8_t * result = Sha256.result();

  String hash = "";
  
  for (int i = 0; i < 32; i++) {
          hash += (char)pgm_read_byte(hexMap + (result[i] >> 4));
          hash += (char)pgm_read_byte(hexMap + (result[i] & 0xf));
  }

  Serial.println("");
  Serial.println("Got the following hash.");
  Serial.println(hash);

  return hash;
}

bool isSameKey(String key) {
  return hashedUID.equals(key);
}

void closeRelay() {
  digitalWrite(RELAY_PIN, HIGH);
  delay(500);
  digitalWrite(RELAY_PIN, LOW);
}
