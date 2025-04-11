/*******************************************************
 * STEP 1: Button Only
 * 
 * - States:
 *    IDLE (waiting for power on)
 *    WAITING_FOR_BUTTON (user must press the button)
 *    GAME_OVER
 *
 * - If user presses button correctly (while WAITING_FOR_BUTTON),
 *   they score a point and go to the next round.
 * - If user presses button at the wrong time or misses the
 *   time window, GAME_OVER => score = 0
 *******************************************************/

#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// Pin definitions
const int BUTTON_PIN = 2; // pb 1
const int KEY_PIN = 3; // pb 2
const int POWER_PIN  = 7;
const int SPEAKER_PIN = 8; // not used much here, just a placeholder
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

//const int rowPins[4] = {5, 10, 9, 7}; // R1, R2, R3, R4
//const int colPins[3] = {6, 4, 8};     // C1, C2, C3

const int rowPins[4] = {8, 4, 6, 7}; // R1, R2, R3, R4
const int colPins[3] = {9, 10, 5};     // C1, C2, C3

// Keypad map
char keys[4][3] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

// Keypad code logic
const char correctCode[] = "1895";
const int inputLength = sizeof(correctCode) - 1;
char inputCode[inputLength];
int inputIndex = 0;

// For button interrupt
volatile bool buttonPressed = false;
volatile bool keyTwisted = false;
volatile bool keyPressed = false;

volatile unsigned long lastInterruptTime = 0;
volatile unsigned long lastButtonInterrupt = 0;
volatile unsigned long lastKeyInterrupt = 0;

const unsigned long buttonDebounceDelay = 1000; // ms
const unsigned long keyDebounceDelay = 2500;

volatile bool lastButtonState = HIGH;
volatile bool lastKeyState = HIGH;

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

void handleButtonISR();
void handleKeyISR();

// ------------------------------------
// SETUP
// ------------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(KEY_PIN, INPUT_PULLUP);
  pinMode(POWER_PIN, INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(KEY_PIN), handleKeyISR, FALLING);

  // Attach interrupt for button
  //attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonISR, CHANGE);
   // setup pin change interrupts for pb1 (d9) and pb2 (d10)
  /*
  PCICR |= (1 << PCIE0); // Enable PCINT for PORTB
  PCMSK0 |= (1 << PCINT1); // Enable PCINT1 for PB1 (D9)
  PCMSK0 |= (1 << PCINT2); // Enable PCINT2 for PB2 (D10)
*/

  // Set column pins as OUTPUTS
  for (int c = 0; c < 3; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c], HIGH); // Default HIGH
  }

  // Set row pins as INPUTS with pull-up resistors
  for (int r = 0; r < 4; r++) {
    pinMode(rowPins[r], INPUT_PULLUP);
  }

  // initialize the LCD
	lcd.begin();
  // Turn on the blacklight and print a message.
	lcd.backlight();
	displayScore();

  Serial.println("Step 1: Button Only - Setup complete.");
  srand(analogRead(A0));
}

void testLoop() {
  char k = scanKeypad();
  if (k) {
    Serial.print("Key: "); Serial.println(k);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Key: ");
    lcd.print(k);
    delay(300); // debounce
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
      if (powerIsOn) {
        // Start a new game
        score = 0;
        Serial.println("Power ON => Starting game. Round 1: Press the button!");
        // Start the first round
        roundDeadline = millis() + roundTimeMs;
        gameState = getNewState();
        //buttonPressed = false;
        //keyTwisted = false;

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

      // Also check if button was pressed at the wrong time 
      // (i.e., user pressed it outside of WAITING_FOR_BUTTON state).
      // Actually, in this simplified version, pressing the button
      // during WAITING_FOR_BUTTON is always correct. So no "wrong time" check here.
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
        score = 0;
        gameState = GAME_OVER;
      }
      break;

    case GAME_OVER:
      // Wait for power off or something else to reset the game
      // We’ll just do nothing here. If user turns off power => IDLE.
      break;
  }

  // ---------- BUTTON ----------
  if (buttonPressed) {
    noInterrupts();
    buttonPressed = false;
    interrupts();

    // Evaluate the press based on the current game state
    if (gameState == WAITING_FOR_BUTTON) {
      // It's correct
      score++;
      //myDFPlayer.volume(20);
      //myDFPlayer.play(1); // explosion sound
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
        return;
      }
      roundDeadline = millis() + roundTimeMs;
      gameState = getNewState();
    } else {
      // If they pressed the button in IDLE, different command state, or GAME_OVER, that’s “wrong time”
      // => game over, score = 0
      Serial.println("Button pressed at wrong time => Game Over, score = 0.");
      displayGameOver();
      gameState = GAME_OVER;
    }
  }

  // ---------- KEY TWIST ----------
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
        return;
      }
      roundDeadline = millis() + roundTimeMs;
      gameState = getNewState();
    } else {
      // If they pressed the button in IDLE, different command state, or GAME_OVER, that’s “wrong time”
      // => game over, score = 0
      Serial.println("Key twisted at wrong time => Game Over, score = 0.");
      displayGameOver();
      gameState = GAME_OVER;
    }
  }
  // ---------- KEYPAD ----------
  if (key) {  // If a key is detected
    if (gameState == WAITING_FOR_KEYPAD) {
        Serial.print("Key Pressed: ");
        Serial.println(key);

        inputCode[inputIndex] = key;
        inputIndex++;

        // Display the current partial code on line 2
        lcd.setCursor(0, 1);
        lcd.print("                "); // Clear line
        lcd.setCursor(0, 1);
        lcd.print(inputCode);    

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
                    return;
                }
                roundDeadline = millis() + roundTimeMs;
                gameState = getNewState();
            }
        }
    } else {  // Keypad pressed at the wrong time
        Serial.println("Keypad used at wrong time => Game Over, score = 0.");
        displayGameOver();
        gameState = GAME_OVER;
    }
}
}

// Scan keypad
char scanKeypad() {
  for (int c = 0; c < 3; c++) {
    digitalWrite(colPins[c], LOW);
    for (int r = 0; r < 4; r++) {
      if (digitalRead(rowPins[r]) == LOW) {
        delay(200);
        while (digitalRead(rowPins[r]) == LOW);
        digitalWrite(colPins[c], HIGH);
        return keys[r][c];
      }
    }
    digitalWrite(colPins[c], HIGH);
  }
  return 0;
}

// ------------------------------------
// ISR for button press (debounce)
// ------------------------------------
void handleButtonISR() {
  unsigned long now = millis();
  bool state = digitalRead(BUTTON_PIN) == LOW;
  if (state && now - lastInterruptTime > buttonDebounceDelay) {
    buttonPressed = true;
    lastInterruptTime = now;
  }
}

// ISR for key twist (debounce)
void handleKeyISR() {
  unsigned long now = millis();
  bool state = digitalRead(KEY_PIN) == LOW;
  if (state && now - lastInterruptTime > keyDebounceDelay) {
    keyTwisted = true;
    lastInterruptTime = now;
  }
}

// picking new state after each round
GameState getNewState() {
  inputIndex = 0;

  // Generates a number 
  int n = 100;
  int rnd = rand() % (n + 1);

  // button state
  if (rnd <= 33) {
    Serial.print("Next round! You have ");
    Serial.print(roundTimeMs / 1000.0);
    Serial.println(" seconds left. PRESS THE BUTTON!");
    //myDFPlayer.volume(20);
    //myDFPlayer.play(4); // nuke it
    lcd.setCursor(0, 1);
    lcd.print("Button");
    return WAITING_FOR_BUTTON;
  } 
  // key twist state
  else if (rnd < 67) {
    Serial.print("Next round! You have ");
    Serial.print(roundTimeMs / 1000.0);
    Serial.println(" seconds left. TWIST THE KEY!");
    //myDFPlayer.volume(20);
    //myDFPlayer.play(2); // twist it
    lcd.setCursor(0, 1);
    lcd.print("Key Twist");
    return WAITING_FOR_KEY_TWIST;
  }
  // keypad state
  else {
    Serial.print("Next round! You have ");
    Serial.print(roundTimeMs / 1000.0);
    Serial.println(" seconds left. ENTER 1895!");
    //myDFPlayer.volume(20);
    //myDFPlayer.play(2); // twist it
    lcd.setCursor(0, 1);
    lcd.print("Enter 1895");
    return WAITING_FOR_KEYPAD;
  }
}

void displayScore() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  lcd.print(score);
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

