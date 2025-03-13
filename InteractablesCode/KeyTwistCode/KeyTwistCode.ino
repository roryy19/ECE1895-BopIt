// Basic Key Twist Detection Code
// 3/13/25

const int keyPin = 2;  // key switch on pin D2

void setup() 
{
    pinMode(keyPin, INPUT_PULLUP);  // enables internal pull-up resistor
    Serial.begin(9600);
}

void loop() 
{
    int keyState = digitalRead(keyPin);  // read the key state

    if (keyState == LOW) 
    {
        Serial.println("Key is ON");
    } 
    else 
    {
        Serial.println("Key is OFF");
    }

    delay(200);  // delay to avoid excess serial prints
}
