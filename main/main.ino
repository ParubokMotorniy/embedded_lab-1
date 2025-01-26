//founding assumptions:
// + The press state does not change in between the frames
// + The frame duration is much shorter than one can press and release a button

enum class FrameType {
  STANDBYOFF,
  STANDBYON,
  OFFTOON,
  ONTOOFF,
};

struct Frame {
  int frameStartLevel{ 0 };
  int frameEndLevel{ 0 };
  FrameType frameType{ FrameType::STANDBYOFF };
};

constexpr size_t frameDurationMs = 40;
constexpr size_t minPressFrames = 5;

//mappings
constexpr int buttonIncrement = 9;  //k2
constexpr int buttonToggle = 8;     //k1

constexpr int numLeds = 4;
constexpr int led1 = 4;
constexpr int led2 = 5;
constexpr int led3 = 6;
constexpr int led4 = 7;

void setup() {
  // put your setup code here, to run once:
  pinMode(buttonIncrement, INPUT);
  pinMode(buttonToggle, INPUT);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);

  Serial.begin(9600);
}

void incrementDiodeCounter() {
  static uint8_t currentCount = 0;
  ++currentCount;

  int leds[] = { led1, led2, led3, led4 };
  for (int i = 0; i < numLeds; ++i) {
    digitalWrite(leds[i], (currentCount >> i) & 0x01);
  }
}

void toggleDiodes(int newState) {
  //TODO: apply new state to all diodes
}

void assignFrameType(Frame &frame) {
  if (frame.frameStartLevel == LOW && frame.frameEndLevel == LOW)
    frame.frameType = FrameType::STANDBYON;

  if (frame.frameStartLevel == LOW && frame.frameEndLevel == HIGH)
    frame.frameType = FrameType::ONTOOFF;

  if (frame.frameStartLevel == HIGH && frame.frameEndLevel == HIGH)
    frame.frameType = FrameType::STANDBYOFF;

  if (frame.frameStartLevel == HIGH && frame.frameEndLevel == LOW)
    frame.frameType = FrameType::OFFTOON;

  switch (frame.frameType) {
    case FrameType::ONTOOFF:
      Serial.println("Type: on to off");

    case FrameType::OFFTOON:
      Serial.println("Type: off to on");

    case FrameType::STANDBYON:
      Serial.println("Type: steady on");

    case FrameType::STANDBYOFF:
      Serial.println("Type: steady off");
  }
}

bool pressSequenceInProgress = false;
size_t pressFramesElapsed{ 0 };
Frame previousFrame{};
Frame currentFrame{};

auto pollStart = millis();
bool ifResetTheState = true;

void loop() {
  if (ifResetTheState) {
    //reset the state
    currentFrame.frameStartLevel = digitalRead(buttonIncrement);
    pollStart = millis();
    ifResetTheState = false;
  }

  if (millis() - pollStart >= frameDurationMs) {
    //process frame
    currentFrame.frameEndLevel = digitalRead(buttonIncrement);

    assignFrameType(currentFrame);

    if (!pressSequenceInProgress) {
      if (previousFrame.frameType == FrameType::STANDBYOFF && currentFrame.frameType == FrameType::OFFTOON)
        pressSequenceInProgress = true;
    } else {
      if (previousFrame.frameType == FrameType::ONTOOFF && currentFrame.frameType == FrameType::STANDBYOFF) {
        pressSequenceInProgress = false;
        incrementDiodeCounter();
      }
    }

    previousFrame = currentFrame;
    ifResetTheState = true;
  }
}
