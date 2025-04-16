#include <WiFi.h>
#include <FirebaseESP32.h>

// Wi-Fi credentials
#define WIFI_SSID "ZTE-cz9JjE"
#define WIFI_PASSWORD "pgh3pt6r"

// Firebase config
#define FIREBASE_HOST "esp32-cam-python-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "jd04MpCh5jRrlrIfuI3LFVqgffengJMvIU4Ryynb"

// Button GPIOs
#define BUTTON_A 15
#define BUTTON_B 4
#define BUTTON_C 5
#define BUTTON_D 18
#define BUTTON_E 19
#define VOTE_BUTTON 22

// Feedback
#define VOTE_BUZZER 21

// Vote counts
int voteA = 0, voteB = 0, voteC = 0, voteD = 0, voteE = 0;

// Voting control flag
bool canVote = false;

FirebaseData firebaseData;

void setup() {
  Serial.begin(115200);

  // Setup pins
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(BUTTON_D, INPUT_PULLUP);
  pinMode(BUTTON_E, INPUT_PULLUP);
  pinMode(VOTE_BUTTON, INPUT_PULLUP);
  pinMode(VOTE_BUZZER, OUTPUT);

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected to Wi-Fi!");

  // Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
}

void loop() {
  // Unlock voting when VOTE_BUTTON is pressed
  if (digitalRead(VOTE_BUTTON) == LOW) {
    canVote = true;
    Serial.println("Vote session started. Please choose your option.");
    buzz();
    waitRelease(VOTE_BUTTON);
  }

  if (canVote) {
    if (digitalRead(BUTTON_A) == LOW) {
      voteA++;
      sendVote("/votes/optionA", voteA);
      canVote = false;
      waitRelease(BUTTON_A);
    }
    else if (digitalRead(BUTTON_B) == LOW) {
      voteB++;
      sendVote("/votes/optionB", voteB);
      canVote = false;
      waitRelease(BUTTON_B);
    }
    else if (digitalRead(BUTTON_C) == LOW) {
      voteC++;
      sendVote("/votes/optionC", voteC);
      canVote = false;
      waitRelease(BUTTON_C);
    }
    else if (digitalRead(BUTTON_D) == LOW) {
      voteD++;
      sendVote("/votes/optionD", voteD);
      canVote = false;
      waitRelease(BUTTON_D);
    }
    else if (digitalRead(BUTTON_E) == LOW) {
      voteE++;
      sendVote("/votes/optionE", voteE);
      canVote = false;
      waitRelease(BUTTON_E);
    }
  }
}

// Send vote to Firebase
void sendVote(String path, int value) {
  Serial.print("Sending vote to ");
  Serial.println(path);
  buzz();
  if (Firebase.setInt(firebaseData, path, value)) {
    Serial.println("Vote sent!");
  } else {
    Serial.print("Firebase Error: ");
    Serial.println(firebaseData.errorReason());
  }
}

// Buzzer feedback
void buzz() {
  digitalWrite(VOTE_BUZZER, HIGH);
  delay(300);
  digitalWrite(VOTE_BUZZER, LOW);
}

// Debounce button release
void waitRelease(int pin) {
  while (digitalRead(pin) == LOW);
  delay(200);
}
