#include <BleKeyboard.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

// Wi-Fi credentials
#define WIFI_SSID "ZTE-cz9JjE"
#define WIFI_PASSWORD "pgh3pt6r"

// Firebase config
#define FIREBASE_HOST "https://sanmati-voting-machine-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "Q7vE5Hcvk1r0PMKH0K3k9nPFahr5j7YFOUxLJZsv"

// Button GPIOs
#define BUTTON_A 33
#define BUTTON_B 25
#define BUTTON_C 26
#define BUTTON_D 27
#define BUTTON_E 14
#define VOTE_BUTTON 12
#define MODE_SELECT_BUTTON 4

// Feedback
#define VOTE_BUZZER 13
#define VOTE_STATUS_LED 32
#define WIFI_LED 15
#define BLE_LED 2

int voteA = 0, voteB = 0, voteC = 0, voteD = 0, voteE = 0;
bool canVote = false;
bool bleActive = false;
bool useWiFiMode = true;

BleKeyboard bleKeyboard("ESP32 Voting Keyboard", "VOTING_MACHINE", 100);
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// Array of Firebase reference paths for votes
const char* votePaths[] = {
  "/votes/president/optionA",
  "/votes/president/optionB",
  "/votes/president/optionC",
  "/votes/president/optionD",
  "/votes/president/optionE"
};

// const char* votePaths[] = {
//   "/votes/prime-minister/optionA",
//   "/votes/prime-minister/optionB",
//   "/votes/prime-minister/optionC",
//   "/votes/prime-minister/optionD",
//   "/votes/prime-minister/optionE"
// };

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(BUTTON_D, INPUT_PULLUP);
  pinMode(BUTTON_E, INPUT_PULLUP);
  pinMode(VOTE_BUTTON, INPUT_PULLUP);
  pinMode(MODE_SELECT_BUTTON, INPUT_PULLUP);

  pinMode(VOTE_BUZZER, OUTPUT);
  pinMode(VOTE_STATUS_LED, OUTPUT);
  pinMode(WIFI_LED, OUTPUT);
  pinMode(BLE_LED, OUTPUT);

  digitalWrite(VOTE_STATUS_LED, LOW);
  digitalWrite(WIFI_LED, LOW);
  digitalWrite(BLE_LED, LOW);

  // Let user select mode
  selectStartupMode();

  if (useWiFiMode) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(300);
    }
    Serial.println("\nConnected to Wi-Fi!");

    config.database_url = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    getVoteData();
  } else {
    bleKeyboard.begin();
    bleActive = true;
    Serial.println("Started in BLE mode.");
  }

  Serial.println("System ready. Use VOTE_BUTTON to begin voting.");
}

void loop() {
  if (digitalRead(VOTE_BUTTON) == LOW) {
    canVote = true;
    digitalWrite(VOTE_STATUS_LED, HIGH);
    Serial.println("Vote session started. Please choose your option.");
    buzz();
    waitRelease(VOTE_BUTTON);
  }

  if (canVote) {
    if (digitalRead(BUTTON_A) == LOW) handleVote('Q', votePaths[0], voteA, BUTTON_A);
    else if (digitalRead(BUTTON_B) == LOW) handleVote('W', votePaths[1], voteB, BUTTON_B);
    else if (digitalRead(BUTTON_C) == LOW) handleVote('E', votePaths[2], voteC, BUTTON_C);
    else if (digitalRead(BUTTON_D) == LOW) handleVote('R', votePaths[3], voteD, BUTTON_D);
    else if (digitalRead(BUTTON_E) == LOW) handleVote('T', votePaths[4], voteE, BUTTON_E);
  }
}

void selectStartupMode() {
  Serial.println("Hold MODE_SELECT_BUTTON to choose mode...");
  Serial.println("1 press = Wi-Fi, 2 quick presses = BLE");

  // Wait for first press
  while (digitalRead(MODE_SELECT_BUTTON) == HIGH) {
    delay(10);
  }

  buzz();
  Serial.println("First press detected.");
  waitRelease(MODE_SELECT_BUTTON);

  // Start 1 second window to detect second press
  unsigned long start = millis();
  bool secondPress = false;

  while (millis() - start < 1000) {
    if (digitalRead(MODE_SELECT_BUTTON) == LOW) {
      buzz();
      secondPress = true;
      waitRelease(MODE_SELECT_BUTTON);
      break;
    }
  }

  if (secondPress) {
    useWiFiMode = false;
    digitalWrite(BLE_LED, HIGH);
    digitalWrite(WIFI_LED, LOW);
    Serial.println("BLE mode selected.");
  } else {
    useWiFiMode = true;
    digitalWrite(WIFI_LED, HIGH);
    digitalWrite(BLE_LED, LOW);
    Serial.println("Wi-Fi mode selected.");
  }
}

void handleVote(char key, const char* path, int &localVoteVar, int buttonPin) {
  if (!useWiFiMode) {
    if (bleActive && bleKeyboard.isConnected()) {
      Serial.print("Sending BLE key: ");
      Serial.println(key);
      bleKeyboard.press(key);
      delay(100);
      bleKeyboard.release(key);
    } else {
      Serial.println("BLE not active or not connected.");
    }
  } else {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi disconnected. Trying to reconnect...");
      WiFi.reconnect();
      buzz();
      return;
    }

    int currentValue = 0;
    if (Firebase.getInt(firebaseData, path)) {
      currentValue = firebaseData.intData();
    } else {
      Serial.println("Error fetching vote: " + firebaseData.errorReason());
    }

    localVoteVar = currentValue + 1;
    if (Firebase.setInt(firebaseData, path, localVoteVar)) {
      Serial.println("Vote recorded: " + String(localVoteVar));
    } else {
      Serial.println("Error writing vote: " + firebaseData.errorReason());
    }
  }

  buzz();
  finishVoting(buttonPin);
}

void finishVoting(int buttonPin) {
  canVote = false;
  digitalWrite(VOTE_STATUS_LED, LOW);
  waitRelease(buttonPin);
}

void getVoteData() {
  if (Firebase.getInt(firebaseData, votePaths[0])) voteA = firebaseData.intData(); else voteA = 0;
  if (Firebase.getInt(firebaseData, votePaths[1])) voteB = firebaseData.intData(); else voteB = 0;
  if (Firebase.getInt(firebaseData, votePaths[2])) voteC = firebaseData.intData(); else voteC = 0;
  if (Firebase.getInt(firebaseData, votePaths[3])) voteD = firebaseData.intData(); else voteD = 0;
  if (Firebase.getInt(firebaseData, votePaths[4])) voteE = firebaseData.intData(); else voteE = 0;

  Serial.println("Firebase vote counts loaded:");
  Serial.println("A: " + String(voteA));
  Serial.println("B: " + String(voteB));
  Serial.println("C: " + String(voteC));
  Serial.println("D: " + String(voteD));
  Serial.println("E: " + String(voteE));
}

void buzz() {
  digitalWrite(VOTE_BUZZER, HIGH);
  delay(200);
  digitalWrite(VOTE_BUZZER, LOW);
}

void waitRelease(int pin) {
  while (digitalRead(pin) == LOW) delay(10);
  delay(200);
}
