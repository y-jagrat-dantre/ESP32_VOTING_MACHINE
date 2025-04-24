#include <WiFi.h>
#include <FirebaseESP32.h>

// Wi-Fi credentials
#define WIFI_SSID "senior wing #2"
#define WIFI_PASSWORD "atl@2022"

// Firebase config
#define FIREBASE_HOST "esp32-cam-python-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "jd04MpCh5jRrlrIfuI3LFVqgffengJMvIU4Ryynb"

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
  pinMode(VOTE_STATUS_LED, OUTPUT);
  digitalWrite(VOTE_STATUS_LED, LOW);  // LED off initially

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

  // Load previous vote counts from Firebase
  getVoteData();
}

void loop() {
  // Unlock voting when VOTE_BUTTON is pressed
  if (digitalRead(VOTE_BUTTON) == LOW) {
    canVote = true;
    digitalWrite(VOTE_STATUS_LED, HIGH);
    Serial.println("Vote session started. Please choose your option.");
    buzz();
    waitRelease(VOTE_BUTTON);
  }

  if (canVote) {
    if (digitalRead(BUTTON_A) == LOW) {
      sendVote("/votes/optionA1", voteA);
      finishVoting(BUTTON_A);
    } else if (digitalRead(BUTTON_B) == LOW) {
      sendVote("/votes/optionB1", voteB);
      finishVoting(BUTTON_B);
    } else if (digitalRead(BUTTON_C) == LOW) {
      sendVote("/votes/optionC1", voteC);
      finishVoting(BUTTON_C);
    } else if (digitalRead(BUTTON_D) == LOW) {
      sendVote("/votes/optionD1", voteD);
      finishVoting(BUTTON_D);
    } else if (digitalRead(BUTTON_E) == LOW) {
      sendVote("/votes/optionE1", voteE);
      finishVoting(BUTTON_E);
    }
  }
}

// Handle end of voting: disable voting, turn off LED, wait for button release
void finishVoting(int buttonPin) {
  canVote = false;
  digitalWrite(VOTE_STATUS_LED, LOW);
  waitRelease(buttonPin);
}

// Send vote to Firebase after fetching current count
void sendVote(String path, int &localVoteVar) {
  int currentValue = 0;

  // Get current vote count from Firebase
  if (Firebase.getInt(firebaseData, path)) {
    currentValue = firebaseData.intData();
  } else {
    Serial.println("Error fetching current vote: " + firebaseData.errorReason());
  }

  // Update local variable and Firebase
  localVoteVar = currentValue + 1;
  buzz();
  Serial.print("Sending vote to ");
  Serial.println(path);

  if (Firebase.setInt(firebaseData, path, localVoteVar)) {
    Serial.println("Vote sent! New value: " + String(localVoteVar));
  } else {
    Serial.print("Firebase Error: ");
    Serial.println(firebaseData.errorReason());
  }
}

// Load existing vote counts on startup
void getVoteData() {
  if (Firebase.getInt(firebaseData, "/votes/optionA1"))
    voteA = firebaseData.intData();
  else
    voteA = 0;

  if (Firebase.getInt(firebaseData, "/votes/optionB1"))
    voteB = firebaseData.intData();
  else
    voteB = 0;

  if (Firebase.getInt(firebaseData, "/votes/optionC1"))
    voteC = firebaseData.intData();
  else
    voteC = 0;

  if (Firebase.getInt(firebaseData, "/votes/optionD1"))
    voteD = firebaseData.intData();
  else
    voteD = 0;

  if (Firebase.getInt(firebaseData, "/votes/optionE1"))
    voteE = firebaseData.intData(); 
  else
    voteE = 0;

  Serial.println("Vote counts synced from Firebase:");
  Serial.println("A: " + String(voteA));
  Serial.println("B: " + String(voteB));
  Serial.println("C: " + String(voteC));
  Serial.println("D: " + String(voteD));
  Serial.println("E: " + String(voteE));
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
