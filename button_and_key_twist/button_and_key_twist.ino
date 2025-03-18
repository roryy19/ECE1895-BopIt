#include <Arduino.h>

// Pin definitions
const int KEY_PIN = 2;
const int BUTTON_PIN = 3;
const int POWER_PIN  = 7;
const int SPEAKER_PIN = 8; // not used much here, just a placeholder

// For button interrupt
volatile bool buttonPressed = false;
volatile bool keyTwisted = false;
volatile unsigned long lastInterruptTime = 0;
const unsigned long buttonDebounceDelay = 1000; // ms
const unsigned long keyDebounceDelay = 1500;

// Game states
enum GameState {
  IDLE,
  WAITING_FOR_BUTTON,
  WAITING_FOR_KEY_TWIST,
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

  // Attach interrupt for button and key
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(KEY_PIN), handleKeyISR, CHANGE);

  Serial.println("Setup complete.");
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
        score = 0;
        gameState = GAME_OVER;
      }

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
      gameState = getNewState();
    } 
    else {
      // If they pressed the button in IDLE, different command state, or GAME_OVER, that’s “wrong time”
      // => game over, score = 0
      Serial.println("Button pressed at wrong time => Game Over, score = 0.");
      score = 0;
      gameState = GAME_OVER;
    }
  }

  // If the user has pressed the button (ISR flag), handle it
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
      gameState = getNewState();
    } 
    else {
      // If they pressed the button in IDLE, different command state, or GAME_OVER, that’s “wrong time”
      // => game over, score = 0
      Serial.println("Key twisted at wrong time => Game Over, score = 0.");
      score = 0;
      gameState = GAME_OVER;
    }
  }
}
 
// picking new state after each round
GameState getNewState() {
  //srand(time(NULL));

  int rnd = random(0, 100); // Generates a number from 0 to 9

  // button state
  if (rnd < 50) {
    Serial.print("Next round! You have ");
    Serial.print(roundTimeMs / 1000.0);
    Serial.println(" seconds left. PRESS THE BUTTON!");
    return WAITING_FOR_BUTTON;
  } 
  // key twist state
  else {
    Serial.print("Next round! You have ");
    Serial.print(roundTimeMs / 1000.0);
    Serial.println(" seconds left. TWIST THE KEY!");
    return WAITING_FOR_KEY_TWIST;
  }
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
