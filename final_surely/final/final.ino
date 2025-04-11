#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// Pin definitions
const int KEY_PIN = 10; // PB2 -> physical pin 16 on ATmega
const int BUTTON_PIN = 9; // PB1 --> physical pin 15 on the ATmega

// ATmega328P: pin 26 = D4 (TX), use D5 for RX
// SoftwareSerial dfSerial(5, 4); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// set LCD address to 0x27 for 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// For button interrupt
volatile bool buttonPressed = false;
volatile bool keyTwisted = false;
volatile bool keyPressed = false;
volatile unsigned long lastInterruptTime = 0;
volatile unsigned long lastButtonInterrupt = 0;
volatile unsigned long lastKeyInterrupt = 0;
const unsigned long buttonDebounceDelay = 1000; // ms
const unsigned long keyDebounceDelay = 2500;



// Game states
enum GameState {
  IDLE,
  WAITING_FOR_BUTTON,
  WAITING_FOR_KEY_TWIST,
  WAITING_FOR_KEYPAD,
  GAME_OVER
};

// Global variables
GameState gameState = IDLE;
int score = 0;

// We’ll give the user some time to press the button each round
unsigned long roundDeadline = 0;
unsigned long roundTimeMs   = 30000; // 3 seconds, for example
unsigned long roundDecMs    = 250;

const char correctCode[] = "1895"; // code to input
const int inputLength = sizeof(correctCode) - 1; // -1 for null terminator

char inputCode[inputLength];
int inputIndex = 0;

// Define row and column pins
const int rowPins[4] = {2, 3, 4, 5};  // R1, R2, R3, R4 --> PD2, PD3, PD4, PD5
const int colPins[3] = {6, 7, 8};      // C1, C2, C3 --> PD6, PD7, PB0

// Define key mappings for a standard 4x3 keypad
char keys[4][3] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

void handleButtonISR();
void handleKeyISR();

// ------------------------------------
// SETUP
// ------------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(KEY_PIN, INPUT_PULLUP);

  // setup pin change interrupts for pb1 (d9) and pb2 (d10)
  PCICR |= (1 << PCIE0); // Enable PCINT for PORTB
  PCMSK0 |= (1 << PCINT1); // Enable PCINT1 for PB1 (D9)
  PCMSK0 |= (1 << PCINT2); // Enable PCINT2 for PB2 (D10)

  // Attach interrupt for button and key
  //attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonISR, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(KEY_PIN), handleKeyISR, CHANGE);

  // Set column pins as OUTPUTS
  for (int c = 0; c < 3; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c], HIGH); // Default HIGH
  }

  // Set row pins as INPUTS with pull-up resistors
  for (int r = 0; r < 4; r++) {
    pinMode(rowPins[r], INPUT_PULLUP);
  }

  Serial.println("Setup complete.");
}

// ------------------------------------
// PIN CHANGE INTERRUPT HANDLER (PB1 = D9, PB2 = D10)
// ------------------------------------
ISR(PCINT0_vect) {
  unsigned long now = millis();

  // Check PB1 (D9) - BUTTON
  if (!(PINB & (1 << PB1))) {
    if (now - lastButtonInterrupt > buttonDebounceDelay) {
      buttonPressed = true;
      lastButtonInterrupt = now;
    }
  }

  // Check PB2 (D10) - KEY
  if (!(PINB & (1 << PB2))) {
    if (now - lastKeyInterrupt > keyDebounceDelay) {
      keyTwisted = true;
      lastKeyInterrupt = now;
    }
  }
}

// ------------------------------------
// LOOP
// ------------------------------------
void loop() {
  // Check power switch
  bool powerIsOn = true; //(digitalRead(POWER_PIN) == LOW);
  if (!powerIsOn) {
    // Power off => reset to IDLE
    if (gameState != IDLE) {
      Serial.println("Power turned off => Game resetting to IDLE.");
      gameState = IDLE;
      score = 0; // reset score
    }
  }
  char key = scanKeypad();

  switch (gameState) {
    case IDLE:
      if (powerIsOn && buttonPressed) {
        // Start a new game
        noInterrupts();
        buttonPressed = false;
        interrupts();

        score = 0;
        Serial.println("Power ON => Starting game.");
        // Start the first round
        roundDeadline = millis() + roundTimeMs;
        gameState = getNewState();
      }
      break;

    case WAITING_FOR_BUTTON:
      // Check if time is up
      if (millis() > roundDeadline) {
        // Timed out => game over
        Serial.println("TIMEOUT => Game Over. Score reset to 0.");
        displayGameOver();
        score = 0;
        gameState = GAME_OVER;
      }
      break;

    case WAITING_FOR_KEY_TWIST:
      // Check if time is up
      if (millis() > roundDeadline) {
        // Timed out => game over
        Serial.println("TIMEOUT => Game Over. Score reset to 0.");
        displayGameOver();
        score = 0;
        gameState = GAME_OVER;
      }
      break;

    case WAITING_FOR_KEYPAD:
      // Check if time is up
      if (millis() > roundDeadline) {
        // Timed out => game over
        Serial.println("TIMEOUT => Game Over. Score reset to 0.");
        displayGameOver();
        score = 0;
        gameState = GAME_OVER;
      }
      break;

    case GAME_OVER:
      // Wait for power off or something else to reset the game
      // We’ll just do nothing here. If user turns off power => IDLE.
      break;
  }

  // button logic
  if (buttonPressed) {
    noInterrupts();
    buttonPressed = false;
    interrupts();

    // Evaluate the press based on the current game state
    if (gameState == WAITING_FOR_BUTTON) {
      // It's correct
      score++;
      myDFPlayer.volume(20);
      myDFPlayer.play(1); // explosion sound
      Serial.print("Correct button press! Score = ");
      Serial.println(score);
      displayScore();

      // Start next round
      if (score < 10) {
        roundTimeMs -= roundDecMs;
      }
      else {
        Serial.println("Congratulations! You Win! :)");
        displayVictory();
        score = 0;
        gameState = GAME_OVER;
        return;
      }
      roundDeadline = millis() + roundTimeMs;
      gameState = getNewState();
    } else {
      // If they pressed the button in IDLE, different command state, or GAME_OVER, that’s “wrong time”
      // => game over, score = 0
      Serial.println("Button pressed at wrong time => Game Over, score = 0.");
      displayGameOver();
      score = 0;
      gameState = GAME_OVER;
    }
  }

  // key twist logic
  if (keyTwisted) {
    noInterrupts();
    keyTwisted = false;
    interrupts();

    // Evaluate the press based on the current game state
    if (gameState == WAITING_FOR_KEY_TWIST) {
      // It's correct
      score++;
      Serial.print("Correct key twist! Score = ");
      Serial.println(score);
      displayScore();

      // Start next round
      if (score < 10) {
        roundTimeMs -= roundDecMs;
      }
      else {
        Serial.println("Congratulations! You Win! :)");
        displayVictory();
        score = 0;
        gameState = GAME_OVER;
        return;
      }
      roundDeadline = millis() + roundTimeMs;
      gameState = getNewState();
    } else {
      // If they pressed the button in IDLE, different command state, or GAME_OVER, that’s “wrong time”
      // => game over, score = 0
      Serial.println("Key twisted at wrong time => Game Over, score = 0.");
      displayGameOver();
      score = 0;
      gameState = GAME_OVER;
    }
  }

  // keypad logic
  if (key) {  // If a key is detected
    if (gameState == WAITING_FOR_KEYPAD) {
        Serial.print("Key Pressed: ");
        Serial.println(key);

        inputCode[inputIndex] = key;
        inputIndex++;

        if (inputIndex == inputLength) {  // Only check after 4 key presses
            inputCode[inputIndex] = '\0';  // Null terminate the string

            if (strcmp(inputCode, correctCode) == 0) {
                score++;
                Serial.print("Correct Code! Score = ");
                Serial.println(score);
                displayScore();
            } else {
                Serial.println("Wrong Code. => Game Over, score = 0");
                displayGameOver();
                score = 0;
                gameState = GAME_OVER;
            }

            inputIndex = 0;  // Reset input storage only after checking
            if (gameState != GAME_OVER) {  // If the game is still running, start the next round
                if (score < 10) {
                    roundTimeMs -= roundDecMs;
                } else {
                    Serial.println("Congratulations! You Win! :)");
                    displayVictory();
                    score = 0;
                    gameState = GAME_OVER;
                    return;
                }
                roundDeadline = millis() + roundTimeMs;
                gameState = getNewState();
            }
        }
    } else {  // Keypad pressed at the wrong time
        Serial.println("Keypad used at wrong time => Game Over, score = 0.");
        displayGameOver();
        score = 0;
        gameState = GAME_OVER;
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
 
// picking new state after each round
GameState getNewState() {
  //srand(time(NULL));

  // Generates a number 
  int n = 100;
  int rnd = rand() % (n + 1);

  // button state
  if (rnd <= 33) {
    Serial.print("Next round! You have ");
    Serial.print(roundTimeMs / 1000.0);
    Serial.println(" seconds left. PRESS THE BUTTON!");
    myDFPlayer.volume(20);
    myDFPlayer.play(4); // nuke it
    return WAITING_FOR_BUTTON;
  } 
  // keypad state
  else if (rnd > 33 && rnd <= 67) {
    Serial.print("Next round! You have ");
    Serial.print(roundTimeMs / 1000.0);
    Serial.println(" seconds left. ENTER 1895 ON THE KEYPAD!");
    myDFPlayer.volume(20);
    myDFPlayer.play(3); // code it 
    displayKeypadCode();
    return WAITING_FOR_KEYPAD;
  }
  // key twist state
  else {
    Serial.print("Next round! You have ");
    Serial.print(roundTimeMs / 1000.0);
    Serial.println(" seconds left. TWIST THE KEY!");
    myDFPlayer.volume(20);
    myDFPlayer.play(2); // twist it
    return WAITING_FOR_KEY_TWIST;
  }
}

void displayScore() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  lcd.print(score);
}

void displayKeypadCode() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  lcd.print(score);
  lcd.setCursor(0, 1);
  lcd.print("Enter 1895");
}

void displayGameOver() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Game Over");
  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(score);
}

void displayVictory() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("You Win :)");
  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(score);
}

// ISR for button press (debounce)
void handleButtonISR() {
  unsigned long now = millis();
  if (now - lastInterruptTime > buttonDebounceDelay) {
    buttonPressed = true;
    lastInterruptTime = now;
  }
}

// ISR for key twist (debounce)
void handleKeyISR() {
  unsigned long now = millis();
  if (now - lastInterruptTime > keyDebounceDelay) {
    keyTwisted = true;
    lastInterruptTime = now;
  }
}