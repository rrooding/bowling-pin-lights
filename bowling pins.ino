// Comment this line to turn off debug logging
#define DEBUG

// Pretty print debugging information
#ifdef DEBUG
#define INIT_SERIAL() Serial.begin(115200);
#define DEBUG_PRINT(s)               \
  Serial.print(millis());            \
  Serial.print(": ");                \
  Serial.print(__PRETTY_FUNCTION__); \
  Serial.print(':');                 \
  Serial.print(__LINE__);            \
  Serial.print(' ');                 \
  Serial.println(s)
#else
#define INIT_SERIAL()
#define DEBUG_PRINT(s)
#endif

// Output pin configuration
const int numLights    = 5;
const int outputPins[] = { 2, 3, 4, 5, 6 };
const int inputPins[]  = { A0, A1, A2, A3, A4 };

// The amount of cycles that we will only read and not
// enable the lights
int startupCycles = 20;

// How much difference should there be before turning on a light?
const int delta = 25;

// How many readings to use for the moving average
const int numReadings  = 10;

// Timing (for loop and lights)
long previousLoop = 0;
long loopInterval = 100; // Take a reading every 100 milliseconds
long lightEnabledTime = 30000; // Keep the light on for 30 seconds

// Magic sequence
const int magicCode[] = { 0, 1, 2, 3, 4 };
const int magicSequence[16][5] = {
  { 1, 0, 0, 0, 0 },
  { 0, 1, 0, 0, 0 },
  { 0, 0, 1, 0, 0 },
  { 0, 0, 0, 1, 0 },
  { 0, 0, 0, 0, 1 },
  { 0, 0, 0, 1, 0 },
  { 0, 0, 1, 0, 0 },
  { 0, 1, 0, 0, 0 },
  { 1, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0 },
  { 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1 }
};

// Some variables for state keeping
int readings[numLights][numReadings];
int totals[numLights];
int readingIndex = 0;
long lightState[numLights];

void setup() {
  // initialize serial comms for debugging
  INIT_SERIAL();

  // initialize all output pins
  initOutputPins();

  // initialize the readings
  initReadings();
}

void loop() {
  unsigned long currentLoop = millis();

  if(currentLoop - previousLoop > loopInterval) {
    // Read all the sensor inputs
    updateReadings();

    // Early return if we're still in the startup cycle
    if(startupCycles > 0) {
      startupCycles--;
      return;
    }

    // Check to see if there are any lights that need to be switched on or off
    toggleLights();

    // Check if the magic sequence was used
    performMagic();
  }
}

void performMagic() {
  if(!allLightsOn()) {
    return;
  }

  if(isMagicSequence()) {
    turnOffAllLights();
  }
}

bool allLightsOn() {
  for(int light = 0; light < numLights; light++) {
    if(lightState[light] == 0) {
      return false;
    }
  }
  return true;
}

bool isMagicCode() {
  previousState = 0;
  for(int i; i < numLights; i++) {
    int index = magicCode[i];

    if(i > 0) {
      if(previousState > lightState[index]) {
        return false;
      }
    }

    previousState = lightState[index];
  }

  return true;
}

void initOutputPins() {
  DEBUG_PRINT("Initializing output pins");
  for(int i = 0; i < numLights; i++) {
    pinMode(outputPins[i], OUTPUT);
  }
}

void initReadings() {
  DEBUG_PRINT("Initializing readings");
  for(int i = 0; i < numLights; i++) {
    for(int y = 0; y < numReadings; y++) {
      readings[i][y] = 0;
    }

    lightState[i] = 0;
    totals[i] = 0;
  }
}

void updateReadings() {
  DEBUG_PRINT("Updating readings");
  for(int i = 0; i < numLights; i++) {
    updateReading(i);
  }

  if(++readingIndex >= numReadings) {
    readingIndex = 0;
  }
}

void updateReading(int light) {
  int val = analogRead(inputPins[light]);

  // Subtract the old reading from the total for this light before overwriting it
  totals[light] = totals[light] - readings[light][readingIndex];

  // Save the new reading and add it to the total
  readings[light][readingIndex] = val;
  totals[light] = totals[light] + val;
}

void toggleLights() {
  for(int i = 0; i < numLights; i++) {
    toggleLight(i);
  }
}

void toggleLight(int light) {
  if(lightState[light] <= 0) {
    // Light is turned off, should we turn it on?
    int avg = movingAverageForLight(light);
    int last = lastReading(light);

    int currentDelta = abs(avg - last);
    if(currentDelta >= delta) {
      DEBUG_PRINT("Turning on light");
      DEBUG_PRINT(light);
      turnOnLight(light);
    }
  } else {
    // Light is turned on, should we turn it off?
    unsigned long currentMillis = millis();
    if(currentMillis - lightState[light] > lightEnabledTime) {
      DEBUG_PRINT("Turning off light");
      DEBUG_PRINT(light);
      turnOffLight(light);
    }
  }
}

int movingAverageForLight(int light) {
  return totals[light] / numReadings;
}

int previousIndex() {
  int previousIndex = readingIndex - 1;
  if(previousIndex < 0) {
    previousIndex = numReadings - 1;
  }
  return previousIndex;
}

int lastReading(int light) {
  return readings[light][previousIndex()];
}

void turnOffAllLights() {
  for(int i = 0; i < numLights; i++) {
    turnOffLight(i);
  }
}

void turnOnLight(int light) {
  lightState[light] = millis();
  digitalWrite(outputPins[light], HIGH);
}

void turnOffLight(int light) {
  lightState[light] = 0;
  digitalWrite(outputPins[light], LOW);
}
