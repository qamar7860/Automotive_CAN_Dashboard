

/*pio device monitor -p /dev/ttyUSBX -b 115200
//ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
//dmesg | tail -20

*/
#include <SPI.h>
#include <mcp2515.h>
#include <DHT.h>

// pio device monitor -p /dev/esp32_ecu -b 115200

#define CAN_CS 5

#define DHT_PIN 15
#define DHT_TYPE DHT11

#define SPEED_PIN 34
#define RPM_PIN   35
#define FUEL_PIN  32

// IMPORTANT: check the crystal printed on your MCP2515 board.
// Common breakout boards use 8MHz OR 16MHz - using the wrong one
// silently breaks bit timing and nothing will transmit correctly.
#define MCP_CRYSTAL MCP_8MHZ   // change to MCP_16MHZ if your board has a 16MHz can

DHT dht(DHT_PIN, DHT_TYPE);
MCP2515 mcp2515(CAN_CS);
struct can_frame canMsg;

void setup()
{
  Serial.begin(115200);
  delay(1000); // give serial monitor time to attach

  dht.begin();

  SPI.begin(18, 19, 23, CAN_CS); // SCK, MISO, MOSI, SS

  // ---- MCP2515 init with explicit error checks ----
  MCP2515::ERROR err;

  err = mcp2515.reset();
  Serial.print("MCP2515 reset: ");
  Serial.println(err == MCP2515::ERROR_OK ? "OK" : "FAILED");

  err = mcp2515.setBitrate(CAN_500KBPS, MCP_CRYSTAL);
  Serial.print("MCP2515 setBitrate: ");
  Serial.println(err == MCP2515::ERROR_OK ? "OK" : "FAILED");

  err = mcp2515.setNormalMode();
  Serial.print("MCP2515 setNormalMode: ");
  Serial.println(err == MCP2515::ERROR_OK ? "OK" : "FAILED");

  Serial.println("ESP32 ECU READY");
}

void loop()
{
  // ---- Read DHT11 ----
  float temperature = dht.readTemperature();
  float humidity    = dht.readHumidity();

  if (isnan(temperature)) temperature = 0;
  if (isnan(humidity))    humidity = 0;

  // ---- Read potentiometers ----
  int speedRaw = analogRead(SPEED_PIN);
  int rpmRaw   = analogRead(RPM_PIN);
  int fuelRaw  = analogRead(FUEL_PIN);

  int speed = map(speedRaw, 0, 4095, 0, 120);
  int rpm   = map(rpmRaw,   0, 4095, 0, 6000);
  int fuel  = map(fuelRaw,  0, 4095, 0, 100);

  // ---- Build CAN frame ----
  canMsg.can_id  = 0x100;
  canMsg.can_dlc = 8;

  canMsg.data[0] = (rpm >> 8) & 0xFF;
  canMsg.data[1] = rpm & 0xFF;
  canMsg.data[2] = (speed >> 8) & 0xFF;
  canMsg.data[3] = speed & 0xFF;
  canMsg.data[4] = (uint8_t)temperature;
  canMsg.data[5] = (uint8_t)fuel;
  canMsg.data[6] = (uint8_t)humidity;
  canMsg.data[7] = 0;

  // ---- Send ----
  MCP2515::ERROR sendErr = mcp2515.sendMessage(&canMsg);

  Serial.print("RPM: ");        Serial.print(rpm);
  Serial.print(" | Speed: ");   Serial.print(speed);
  Serial.print(" | Temp: ");    Serial.print(temperature);
  Serial.print(" | Fuel: ");    Serial.print(fuel);
  Serial.print(" | Humidity: ");Serial.print(humidity);
  Serial.print(" | Send: ");
  Serial.println(sendErr == MCP2515::ERROR_OK ? "OK" : "ERROR (" + String(sendErr) + ")");

  delay(500);
}
/**
 * | TFT      | STM32 |
| -------- | ----- |
| SCK      | PA5   |
| SDO/MISO | PA6   |
| SDI/MOSI | PA7   |
| CS       | PB1   |
| DC       | PB10  |
| RESET    | PB11  |
| VCC      | 3.3V* |
| GND      | GND   |
| LED      | 3.3V* |
Prescaler = 4
Time Quanta in Bit Segment 1 = 14 TQ
Time Quanta in Bit Segment 2 = 3 TQ
Synchronization Jump Width = 1 TQ
SPI
PA5 → SCK
PA6 → MISO
PA7 → MOSI

 */