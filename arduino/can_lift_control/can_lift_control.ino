#include <SPI.h>       
#include <mcp2515.h>      

int PowerPin_s1 = 2;
int PowerPin_s2 = 6;
int PowerPin_s3 = 5;      // Always HIGH 5V pin
int DirectionPin_A = 7;   // HIGH when data = 1
int DirectionPin_B = 8;   // HIGH when data = -1

struct can_frame canMsg;
MCP2515 mcp2515(10);

unsigned long lastCanReceiveTime = 0;
unsigned long canTimeoutDuration = 500;  // 500ms timeout, adjust as needed
bool canTimeout = false;

void setup()
{
  while (!Serial);
  Serial.begin(115200);
  SPI.begin();

  pinMode(PowerPin_s1, OUTPUT);
  pinMode(PowerPin_s2, OUTPUT);
  pinMode(PowerPin_s3, OUTPUT);
  pinMode(DirectionPin_A, OUTPUT);
  pinMode(DirectionPin_B, OUTPUT);

  digitalWrite(PowerPin_s3, HIGH);   // Always HIGH
  digitalWrite(DirectionPin_A, LOW);
  digitalWrite(DirectionPin_B, LOW);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  lastCanReceiveTime = millis();  // Initialize timer
}

void loop()
{
  read_can_messages();

  // Check if CAN message has timed out
  if (millis() - lastCanReceiveTime > canTimeoutDuration) {
    if (!canTimeout) {
      digitalWrite(DirectionPin_A, LOW);
      digitalWrite(DirectionPin_B, LOW);
      Serial.println("CAN Timeout: Both pins set LOW");
      canTimeout = true;
    }
  }

  delay(10);
}

void read_can_messages() {
  can_frame frame;
  if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {

    lastCanReceiveTime = millis();  // Reset timer on every received message
    canTimeout = false;             // Clear timeout flag

    // CAN ID 0x37 controls PowerPin_s2
    if (frame.can_id == 0x37) {
      byte received_data = frame.data[0];
      if (received_data == 1) {
        digitalWrite(PowerPin_s2, HIGH);
        Serial.println("PowerPin_s2: HIGH");
      } else {
        digitalWrite(PowerPin_s2, LOW);
        Serial.println("PowerPin_s2: LOW");
      }
    }

    // CAN ID 0x38 controls Direction Pins 7 and 8
    if (frame.can_id == 0x38) {
      int8_t direction_data = (int8_t)frame.data[0];

      if (direction_data == 1) {
        digitalWrite(DirectionPin_A, HIGH);
        digitalWrite(DirectionPin_B, LOW);
        Serial.println("Direction: Pin7=HIGH, Pin8=LOW");
      }
      else if (direction_data == -1) {
        digitalWrite(DirectionPin_A, LOW);
        digitalWrite(DirectionPin_B, HIGH);
        Serial.println("Direction: Pin7=LOW, Pin8=HIGH");
      }
      else {
        digitalWrite(DirectionPin_A, LOW);
        digitalWrite(DirectionPin_B, LOW);
        Serial.println("Direction: Unknown value, both LOW");
      }
    }
  }
}
