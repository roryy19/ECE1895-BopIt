const char correctCode[] = "1895"; // code to input
const int inputLength = sizeof(correctCode) - 1; // -1 for null terminator

char inputCode[inputLength];
int inputIndex = 0;

// Define row and column pins
const int rowPins[4] = {5, 10, 9, 7};  // R1, R2, R3, R4
const int colPins[3] = {6, 4, 8};      // C1, C2, C3

// Define key mappings for a standard 4x3 keypad
char keys[4][3] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};



void setup() {
  Serial.begin(9600);

  // Set column pins as OUTPUTS
  for (int c = 0; c < 3; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c], HIGH); // Default HIGH
  }

  // Set row pins as INPUTS with pull-up resistors
  for (int r = 0; r < 4; r++) {
    pinMode(rowPins[r], INPUT_PULLUP);
  }

  Serial.println("Enter 1895 on the keypad:");
}

void loop() {
  char key = scanKeypad();

  if (key) {  // If a key is detected
    Serial.print("Key Pressed: ");
    Serial.println(key);

    inputCode[inputIndex] = key;
    inputIndex++;

    if (inputIndex == inputLength) {
      inputCode[inputIndex] = '\0';
      if (strcmp(inputCode, correctCode) == 0) {
        Serial.println("Correct Code!");
      } else {
        Serial.println("Wrong Code.");
      }
      inputIndex = 0;
    }
  }
}

// Function to scan the keypad
char scanKeypad() {
  for (int c = 0; c < 3; c++) {
    digitalWrite(colPins[c], LOW); // Activate this column

    for (int r = 0; r < 4; r++) {
      if (digitalRead(rowPins[r]) == LOW) { // If a key is pressed
        delay(500); // Debounce
        while (digitalRead(rowPins[r]) == LOW); // Wait for release
        digitalWrite(colPins[c], HIGH); // Reset column

        return keys[r][c]; // Return the key pressed
      }
    }

    digitalWrite(colPins[c], HIGH); // Reset column
  }
  
  return 0; // No key pressed
}
