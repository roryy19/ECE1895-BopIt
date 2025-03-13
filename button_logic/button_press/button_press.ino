const int buttonPin = 3;  // Button connected to digital pin 3 (INT1)
volatile bool buttonPressed = false;  // Flag for button press event
volatile unsigned long lastInterruptTime = 0;  // Last time an interrupt fired
const unsigned long debounceDelay = 50;  // Debounce time (ms)

bool lastButtonState = HIGH;  // Last known button state (HIGH = released)
bool lastPrintedState = HIGH; // Last printed state (HIGH = last message was OFF)

void setup() {
  Serial.begin(9600);
  
  pinMode(buttonPin, INPUT_PULLUP);  // Use internal pull-up resistor
  
  // Attach Interrupt to pin 3 (INT1), triggers on state change
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButton, CHANGE);
}

void loop() {
  // If the button was pressed (flag set by ISR)
  if (buttonPressed) {
    noInterrupts();  // Temporarily disable interrupts to prevent race conditions
    int currentState = digitalRead(buttonPin);
    buttonPressed = false;  // Reset flag
    interrupts();  // Re-enable interrupts

    // If button is pressed, print "ON"
    if (currentState == LOW) {
      Serial.println("Button is turned ON");
      lastPrintedState = LOW; // Track that we printed "ON"
    }
    
    lastButtonState = currentState; // Store the last button state
  }

  // Outside the buttonPressed condition:
  // If the last printed message was ON and the button is now released, print OFF
  if (lastPrintedState == LOW && digitalRead(buttonPin) == HIGH) {
    Serial.println("Button is turned OFF");
    lastPrintedState = HIGH; // Track that we printed "OFF"
  }
}

// Interrupt Service Routine (ISR) with debounce
void handleButton() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > debounceDelay) {  // Debounce check
    buttonPressed = true;  // Set flag for the main loop to process
    lastInterruptTime = interruptTime;
  }
}
