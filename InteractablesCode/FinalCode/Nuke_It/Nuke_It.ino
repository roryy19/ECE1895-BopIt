//------------------------------------------------------------
// NUKE-IT Project Prototype Code
// Group Psi
//------------------------------------------------------------
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>

//------------------------------------------------------------
// Constants and Pin Definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define BUTTON_PIN 2
#define KEY_TWIST_PIN 3

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Keypad Setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {4, 5, 6, 7};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

//------------------------------------------------------------
// Variables
volatile bool buttonPressed = false;
volatile bool keyTwisted = false;

unsigned long taskStartTime;
unsigned long taskTimeLimit = 5000;
String enteredCode = "";
int score = 0;

enum Task { BUTTON, KEYPAD, KEY };

//------------------------------------------------------------
void setup() 
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(KEY_TWIST_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);
  attachInterrupt(digitalPinToInterrupt(KEY_TWIST_PIN), onKeyTwist, CHANGE);

  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
  {
    Serial.println(F("OLED init failed"));
    while (1);
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();

  // Start logo animation
  logoAnimation();
}

//------------------------------------------------------------
void loop() {
  if (buttonPressed) {
    buttonPressed = false;
    startGame();
  }
}

//------------------------------------------------------------
void logoAnimation() {
  int posY = -32;  // Start position for the logo

  // Move the logo down to the center
  while (posY < 16) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(25, posY);
    display.println("NUKE-IT");
    display.display();
    
    posY++;  // Move the logo down one pixel
    delay(30);  // Slow down the movement for a smoother effect
  }

  // Wait for the logo to center
  delay(500);

  // display start instructions
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.println("Press Button To Start");
  display.display();
}

//------------------------------------------------------------
void startGame() {
  score = 0;
  taskTimeLimit = 5000;

  while (true) {
    enteredCode = "";
    buttonPressed = false;
    keyTwisted = false;
    keypad.getKey(); // clear input buffer

    Task currentTask = (Task)random(0, 3);
    displayTaskPrompt(currentTask);

    bool success = checkSuccess(currentTask);

    if (success) {
      score++;
      if (taskTimeLimit > 2000) taskTimeLimit -= 200;
      handleSuccess();
    } else {
      gameOver();
      break;
    }
  }
}

//------------------------------------------------------------
void displayTaskPrompt(Task task) 
{
  display.clearDisplay();

  // Score in top-right corner
  display.setTextSize(1);
  display.setCursor(70, 0);
  display.print("Score: ");
  display.print(score);

  // Timer bar frame
  display.drawRect(0, 10, 128, 8, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 25);
  switch (task) 
  {
    case BUTTON:
      display.println("Nuke It!");
      break;
    case KEY:
      display.println("Twist It!");
      break;
    case KEYPAD:
      display.println("Code It!");
      display.setTextSize(1);
      display.setCursor(0, 50);
      display.println("Enter Code: 1895");
      break;
  }
  display.display();
}

//------------------------------------------------------------
bool checkSuccess(Task task) 
{
  taskStartTime = millis();
  enteredCode = "";

  while (millis() - taskStartTime < taskTimeLimit) 
  {
    // Clear the timer bar
    display.fillRect(1, 11, 126, 6, SSD1306_BLACK);

    // Draw the new timer bar based on remaining time
    int remaining = map(taskTimeLimit - (millis() - taskStartTime), 0, taskTimeLimit, 0, 126);
    display.fillRect(1, 11, remaining, 6, SSD1306_WHITE);

    display.display();

    switch (task) 
    {
      case BUTTON:
        if (buttonPressed) {
          buttonPressed = false;
          return true;
        }
        break;

      case KEY:
        if (keyTwisted) {
          keyTwisted = false;
          return true;
        }
        break;

      case KEYPAD: {
        char key = keypad.getKey();
        if (key && isDigit(key)) {
          enteredCode += key;
          display.clearDisplay();
          display.setTextSize(2);
          display.setCursor(0, 20);
          display.println(enteredCode);
          display.display();

          if (enteredCode.length() == 4) {
            return enteredCode == "1895";
          }
        }
        break;
      }
    }
  }

  return false;
}

//------------------------------------------------------------
void handleSuccess() 
{
  delay(500);
}

//------------------------------------------------------------
void gameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println("You lost!");
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.println("Nuke-It to Restart");
  display.display();

  while (!buttonPressed);
  buttonPressed = false;
  delay(300);
}

//------------------------------------------------------------
// INTERRUPT HANDLERS
void onButtonPress() 
{
  buttonPressed = true;
}

void onKeyTwist() 
{
  keyTwisted = true;
}
