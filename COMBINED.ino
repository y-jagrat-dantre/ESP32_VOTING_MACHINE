#include <BleKeyboard.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

// Wi-Fi credentials
#define WIFI_SSID "ZTE-cz9JjE"
#define WIFI_PASSWORD "pgh3pt6r"

// Firebase config
#define FIREBASE_HOST "https://esp32-cam-python-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "jd04MpCh5jRrlrIfuI3LFVqgffengJMvIU4Ryynb"

// Button GPIOs
#define BUTTON_A 33
#define BUTTON_B 25
#define BUTTON_C 26
#define BUTTON_D 27
#define BUTTON_E 14
#define VOTE_BUTTON 12
#define MODE_SELECT_BUTTON 4  // Button to select Wi-Fi or BLE mode

// Feedback
#define VOTE_BUZZER 13
#define VOTE_STATUS_LED 32   // Voting status LED (keep it)
#define WIFI_LED 16          // Wi-Fi mode indicator LED
#define BLE_LED 2            // BLE mode indicator LED

// Vote counts
int voteA = 0, voteB = 0, voteC = 0, voteD = 0, voteE = 0;

// Control flags
bool canVote = false;
bool bleActive = false;
bool useWiFiMode = true;  // Mode selection flag

BleKeyboard bleKeyboard("ESP32 Voting Keyboard", "VOTING_MACHINE", 100);
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
  Serial.begin(115200);

  // Setup pins
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
  digitalWrite(WIFI_LED, HIGH);  // Start in Wi-Fi mode
  digitalWrite(BLE_LED, LOW);

  // Initialize Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected to Wi-Fi!");

  // Initialize Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Load previous vote counts from Firebase
  getVoteData();

  Serial.println("Press MODE_SELECT_BUTTON to switch between Wi-Fi and BLE mode.");
}

void loop() {
  // Mode selection
  if (digitalRead(MODE_SELECT_BUTTON) == LOW) {
    useWiFiMode = !useWiFiMode;

    if (useWiFiMode) {
      Serial.println("Switched to Wi-Fi Mode.");
      digitalWrite(WIFI_LED, HIGH);
      digitalWrite(BLE_LED, LOW);
      if (bleActive) {
        bleKeyboard.end();
        bleActive = false;
      }
    } else {
      Serial.println("Switched to BLE Mode.");
      digitalWrite(WIFI_LED, LOW);
      digitalWrite(BLE_LED, HIGH);
      if (!bleActive) {
        bleKeyboard.begin();
        bleActive = true;
        Serial.println("BLE Keyboard enabled.");
      }
    }

    buzz();
    waitRelease(MODE_SELECT_BUTTON);
  }

  // Start voting session
  if (digitalRead(VOTE_BUTTON) == LOW) {
    canVote = true;
    digitalWrite(VOTE_STATUS_LED, HIGH);  // Voting started, LED ON
    Serial.println("Vote session started. Please choose your option.");
    buzz();
    waitRelease(VOTE_BUTTON);
  }

  if (canVote) {
    if (digitalRead(BUTTON_A) == LOW) {
      handleVote('Q', "/votes/optionA", voteA, BUTTON_A);
    } else if (digitalRead(BUTTON_B) == LOW) {
      handleVote('W', "/votes/optionB", voteB, BUTTON_B);
    } else if (digitalRead(BUTTON_C) == LOW) {
      handleVote('E', "/votes/optionC", voteC, BUTTON_C);
    } else if (digitalRead(BUTTON_D) == LOW) {
      handleVote('R', "/votes/optionD", voteD, BUTTON_D);
    } else if (digitalRead(BUTTON_E) == LOW) {
      handleVote('T', "/votes/optionE", voteE, BUTTON_E);
    }
  }
}

void handleVote(char key, String path, int &localVoteVar, int buttonPin) {
  if (!useWiFiMode) {
    // BLE Mode
    if (bleActive && bleKeyboard.isConnected()) {
      Serial.print("Sending BLE key: ");
      Serial.println(key);
      bleKeyboard.press(key);
      delay(100);
      bleKeyboard.release(key);
    } else {
      Serial.println("BLE not active or not connected, skipping key press.");
    }
  } else {
    // Wi-Fi Mode
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
      Serial.println("Error fetching current vote: " + firebaseData.errorReason());
    }

    localVoteVar = currentValue + 1;
    Serial.print("Sending vote to ");
    Serial.println(path);

    if (Firebase.setInt(firebaseData, path, localVoteVar)) {
      Serial.println("Vote sent! New value: " + String(localVoteVar));
    } else {
      Serial.println("Firebase Error: " + firebaseData.errorReason());
    }
  }

  buzz();
  finishVoting(buttonPin);
}

void finishVoting(int buttonPin) {
  canVote = false;
  digitalWrite(VOTE_STATUS_LED, LOW); // Turn OFF vote session LED
  waitRelease(buttonPin);
}

void getVoteData() {
  if (Firebase.getInt(firebaseData, "/votes/optionA"))
    voteA = firebaseData.intData();
  else
    voteA = 0;

  if (Firebase.getInt(firebaseData, "/votes/optionB"))
    voteB = firebaseData.intData();
  else
    voteB = 0;

  if (Firebase.getInt(firebaseData, "/votes/optionC"))
    voteC = firebaseData.intData();
  else
    voteC = 0;

  if (Firebase.getInt(firebaseData, "/votes/optionD"))
    voteD = firebaseData.intData();
  else
    voteD = 0;

  if (Firebase.getInt(firebaseData, "/votes/optionE"))
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

void buzz() {
  digitalWrite(VOTE_BUZZER, HIGH);
  delay(200);
  digitalWrite(VOTE_BUZZER, LOW);
}

void waitRelease(int pin) {
  while (digitalRead(pin) == LOW) {
    delay(10);
  }
  delay(200);
}
