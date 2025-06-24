#include <BleKeyboard.h>

// Button GPIOs
#define BUTTON_A 33
#define BUTTON_B 25
#define BUTTON_C 26
#define BUTTON_D 27
#define BUTTON_E 14
#define VOTE_BUTTON 12

// Feedback
#define VOTE_BUZZER 13
#define VOTE_STATUS_LED 32  // Voting status LED

BleKeyboard bleKeyboard("ESP32 Voting Keyboard", "VOTING_MACHINE", 100);

// Voting control flag
bool canVote = false;

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(BUTTON_D, INPUT_PULLUP);
  pinMode(BUTTON_E, INPUT_PULLUP);
  pinMode(VOTE_BUTTON, INPUT_PULLUP);

  pinMode(VOTE_BUZZER, OUTPUT);
  pinMode(VOTE_STATUS_LED, OUTPUT);
  digitalWrite(VOTE_STATUS_LED, LOW);

  bleKeyboard.begin();
  Serial.println("BLE Keyboard initialized, waiting for connection...");
}

void loop() {
  if (!bleKeyboard.isConnected()) {
    digitalWrite(VOTE_STATUS_LED, LOW);
    delay(500);
    return; // wait for connection
  }

  // Check vote session start button
  if (digitalRead(VOTE_BUTTON) == LOW) {
    canVote = true;
    digitalWrite(VOTE_STATUS_LED, HIGH);
    Serial.println("Voting session started.");
    buzz();
    waitRelease(VOTE_BUTTON);
  }

  if (canVote) {
    if (digitalRead(BUTTON_A) == LOW) {
      sendKey('Q');
      finishVoting(BUTTON_A);
    } 
    else if (digitalRead(BUTTON_B) == LOW) {
      sendKey('W');
      finishVoting(BUTTON_B);
    } 
    else if (digitalRead(BUTTON_C) == LOW) {
      sendKey('E');
      finishVoting(BUTTON_C);
    } 
    else if (digitalRead(BUTTON_D) == LOW) {
      sendKey('R');
      finishVoting(BUTTON_D);
    } 
    else if (digitalRead(BUTTON_E) == LOW) {
      sendKey('T');
      finishVoting(BUTTON_E);
    }
  }
}

void sendKey(char key) {
  if (!bleKeyboard.isConnected()) return;

  Serial.print("Sending key: ");
  Serial.println(key);
  bleKeyboard.press(key);
  delay(100);
  bleKeyboard.release(key);
  buzz();
}

void finishVoting(int buttonPin) {
  canVote = false;
  digitalWrite(VOTE_STATUS_LED, LOW);
  waitRelease(buttonPin);
}

void buzz() {
  digitalWrite(VOTE_BUZZER, HIGH);
  delay(200);
  digitalWrite(VOTE_BUZZER, LOW);
}

void waitRelease(int pin) {
  while (digitalRead(pin) == LOW) {
    delay(10);
  }
  delay(200); // debounce
}
