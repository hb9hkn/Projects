/*******************************************
 * 
 * Author: Matej Sustic
 * Date: 11 Jan 2025
 * 
 * Version control:
 * - FINAL VERSION
 * 
 */

#include <TFT_eSPI.h>

// Initialize the TFT display
TFT_eSPI tft = TFT_eSPI();

#define VERSION 1.00

// Define colors
#define BORDER_COLOR TFT_WHITE
#define TEXT_COLOR TFT_GREEN
#define NAME_COLOR TFT_WHITE
#define BACKGROUND_COLOR TFT_BLACK
#define FREQUENCY_COLOR TFT_YELLOW
#define MODE_COLOR TFT_YELLOW
#define VALUE_COLOR TFT_GREEN
#define DEFAULT_COLOR TFT_WHITE
#define SPLIT_COLOR TFT_RED
#define SELECTION_COLOR TFT_BLUE

// Encoder pins
#define ENCODER1_PIN_A 34
#define ENCODER1_PIN_B 35
#define BUTTON1_PIN 32
#define ENCODER2_PIN_A 36
#define ENCODER2_PIN_B 39
#define BUTTON2_PIN 33

// Band up/down button pins
#define BAND_UP_PIN 27
#define BAND_DOWN_PIN 26

#define TUNE_BUTTON_PIN 2
#define ZERO_IN_BUTTON_PIN 12
#define BUTTON3_PIN 13
#define BUTTON4_PIN 14
#define BUTTON5_PIN 15
#define BUTTON6_PIN 25

// Debounce delay for buttons
#define DEBOUNCE_DELAY 20

// Debounce variables for button states and debouncing
unsigned long lastDebounceTimeBandUp = 0, lastDebounceTimeBandDown = 0;
bool lastBandUpState = true, lastBandDownState = true;
bool bandUpState = true, bandDownState = true;
unsigned long lastDebounceTimeTune = 0;
unsigned long lastDebounceTimeZeroIn = 0;
unsigned long lastDebounceTimeButton3 = 0;
unsigned long lastDebounceTimeButton4 = 0;
unsigned long lastDebounceTimeButton5 = 0;
unsigned long lastDebounceTimeButton6 = 0;
bool lastTuneButtonState = true;
bool lastZeroInButtonState = true;
bool lastButton3State = true;
bool lastButton4State = true;
bool lastButton5State = true;
bool lastButton6State = true;
bool tuneButtonState = true;
bool zeroInButtonState = true;
bool Button3State = true;
bool Button4State = true;
bool Button5State = true;
bool Button6State = true;
String lastFormattedFrequency = "";
bool isSplit = false;  // Track if split mode is active

// Toggle state for tuning mode
bool isTuning = false;

// RS232 Configuration
#define RX2_PIN 16  // Connect to TX of the radio
#define TX2_PIN 17  // Connect to RX of the radio
#define SERIAL2_BAUD 38400

// Screen dimensions
const int SCREEN_WIDTH = 480;
const int SCREEN_HEIGHT = 320;

// New top info bar height
const int TOP_BAR_HEIGHT = SCREEN_HEIGHT * 0.1;

// Adjusted field dimensions
const int TOP_HEIGHT = SCREEN_HEIGHT * 0.3;
const int BOTTOM_HEIGHT = SCREEN_HEIGHT * 0.6;

// Field variables
int fieldValues[10] = {0, 0, 75, 22, 33, 44, 55, 66, 77, 88};
int lastFieldValues[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
// Global array for string-based fields
String fieldStringValues[10] = {"", "", "", "", "", "", "", "", "", ""};


// Field names
const char* fieldNames[10] = {
    "CW Speed / wpm", "RF Power / W",
    "BK-Delay / msec", "Moni Level", "Dimmer LCD", "Dimmer LED",
    "CW Pitch / Hz", "Level / dB", "Keyer", "DNF"
};

// Encoder-specific field mappings
const int encoder1Fields[] = {0, 2, 3, 6, 7};
const int encoder2Fields[] = {1, 4, 5};
const int encoder1FieldCount = sizeof(encoder1Fields) / sizeof(encoder1Fields[0]);
const int encoder2FieldCount = sizeof(encoder2Fields) / sizeof(encoder2Fields[0]);

// Debounce for encoders
volatile unsigned long lastEncoder1Interrupt = 0;
volatile unsigned long lastEncoder2Interrupt = 0;
const unsigned long debounceDelay = 0; // Adjust to fine-tune debounce in milliseconds

// Encoder values and states
volatile int encoder1RawValue = 0;
volatile int encoder2RawValue = 0;

volatile int lastEncoder1State = 0;
volatile int lastEncoder2State = 0;

volatile int encoder1Direction = 0; // 1 = clockwise, -1 = counterclockwise, 0 = no change
volatile int encoder2Direction = 0; // 1 = clockwise, -1 = counterclockwise, 0 = no change

// Debounce variables for buttons
unsigned long lastDebounceTime1 = 0, lastDebounceTime2 = 0;
bool lastButtonState1 = false, lastButtonState2 = false;
bool buttonState1 = false, buttonState2 = false;

// Debounce (buttons!) delay in milliseconds
#define DEBOUNCE_DELAY 10

// serial wait period in msec (waiting for RS232 response)
unsigned long myTimeout = 25;

// Modes for encoders
enum Mode { SELECTION, VALUE };
Mode mode1 = VALUE;
Mode mode2 = VALUE;

// Current field selection
int selectedField1 = 0;
int selectedField2 = 0;

// Function declarations
void drawTopBar(const char* info);
void drawBorders();
void updateAllFields();
void updateField(int fieldIndex, int value, uint16_t color = DEFAULT_COLOR);
void handleEncoder1();
void handleEncoder2();
void handleButton1();
void handleButton2();
void sendCATCommand(const char* command);
void handleBandUpButton();
void handleBandDownButton();
String readCATResponse(unsigned long myTimeout);
void processCATResponses();
void processTopBarInfo();
//void processField0(); // CW Speed
//void processField1(); // RF Power
//void processField2(); // BK-Delay
// Additional process functions for other fields...

void updateFieldString(int fieldIndex, const String& value, uint16_t color) {
  int fieldWidth, fieldHeight, x, y;
  bool isTopField = (fieldIndex < 2);

  if (isTopField) {
    fieldWidth = SCREEN_WIDTH / 2;
    fieldHeight = TOP_HEIGHT;
    x = (fieldIndex == 0) ? 0 : SCREEN_WIDTH / 2;
    y = TOP_BAR_HEIGHT;
  } else {
    fieldIndex -= 2;
    fieldWidth = SCREEN_WIDTH / 4;
    fieldHeight = BOTTOM_HEIGHT / 2;
    int col = fieldIndex % 4;
    int row = fieldIndex / 4;
    x = col * fieldWidth;
    y = TOP_BAR_HEIGHT + TOP_HEIGHT + row * fieldHeight;
  }

  // Clear the previous value
  tft.fillRect(x + 3, y + 30, fieldWidth - 6, fieldHeight - 35, BACKGROUND_COLOR);

  // Set font and color
  tft.setTextFont(isTopField ? 6 : 4);
  tft.setTextColor(color, BACKGROUND_COLOR);

  // Center the text
  int textWidth = tft.textWidth(value, isTopField ? 6 : 4);
  int textX = x + (fieldWidth - textWidth) / 2;
  int textY = y + (fieldHeight - 8 * (isTopField ? 6 : 4)) / 2 + (isTopField ? 20 : 15);

  tft.setCursor(textX, textY);
  tft.print(value);
}

void IRAM_ATTR handleEncoder1Interrupt() {
  unsigned long now = millis();
  if (now - lastEncoder1Interrupt > debounceDelay) { // Software debouncing
    int aState = digitalRead(ENCODER1_PIN_A);
    int bState = digitalRead(ENCODER1_PIN_B);
    int currentState = (aState << 1) | bState;

    // Direction detection
    if ((lastEncoder1State == 0b00 && currentState == 0b01) || 
        (lastEncoder1State == 0b01 && currentState == 0b11) ||
        (lastEncoder1State == 0b11 && currentState == 0b10) || 
        (lastEncoder1State == 0b10 && currentState == 0b00)) {
      encoder1Direction = 1; // Clockwise
    } else if ((lastEncoder1State == 0b00 && currentState == 0b10) ||
               (lastEncoder1State == 0b10 && currentState == 0b11) ||
               (lastEncoder1State == 0b11 && currentState == 0b01) ||
               (lastEncoder1State == 0b01 && currentState == 0b00)) {
      encoder1Direction = -1; // Counterclockwise
    }

    lastEncoder1State = currentState;
    lastEncoder1Interrupt = now; // Update debounce timestamp
  }
}

void IRAM_ATTR handleEncoder2Interrupt() {
  unsigned long now = millis();
  if (now - lastEncoder2Interrupt > debounceDelay) { // Software debouncing
    int aState = digitalRead(ENCODER2_PIN_A);
    int bState = digitalRead(ENCODER2_PIN_B);
    int currentState = (aState << 1) | bState;

    // Direction detection
    if ((lastEncoder2State == 0b00 && currentState == 0b01) || 
        (lastEncoder2State == 0b01 && currentState == 0b11) ||
        (lastEncoder2State == 0b11 && currentState == 0b10) || 
        (lastEncoder2State == 0b10 && currentState == 0b00)) {
      encoder2Direction = 1; // Clockwise
    } else if ((lastEncoder2State == 0b00 && currentState == 0b10) ||
               (lastEncoder2State == 0b10 && currentState == 0b11) ||
               (lastEncoder2State == 0b11 && currentState == 0b01) ||
               (lastEncoder2State == 0b01 && currentState == 0b00)) {
      encoder2Direction = -1; // Counterclockwise
    }

    lastEncoder2State = currentState;
    lastEncoder2Interrupt = now; // Update debounce timestamp
  }
}


void setup() {
  // Initialize the display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BACKGROUND_COLOR);
  tft.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);

  // Initialize Serial2 for RS232 communication
  Serial2.begin(SERIAL2_BAUD, SERIAL_8N1, RX2_PIN, TX2_PIN);
  Serial.begin(38400);

  // Draw the top info bar
  drawTopBar("CAT in the Box v" + String(VERSION) + " Initializing...", "", "");

  // Draw the borders and field names
  drawBorders();

  // short delay for Serial setup
  delay (3000);

  // Send initial CAT commands to fetch values
  processCATResponses();

  // Initialize encoder pins
  pinMode(ENCODER1_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER1_PIN_B, INPUT_PULLUP);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(ENCODER2_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER2_PIN_B, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

    // Attach interrupts for Encoder 1
  attachInterrupt(digitalPinToInterrupt(ENCODER1_PIN_A), handleEncoder1Interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER1_PIN_B), handleEncoder1Interrupt, CHANGE);

  // Attach interrupts for Encoder 2
  attachInterrupt(digitalPinToInterrupt(ENCODER2_PIN_A), handleEncoder2Interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER2_PIN_B), handleEncoder2Interrupt, CHANGE);

  // Initialize button pins
  pinMode(BAND_UP_PIN, INPUT_PULLUP);
  pinMode(BAND_DOWN_PIN, INPUT_PULLUP);
  pinMode(TUNE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ZERO_IN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);
  pinMode(BUTTON5_PIN, INPUT_PULLUP);
  pinMode(BUTTON6_PIN, INPUT_PULLUP);
}

void loop() {

  handleTuneButton();

  if (!isTuning) {
      handleBandUpButton();
      handleBandDownButton();
      handleButton1();
      handleButton2();
      handleButton3();
      handleButton4();
      handleButton5();
      handleButton6();
    
      handleZeroInButton();
      
      static int lastValue1 = 0;
      static int lastValue2 = 0;
    
      if (encoder1RawValue != 0) {
        int correctedValue = (encoder1RawValue > 0) ? 1 : -1; // Normalize encoder step
        encoder1RawValue = 0;
      }
    
      if (encoder2RawValue != 0) {
        int correctedValue = (encoder2RawValue > 0) ? 1 : -1; // Normalize encoder step
        encoder2RawValue = 0;
      }
    
      // Process Encoder 1
      if (encoder1Direction != 0) {
        processEncoder1(encoder1Direction);
        encoder1Direction = 0; // Reset after processing
      }
    
      // Process Encoder 2
      if (encoder2Direction != 0) {
        processEncoder2(encoder2Direction);
        encoder2Direction = 0; // Reset after processing
      }
    
      // Send initial CAT commands to fetch values
      processCATResponses();

  }
}

void refreshDisplay() {
  // Clear the display and reset the default screen

  tft.fillScreen(BACKGROUND_COLOR);

  // Add code here to refresh the display with the default state
  // For example, redraw UI elements or information.
    tft.setTextSize(1);
      // Reset fieldValues
  for (int i = 0; i < 10; i++) {
    fieldValues[i] = 500;
    fieldStringValues[i] = "";
  }
    lastFormattedFrequency = "";
    drawBorders();
    processCATResponses();
}

void displayTuningMessage() {
  // Clear the display
  // tft.fillScreen(TFT_BLACK);

  // Draw a framed rectangle for the "TUNING" message
  tft.fillRect(45, 95, 380, 130, TFT_BLACK);  // Inner black background
  //  tft.fillScreen(BACKGROUND_COLOR);
  tft.drawRect(50, 100, 370, 120, TFT_RED);  // Frame


  // Set text color, size, and position
  
  tft.setTextFont(4);
  tft.setTextColor(VALUE_COLOR, BACKGROUND_COLOR);
  tft.setTextSize(2);

  // Center the "TUNING" text in the rectangle
  int textX = (480 - tft.textWidth("TUNING")) / 2;
  int textY = 145;  // Vertically centered
  tft.setCursor(textX, textY);
  tft.print("TUNING");
}

void handleTuneButton() {
  static String originalMode = "";
  static String originalPower = "";
  int reading = digitalRead(TUNE_BUTTON_PIN);

  if (reading != lastTuneButtonState) {
    lastDebounceTimeTune = millis();  // Reset debounce timer
  }

  if ((millis() - lastDebounceTimeTune) > DEBOUNCE_DELAY) {
    if (reading != tuneButtonState) {
      tuneButtonState = reading;

      if (tuneButtonState == LOW) {  // Button pressed
        if (isTuning) {
          // Pull station out of tuning mode
          sendCATCommand("TX0;KR1;ED020;");
          if (!originalMode.isEmpty() && !originalPower.isEmpty()) {
            // Convert String to const char* using .c_str()
            delay(30); // delay needed for the FTDX10 to process the previous commands
            String restoreCommand =  "PC" + originalPower + ";MD0" + originalMode + ";"; 
            sendCATCommand(restoreCommand.c_str());
          }

          refreshDisplay();
        } else {
          // Query and store the current mode and power
          sendCATCommand("MD0;");  // Example: Get mode
          originalMode = readCATResponse(myTimeout);
          if (originalMode.startsWith("MD0") && originalMode.endsWith(";")) {
           // Extract substring after "MD" and before ";"
              originalMode = originalMode.substring(3,4);
          }
          sendCATCommand("PC;");  // Example: Get power
          originalPower = readCATResponse(myTimeout);
          if (originalPower.startsWith("PC") && originalPower.endsWith(";")) {
           // Extract substring after "PC" and before ";"
              originalPower = originalPower.substring(2,5);
          }

          // Put station into tuning state
          sendCATCommand("PC005;EU020;MD09;AC000;KR0;BI1;");
          refreshDisplay();
          sendCATCommand("TX1;");
          displayTuningMessage();
        }
        isTuning = !isTuning;  // Toggle tuning state
      }
    }
  }

  lastTuneButtonState = reading;
}


//void handleTuneButton() {
//  int reading = digitalRead(TUNE_BUTTON_PIN);
//
//  if (reading != lastTuneButtonState) {
//    lastDebounceTimeTune = millis();  // Reset debounce timer
//  }
//
//  if ((millis() - lastDebounceTimeTune) > DEBOUNCE_DELAY) {
//    if (reading != tuneButtonState) {
//      tuneButtonState = reading;
//
//      if (tuneButtonState == LOW) {  // Button pressed
//        if (isTuning) {
//          // Pull station out of tuning mode
//          sendCATCommand("KR1;ED030;PC100;");
//          refreshDisplay();
//        } else {
//          // Put station into tuning state
//          sendCATCommand("PC005;EU030;AC000;KR0;BI1;");
//          displayTuningMessage();
//        }
//        isTuning = !isTuning;  // Toggle tuning state
//      }
//    }
//  }
//
//  lastTuneButtonState = reading;
//}

void handleZeroInButton() {
  int reading = digitalRead(ZERO_IN_BUTTON_PIN);

  if (reading != lastZeroInButtonState) {
    lastDebounceTimeZeroIn = millis();  // Reset debounce timer
  }

  if ((millis() - lastDebounceTimeZeroIn) > DEBOUNCE_DELAY) {
    if (reading != zeroInButtonState) {
      zeroInButtonState = reading;

      if (zeroInButtonState == LOW) {  // Button pressed
        // Send zero-in command
        sendCATCommand("ZI0;");
      }
    }
  }

  lastZeroInButtonState = reading;
}

void handleButton3() {
  static bool toggleCommand3 = false;  // Tracks which command to send
  int reading = digitalRead(BUTTON3_PIN);

  if (reading != lastButton3State) {
    lastDebounceTimeButton3 = millis();  // Reset debounce timer
  }

  if ((millis() - lastDebounceTimeButton3) > DEBOUNCE_DELAY) {
    if (reading != Button3State) {
      Button3State = reading;

      if (Button3State == LOW) {  // Button pressed
        if (toggleCommand3) {
          // Send the first command
          sendCATCommand("BI1;");
        } else {
          // Send the second command
          sendCATCommand("BI0;");
        }
        toggleCommand3 = !toggleCommand3;  // Toggle for the next press
      }
    }
  }

  lastButton3State = reading;
}

void handleButton4() {
  static bool toggleCommand4 = false;  // Tracks which command to send
  int reading = digitalRead(BUTTON4_PIN);

  if (reading != lastButton4State) {
    lastDebounceTimeButton4 = millis();  // Reset debounce timer
  }

  if ((millis() - lastDebounceTimeButton4) > DEBOUNCE_DELAY) {
    if (reading != Button4State) {
      Button4State = reading;

      if (Button4State == LOW) {  // Button pressed
        if (isSplit) {
          sendCATCommand("ST0;");  // Send if split mode is ON
        } else {
          sendCATCommand("QS;");   // Send if split mode is OFF
        }
      }
    }
  }
  lastButton4State = reading;
}


void handleButton5() {
  int reading = digitalRead(BUTTON5_PIN);

  if (reading != lastButton5State) {
    lastDebounceTimeButton5 = millis();  // Reset debounce timer
  }

  if ((millis() - lastDebounceTimeButton5) > DEBOUNCE_DELAY) {
    if (reading != Button5State) {
      Button5State = reading;

      if (Button5State == LOW) {  // Button pressed
        // Send zero-in command
        sendCATCommand("SV;");
      }
    }
  }

  lastButton5State = reading;
}

void handleButton6() {
  static bool toggleCommand6 = false;  // Tracks which command to send
  int reading = digitalRead(BUTTON6_PIN);

  if (reading != lastButton6State) {
    lastDebounceTimeButton6 = millis();  // Reset debounce timer
  }

  if ((millis() - lastDebounceTimeButton6) > DEBOUNCE_DELAY) {
    if (reading != Button6State) {
      Button6State = reading;

      if (Button6State == LOW) {  // Button pressed
        if (toggleCommand6) {
          // Send the first command
          sendCATCommand("BC00;");
        } else {
          // Send the second command
          sendCATCommand("BC01;");
        }
        toggleCommand6 = !toggleCommand6;  // Toggle for the next press
      }
    }
  }

  lastButton6State = reading;
}


void handleBandUpButton() {
  int reading = digitalRead(BAND_UP_PIN);

  if (reading != lastBandUpState) {
    lastDebounceTimeBandUp = millis();  // Reset debounce timer
  }

  if ((millis() - lastDebounceTimeBandUp) > DEBOUNCE_DELAY) {
    if (reading != bandUpState) {
      bandUpState = reading;

      if (bandUpState == LOW) {  // Button pressed
        sendCATCommand("BU0;");  // Send "band up" command
      }
    }
  }

  lastBandUpState = reading;
}

void handleBandDownButton() {
  int reading = digitalRead(BAND_DOWN_PIN);

  if (reading != lastBandDownState) {
    lastDebounceTimeBandDown = millis();  // Reset debounce timer
  }

  if ((millis() - lastDebounceTimeBandDown) > DEBOUNCE_DELAY) {
    if (reading != bandDownState) {
      bandDownState = reading;

      if (bandDownState == LOW) {  // Button pressed
        sendCATCommand("BD0;");  // Send "band down" command
      }
    }
  }

  lastBandDownState = reading;
}

void processEncoder1(int direction) {
  if (mode1 == SELECTION) {
    // SELECTION mode: Switch between fields
    // Clear the previous field's value to default color
    if (encoder1Fields[selectedField1] == 7) {
      updateFieldString(encoder1Fields[selectedField1], fieldStringValues[encoder1Fields[selectedField1]], DEFAULT_COLOR);
    } else {
      updateField(encoder1Fields[selectedField1], fieldValues[encoder1Fields[selectedField1]], DEFAULT_COLOR);
    }
    // Update selected field index
    if (direction > 0) { // Clockwise rotation
      selectedField1 = (selectedField1 + 1) % encoder1FieldCount;
    } else { // Counter-clockwise rotation
      selectedField1 = (selectedField1 - 1 + encoder1FieldCount) % encoder1FieldCount;
    }

    // Highlight the new selected field with blue text
    if (encoder1Fields[selectedField1] == 7) {
      updateFieldString(encoder1Fields[selectedField1], fieldStringValues[encoder1Fields[selectedField1]], SELECTION_COLOR);
    } else {
      updateField(encoder1Fields[selectedField1], fieldValues[encoder1Fields[selectedField1]], SELECTION_COLOR);
    }
  } else if (mode1 == VALUE) {
    // VALUE mode: Adjust the value of the selected field
    int fieldIndex = encoder1Fields[selectedField1];
    if (fieldIndex == 0) {
      // Field 0: Increment by 1 within the range 4-60 and send "KS0xx;"
      fieldValues[fieldIndex] += direction;
      fieldValues[fieldIndex] = max(4, min(60, fieldValues[fieldIndex])); // Enforce limits
      char command[8];
      snprintf(command, sizeof(command), "KS0%02d;", fieldValues[fieldIndex]);
      sendCATCommand(command);
    } else if (fieldIndex == 2) {
      // Field 2: Predefined steps within the range 00-33 and send "SDxx;"
      int predefinedValues[] = {30, 50, 100, 150, 200, 250, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500,
                                1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000};
      int predefinedCount = sizeof(predefinedValues) / sizeof(predefinedValues[0]);
      int currentValue = fieldValues[fieldIndex];
      int index = 0;

      for (int i = 0; i < predefinedCount; i++) {
        if (predefinedValues[i] >= currentValue) {
          index = i;
          break;
        }
      }

      index = max(0, min(33, index + direction)); // Enforce range 00-33
      fieldValues[fieldIndex] = predefinedValues[index];
      char command[8];
      snprintf(command, sizeof(command), "SD%02d;", index);
      sendCATCommand(command);
    } else if (fieldIndex == 3) {
      // Field 3: Increment or decrement by 1 and send "ML1xxx;"
      fieldValues[fieldIndex] += direction;
      fieldValues[fieldIndex] = max(0, fieldValues[fieldIndex]); // Ensure no negative values
      char command[8];
      snprintf(command, sizeof(command), "ML1%03d;", fieldValues[fieldIndex]);
      sendCATCommand(command);
    } else if (fieldIndex == 6) {
      // Field 6: Adjust by 10 within the range 300-1050 and send "KPxx;"
      fieldValues[fieldIndex] += direction * 10;
      fieldValues[fieldIndex] = max(300, min(1050, fieldValues[fieldIndex])); // Enforce limits

      // Calculate the corresponding "xx" value
      int kpValue = (fieldValues[fieldIndex] - 300) / 10;
      char command[8];
      snprintf(command, sizeof(command), "KP%02d;", kpValue);
      sendCATCommand(command);
    } else if (fieldIndex == 7) {
      // Field 7: Increment or decrement by 0.5 within -30.0 to +30.0
      String currentStringValue = fieldStringValues[fieldIndex];
      float currentValue = currentStringValue.toFloat();
      currentValue += direction * 0.5;
      currentValue = max(-30.0f, min(30.0f, currentValue)); // Enforce limits

      // Format the value for the LCD without leading zeros
      char displayValue[8];
      snprintf(displayValue, sizeof(displayValue), "%+4.1f", currentValue);

      // Remove leading zero from LCD display (if present)
      if (displayValue[1] == '0') {
        memmove(&displayValue[1], &displayValue[2], strlen(displayValue) - 1);
      }

      // Format the command for the station with leading zeros to ensure exactly 5 characters
      char command[12];
      if (currentValue >= 0) {
        snprintf(command, sizeof(command), "SS04+%04.1f;", currentValue);
      } else {
        snprintf(command, sizeof(command), "SS04-%04.1f;", -currentValue);
      }

      // Update the field value on the LCD
      fieldStringValues[fieldIndex] = String(displayValue);

      // Send the command to the station
      sendCATCommand(command);
    }
    // Update the field value with green color
    if (fieldIndex == 7) {
       updateFieldString(fieldIndex, fieldStringValues[fieldIndex], VALUE_COLOR);
    } else {
       updateField(fieldIndex, fieldValues[fieldIndex], VALUE_COLOR);
    }
  }
}


void processEncoder2(int direction) {
  if (mode2 == SELECTION) {
    // SELECTION mode: Switch between fields
    updateField(encoder2Fields[selectedField2], fieldValues[encoder2Fields[selectedField2]], DEFAULT_COLOR);
    if (direction > 0) { // Clockwise rotation
      selectedField2 = (selectedField2 + 1) % encoder2FieldCount;
    } else { // Counter-clockwise rotation
      selectedField2 = (selectedField2 - 1 + encoder2FieldCount) % encoder2FieldCount;
    }
    updateField(encoder2Fields[selectedField2], fieldValues[encoder2Fields[selectedField2]], SELECTION_COLOR);
  } else if (mode2 == VALUE) {
    int fieldIndex = encoder2Fields[selectedField2];
    if (fieldIndex == 1) {
      // Field 1: Adjust to nearest multiple of 5 within range 5-100
      int currentValue = fieldValues[fieldIndex];
      currentValue += (direction > 0) ? 5 : -5;
      currentValue = max(5, min(100, (currentValue / 5) * 5)); // Enforce limits
      fieldValues[fieldIndex] = currentValue;

      // Send command
      char command[8];
      snprintf(command, sizeof(command), "PC%03d;", currentValue);
      sendCATCommand(command);
    } else if (fieldIndex == 4 || fieldIndex == 5) {
  // Fetch the response from the station
  sendCATCommand("DA;");
  String response = readCATResponse(myTimeout); // Read response

  // Verify the response format
  if (!response.isEmpty() && response.startsWith("DA00")) {
    // Extract the existing values for xx, yy, and zz
    String xxStr = response.substring(4, 6);
    String yyStr = response.substring(6, 8);
    String zzStr = response.substring(8, 10); // Adjusted to capture zz correctly

    int yyValue = yyStr.toInt();
    int zzValue = zzStr.toInt();

    if (fieldIndex == 4) {
      // Increment or decrement yy only
      yyValue += direction;
      yyValue = max(0, min(20, yyValue)); // Ensure yy is within 0-20
      char yy[3];
      snprintf(yy, sizeof(yy), "%02d", yyValue);
      response = "DA00" + xxStr + yy + zzStr + ";"; // Rebuild the response string
    } else if (fieldIndex == 5) {
      // Increment or decrement zz only
      zzValue += direction;
      zzValue = max(0, min(20, zzValue)); // Ensure zz is within 0-20
      char zz[3];
      snprintf(zz, sizeof(zz), "%02d", zzValue);
      response = "DA00" + xxStr + yyStr + zz + ";"; // Rebuild the response string
    }
    // Send the adjusted response back to the station
    sendCATCommand(response.c_str());
  }
}
    updateField(fieldIndex, fieldValues[fieldIndex], VALUE_COLOR);
  }
}




// Function to draw the top info bar
void drawTopBar(const String& frequency, const String& mode, const String& additionalInfo) {
  tft.fillRect(0, 0, SCREEN_WIDTH, TOP_BAR_HEIGHT, BACKGROUND_COLOR);

  // Display frequency in bright blue
  tft.setTextColor(FREQUENCY_COLOR, BACKGROUND_COLOR);
  tft.setTextFont(4);
  int textX = 10;  // Starting X position
  tft.setCursor(textX, (TOP_BAR_HEIGHT - 16) / 2);
  tft.print(frequency);

  // Display mode in yellow
  tft.setTextColor(MODE_COLOR, BACKGROUND_COLOR);
  textX += tft.textWidth(frequency, 4) + 10;  // Adjust position after frequency
  tft.setCursor(textX, (TOP_BAR_HEIGHT - 16) / 2);
  tft.print(mode);

  // Display additional information in red
  if (!additionalInfo.isEmpty()) {
    tft.setTextColor(SPLIT_COLOR, BACKGROUND_COLOR);
    textX += tft.textWidth(mode, 4) + 10;  // Adjust position after mode
    tft.setCursor(textX, (TOP_BAR_HEIGHT - 16) / 2);
    tft.print(additionalInfo);
  }
}


// Function to send a CAT command
void sendCATCommand(const char* command) {
  Serial2.print(command);
  Serial2.print("\r");  // Append carriage return as per CAT protocol
}

// Function to read a response from the radio
String readCATResponse(unsigned long timeout = 50) {
    String response = "";
    unsigned long startTime = millis();
    while (millis() - startTime < timeout) {
        if (Serial2.available()) {
            char c = Serial2.read();
            if (c == '\r') break;  // End of response
            response += c;
        }
    }
    return response;
}


// Function to process top bar information
void processTopBarInfo() {
  // Static variables to store the last displayed values
  static String lastMode = "";
  static String lastAdditionalInfo = "";

  // Variables for current values to compare with last displayed
  String formattedFrequency = "";
  String mode = "";
  String additionalInfo = "";

  // Send and process the "ST;" command for split status
  sendCATCommand("ST;");
  String splitResponse = readCATResponse(myTimeout);
  isSplit = false;

  if (!splitResponse.isEmpty() && splitResponse.length() >= 3) {
    char splitStatus = splitResponse.charAt(2);  // Extract the status value
    isSplit = (splitStatus == '1' || splitStatus == '2');
  }

  // Send and process the "IF;" command for main frequency and info
  sendCATCommand("IF;");
  String ifResponse = readCATResponse(myTimeout);

  if (!ifResponse.isEmpty()) {
    // Extract main frequency
    String rawFrequency = ifResponse.substring(6, 14);
    formattedFrequency = formatFrequency(rawFrequency);

    // Extract and map the mode character (22nd position, index 21)
    char modeChar = ifResponse.charAt(21);
    mode = getHAMMode(modeChar);

    if (isSplit) {
      // If SPLIT is active, fetch VFO B frequency
      additionalInfo = "SPLIT ";
      sendCATCommand("FB;");
      String fbResponse = readCATResponse(myTimeout);

      if (!fbResponse.isEmpty() && fbResponse.length() >= 10) {
        String rawVfoBFrequency = fbResponse.substring(3, 11);
        String formattedVfoB = formatFrequency(rawVfoBFrequency);
        additionalInfo += formattedVfoB;
      }
    } else {
      // If not in SPLIT mode, handle RX CLAR and TX CLAR
      char rxClarChar = ifResponse.charAt(19);  // 19th char
      char txClarChar = ifResponse.charAt(20);  // 20th char

      if (rxClarChar == '1' || txClarChar == '1') additionalInfo = "CLAR ";
      if (rxClarChar == '1') additionalInfo += "RX";
      if (txClarChar == '1') additionalInfo += "TX";
    }

    // Update display only if values have changed
    if (formattedFrequency != lastFormattedFrequency || mode != lastMode || additionalInfo != lastAdditionalInfo) {
      drawTopBar(formattedFrequency, mode, additionalInfo);
      lastFormattedFrequency = formattedFrequency;
      lastMode = mode;
      lastAdditionalInfo = additionalInfo;
    }
  }
}

// Helper function to format frequency with dots
String formatFrequency(String rawFrequency) {
  String formattedFrequency = "";
  int counter = 0;

  // Original logic to format the frequency with dots
  for (int i = rawFrequency.length() - 1; i >= 0; i--) {
    formattedFrequency = rawFrequency[i] + formattedFrequency;
    counter++;
    if (counter % 3 == 0 && i != 0) {
      formattedFrequency = "." + formattedFrequency;
    }
  }

  // Replace leading zeros with spaces
  for (int i = 0; i < formattedFrequency.length(); i++) {
    if (formattedFrequency[i] == '0') {
      formattedFrequency[i] = ' ';  // Replace '0' with space
    } else if (formattedFrequency[i] != '.') {
      break;  // Stop replacing when a non-zero digit is encountered
    }
  }

  return formattedFrequency;
}

// Function to map a character to HAM mode
String getHAMMode(char modeChar) {
  switch (modeChar) {
    case '1': return "LSB";
    case '2': return "USB";
    case '3': return "CW-U";
    case '4': return "FM";
    case '5': return "AM";
    case '6': return "RTTY-L";
    case '7': return "CW-L";
    case '8': return "DATA-L";
    case '9': return "RTTY-U";
    case 'A': return "DATA-FM";
    case 'B': return "FM-N";
    case 'C': return "DATA-U";
    case 'D': return "AM-N";
    case 'E': return "PSK";
    case 'F': return "DATA-FM-N";
    default: return "Unknown";
  }
}

// Function to process all CAT responses during setup
void processCATResponses() {
  processTopBarInfo();
  sendAndParseCATCommand();
}

void sendAndParseCATCommand() {
  // Send all CAT commands in one go
  sendCATCommand("KS;PC;ML1;DA;SS04;SD;KP;KR;BC0;");
  String response = readCATResponse(myTimeout);  // Receive combined response

  // Step 1: Split the response into individual commands
  const int MAX_COMMANDS = 10;  // Maximum expected number of responses
  String responses[MAX_COMMANDS];
  int responseCount = 0;

  while (response.length() > 0 && responseCount < MAX_COMMANDS) {
    int delimiterIndex = response.indexOf(';');
    if (delimiterIndex != -1) {
      responses[responseCount++] = response.substring(0, delimiterIndex + 1);
      response = response.substring(delimiterIndex + 1);
    } else {
      responses[responseCount++] = response;
      response = "";
    }
  }

  // Step 2: Process each command separately
  for (int i = 0; i < responseCount; i++) {
    String currentResponse = responses[i];

    if (currentResponse.startsWith("KS")) {
      // Parse KS response (e.g., KS0xx;)
      String valueStr = currentResponse.substring(3, currentResponse.length() - 1);
      int value = valueStr.toInt();
      if (fieldValues[0] != value) {
        fieldValues[0] = value;
        if (selectedField1 == 0) {
          updateField(0, value, VALUE_COLOR); 
        }
        else {
          updateField(0, value, DEFAULT_COLOR); 
        }
      }
    } else if (currentResponse.startsWith("PC")) {
      // Parse PC response (e.g., PCxxx;)
      String valueStr = currentResponse.substring(2, currentResponse.length() - 1);
      int value = valueStr.toInt();
      if (fieldValues[1] != value) {
        fieldValues[1] = value;
        if (selectedField2 == 0) {
          updateField(1, value, VALUE_COLOR); 
        }
        else {
          updateField(1, value, DEFAULT_COLOR); 
        }
      }
    } else if (currentResponse.startsWith("ML1")) {
      // Parse ML1 response (e.g., ML1xxyyy;)
      String yyy = currentResponse.substring(4, currentResponse.length() - 1);
      int value = yyy.toInt();
      if (fieldValues[3] != value) {
        fieldValues[3] = value;
        if (selectedField1 == 2) {
          updateField(3, value, VALUE_COLOR); 
        }
        else {
          updateField(3, value, DEFAULT_COLOR); 
        }
      }
    } else if (currentResponse.startsWith("DA")) { 
        // Parse DA response (e.g., DA00xxyyzz;)
        String yyStr = currentResponse.substring(6, 8);
        int yyValue = yyStr.toInt();
        if (yyValue >= 0 && yyValue <= 20) {
          if (fieldValues[4] != yyValue) {
            fieldValues[4] = yyValue;
            if (selectedField2 == 1) {
              updateField(4, yyValue, VALUE_COLOR); 
            } else {
              updateField(4, yyValue, DEFAULT_COLOR); 
            }
          }
        }
      
        String zzStr = currentResponse.substring(8, 10);
        int zzValue = zzStr.toInt();
        if (zzValue >= 0 && zzValue <= 100) {  // Adjust range as needed
          if (fieldValues[5] != zzValue) {
            fieldValues[5] = zzValue;
            if (selectedField2 == 2) {
              updateField(5, zzValue, VALUE_COLOR); 
            } else {
              updateField(5, zzValue, DEFAULT_COLOR); 
            }
          }
        }
      }
     else if (currentResponse.startsWith("SS04")) {
  // Parse SS04 response (e.g., SS04+05.5;)
  String valueStr = currentResponse.substring(4, currentResponse.length() - 1);
  
  // Strip leading zero after the sign (+ or -)
  if (valueStr.charAt(1) == '0') {
    valueStr.remove(1, 1); // Remove the '0' at position 1
  }

  if (fieldStringValues[7] != valueStr) {  // Compare with current value
    fieldStringValues[7] = valueStr;       // Update fieldStringValues
        if (selectedField2 == 2) {
          updateFieldString(7, valueStr, VALUE_COLOR); 
        }
        else {
          updateFieldString(7, valueStr, DEFAULT_COLOR); 
        }
  }
} else if (currentResponse.startsWith("SD")) {
      // Parse SD response (e.g., SDxx;)
      String valueStr = currentResponse.substring(2, currentResponse.length() - 1);
      int index = valueStr.toInt(); // Extract and convert xx to integer
      int mappedValue;

      // Map SDxx to corresponding value
      if (index >= 0 && index <= 6) {
        int fixedValues[] = {30, 50, 100, 150, 200, 250, 300};
        mappedValue = fixedValues[index];
      } else if (index >= 7 && index <= 33) {
        mappedValue = 400 + (index - 7) * 100;  // Linear mapping from 400 to 3000
      } else {
        continue;  // Invalid response; skip
      }

      // Update the field only if the value is different
      if (fieldValues[2] != mappedValue) {
        fieldValues[2] = mappedValue;
        if (selectedField1 == 1) {
          updateField(2, mappedValue, VALUE_COLOR); 
        }
        else {
          updateField(2, mappedValue, DEFAULT_COLOR); 
        }
      }
    } else if (currentResponse.startsWith("KP")) {
      // Parse KP response (e.g., KPxx;)
      String valueStr = currentResponse.substring(2, currentResponse.length() - 1);
      int index = valueStr.toInt();

      // Map KPxx to a value between 300 and 1050 in steps of 10
      int mappedValue = 300 + (index * 10);

      // Update field 6 only if the value has changed
      if (fieldValues[6] != mappedValue) {
        fieldValues[6] = mappedValue;
        if (selectedField1 == 3) {
          updateField(6, mappedValue, VALUE_COLOR); 
        }
        else {
          updateField(6, mappedValue, DEFAULT_COLOR); 
        }
      }
    } else if (currentResponse.startsWith("KR")) {
      // Parse KR response (e.g., KR0; or KR1;)
      char state = currentResponse.charAt(2); // Extract the '0' or '1'
      String stateStr = (state == '1') ? "ON" : "OFF";

      if (fieldStringValues[8] != stateStr) {
        fieldStringValues[8] = stateStr;
       if (selectedField2 == 3) {
          updateFieldString(8, stateStr, VALUE_COLOR); 
        }
        else {
          updateFieldString(8, stateStr, DEFAULT_COLOR); 
        }
        //updateFieldString(8, stateStr, VALUE_COLOR);
      }
    } else if (currentResponse.startsWith("BC0")) {
      // Parse BC0 response (e.g., BC00; or BC01;)
      char state = currentResponse.charAt(3); // Extract the '0' or '1'
      String stateStr = (state == '1') ? "ON" : "OFF";

      if (fieldStringValues[9] != stateStr) {
        fieldStringValues[9] = stateStr;
        if (selectedField2 == 4) {
          updateFieldString(9, stateStr, VALUE_COLOR); 
        }
        else {
          updateFieldString(9, stateStr, DEFAULT_COLOR); 
        }
      }
    }
  }
}

// Remaining functions (e.g., handle encoders/buttons) remain the same
// ...

// Function to draw all borders and field names
void drawBorders() {
  int topOffset = 3; // Offset for top fields to avoid clipping

  // Top fields (2 fields)
  for (int i = 0; i < 2; i++) {
    int x = (i == 0) ? 0 : SCREEN_WIDTH / 2;
    int y = TOP_BAR_HEIGHT + topOffset; // Add offset to y-coordinate
    tft.drawRect(x, y, SCREEN_WIDTH / 2, TOP_HEIGHT, BORDER_COLOR);
    tft.drawRect(x + 2, y + 2, SCREEN_WIDTH / 2 - 4, TOP_HEIGHT - 4, BORDER_COLOR);

    tft.setTextColor(NAME_COLOR, BACKGROUND_COLOR);
    tft.setTextFont(2);
    int nameWidth = tft.textWidth(fieldNames[i]);
    int nameX = x + (SCREEN_WIDTH / 4) - (nameWidth / 2);
    tft.setCursor(nameX, y + 10);
    tft.print(fieldNames[i]);
  }

  // Bottom fields (8 fields in 2 rows)
  int fieldWidth = SCREEN_WIDTH / 4;
  int fieldHeight = BOTTOM_HEIGHT / 2;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j++) {
      int fieldIndex = 2 + j * 4 + i;
      int x = i * fieldWidth;
      int y = TOP_BAR_HEIGHT + TOP_HEIGHT + j * fieldHeight;

      tft.drawRect(x, y, fieldWidth, fieldHeight, BORDER_COLOR);
      tft.drawRect(x + 2, y + 2, fieldWidth - 4, fieldHeight - 4, BORDER_COLOR);

      tft.setTextColor(NAME_COLOR, BACKGROUND_COLOR);
      tft.setTextFont(2);
      int nameWidth = tft.textWidth(fieldNames[fieldIndex]);
      int nameX = x + (fieldWidth - nameWidth) / 2;
      tft.setCursor(nameX, y + 10);
      tft.print(fieldNames[fieldIndex]);
    }
  }
}


// Function to update all fields
void updateAllFields() {
  for (int i = 0; i < 10; i++) {
    if (fieldValues[i] != lastFieldValues[i]) {
      updateField(i, fieldValues[i], DEFAULT_COLOR);
      lastFieldValues[i] = fieldValues[i];
    }
  }
}

// Function to update a specific field
void updateField(int fieldIndex, int value, uint16_t color) {
  int fieldWidth, fieldHeight, x, y;
  bool isTopField = (fieldIndex < 2);

  if (isTopField) {
    fieldWidth = SCREEN_WIDTH / 2;
    fieldHeight = TOP_HEIGHT;
    x = (fieldIndex == 0) ? 0 : SCREEN_WIDTH / 2;
    y = TOP_BAR_HEIGHT;
  } else {
    fieldIndex -= 2;
    fieldWidth = SCREEN_WIDTH / 4;
    fieldHeight = BOTTOM_HEIGHT / 2;
    int col = fieldIndex % 4;
    int row = fieldIndex / 4;
    x = col * fieldWidth;
    y = TOP_BAR_HEIGHT + TOP_HEIGHT + row * fieldHeight;
  }

  tft.fillRect(x + 3, y + 30, fieldWidth - 6, fieldHeight - 35, BACKGROUND_COLOR);
  tft.setTextFont(isTopField ? 6 : 4);
  tft.setTextColor(color, BACKGROUND_COLOR);

  char valueBuffer[8];
  snprintf(valueBuffer, sizeof(valueBuffer), "%d", value);
  int valueWidth = tft.textWidth(valueBuffer, isTopField ? 6 : 4);
  int textX = x + (fieldWidth - valueWidth) / 2;
  int textY = y + (fieldHeight - 8 * (isTopField ? 6 : 4)) / 2 + (isTopField ? 20 : 15);

  tft.setCursor(textX, textY);
  tft.print(valueBuffer);
}



// Function to handle button 1 press with debounce
void handleButton1() {
  int reading = digitalRead(BUTTON1_PIN);

  if (reading != lastButtonState1) {
    lastDebounceTime1 = millis();  // Reset debounce timer
  }

  if ((millis() - lastDebounceTime1) > DEBOUNCE_DELAY) {
    if (reading != buttonState1) {
      buttonState1 = reading;

      if (buttonState1 == LOW) {  // Button pressed
        mode1 = (mode1 == SELECTION) ? VALUE : SELECTION;  // Toggle between modes
        if (mode1 == SELECTION) {
          // Highlight the current field in blue text during selection
          if (encoder1Fields[selectedField1] == 7) {
            updateFieldString(encoder1Fields[selectedField1], fieldStringValues[encoder1Fields[selectedField1]], SELECTION_COLOR);
          } else {
            updateField(encoder1Fields[selectedField1], fieldValues[encoder1Fields[selectedField1]], SELECTION_COLOR);
          }
        } else {
          if (encoder1Fields[selectedField1] == 7) {
            updateFieldString(encoder1Fields[selectedField1], fieldStringValues[encoder1Fields[selectedField1]], VALUE_COLOR);
          } else {
          // During value mode, ensure the field text is green
            updateField(encoder1Fields[selectedField1], fieldValues[encoder1Fields[selectedField1]], VALUE_COLOR);
          }
        }
      }
    }
  }

  lastButtonState1 = reading;
}

// Function to handle button 2 press with debounce
void handleButton2() {
  int reading = digitalRead(BUTTON2_PIN);

  if (reading != lastButtonState2) {
    lastDebounceTime2 = millis();  // Reset debounce timer
  }

  if ((millis() - lastDebounceTime2) > DEBOUNCE_DELAY) {
    if (reading != buttonState2) {
      buttonState2 = reading;

      if (buttonState2 == LOW) {  // Button pressed
        mode2 = (mode2 == SELECTION) ? VALUE : SELECTION;  // Toggle between modes
        if (mode2 == SELECTION) {
          // Highlight the current field in blue text during selection
          updateField(encoder2Fields[selectedField2], fieldValues[encoder2Fields[selectedField2]], SELECTION_COLOR);
        } else {
          // During value mode, ensure the field text is green
          updateField(encoder2Fields[selectedField2], fieldValues[encoder2Fields[selectedField2]], VALUE_COLOR);
        }
      }
    }
  }

  lastButtonState2 = reading;
}
