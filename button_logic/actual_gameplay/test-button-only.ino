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

// Pin definitions
const int BUTTON_PIN = 3;
const int POWER_PIN  = 7;
const int SPEAKER_PIN = 8; // not used much here, just a placeholder

// For button interrupt
volatile bool buttonPressed = false;
volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 1000; // ms

// Game states
enum GameState {
  IDLE,
  WAITING_FOR_BUTTON,
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

// ------------------------------------
// SETUP
// ------------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(POWER_PIN, INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);

  // Attach interrupt for button
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonISR, CHANGE);

  Serial.println("Step 1: Button Only - Setup complete.");
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

  switch (gameState) {
    case IDLE:
      if (powerIsOn) {
        // Start a new game
        score = 0;
        Serial.println("Power ON => Starting game. Round 1: Press the button!");
        // Start the first round
        roundDeadline = millis() + roundTimeMs;
        gameState = WAITING_FOR_BUTTON;
      }
      break;

    case WAITING_FOR_BUTTON:
      // Check if time is up
      if (millis() > roundDeadline) {
        // Timed out => game over
        Serial.println("TIMEOUT => Game Over. Score reset to 0.");
        score = 0;
        gameState = GAME_OVER;
      }

      // Also check if button was pressed at the wrong time 
      // (i.e., user pressed it outside of WAITING_FOR_BUTTON state).
      // Actually, in this simplified version, pressing the button
      // during WAITING_FOR_BUTTON is always correct. So no "wrong time" check here.
      break;

    case GAME_OVER:
      // Wait for power off or something else to reset the game
      // We’ll just do nothing here. If user turns off power => IDLE.
      break;
  }

  // If the user has pressed the button (ISR flag), handle it
  if (buttonPressed) {
    noInterrupts();
    buttonPressed = false;
    interrupts();

    // Evaluate the press based on the current game state
    if (gameState == WAITING_FOR_BUTTON) {
      // It's correct
      score++;
      Serial.print("Correct button press! Score = ");
      Serial.println(score);

      // Start next round
      if (score < 10) {
        roundTimeMs -= roundDecMs;
      }
      else {
        Serial.println("Congratulations! You Win! :)");
        gameState = GAME_OVER;
        return;
      }
      
      roundDeadline = millis() + roundTimeMs;
      Serial.print("Next round! You have ");
      Serial.print(roundTimeMs / 1000.0);
      Serial.println(" seconds left. Press the button again!");
    } else {
      // If they pressed the button in IDLE or GAME_OVER, that’s “wrong time”
      // => game over, score = 0
      Serial.println("Button pressed at wrong time => Game Over, score = 0.");
      score = 0;
      gameState = GAME_OVER;
    }
  }
}

// ------------------------------------
// ISR for button press (debounce)
// ------------------------------------
void handleButtonISR() {
  unsigned long now = millis();
  if (now - lastInterruptTime > debounceDelay) {
    buttonPressed = true;
    lastInterruptTime = now;
  }
}
