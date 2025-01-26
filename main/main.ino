// founding assumptions:
// + The press state does not change in between the frames
// + The frame duration is much shorter than one can press and release a button
// + The state of button will eventually stabilize over time when it is not pressed

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
constexpr size_t minPressSequenceExitConfirmationFrames = 3;
constexpr size_t minPressSequenceEnterConfirmationFrames = 8;

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

void incrementLedCounter() {
  static uint8_t currentCount = 0;
  ++currentCount;

  Serial.print("+++++ [Count to display] +++++ ");
  Serial.println(currentCount);

  int leds[] = { led1, led2, led3, led4 };
  for (int i = 0; i < numLeds; ++i) {
    digitalWrite(leds[i], (currentCount >> i) & 0x01);
  }
}

void toggleLedsOn() {
  Serial.println("+++++ [Toggling LEDs on] +++++");

  int leds[] = { led1, led2, led3, led4 };
  for (int i = 0; i < numLeds; ++i) {
    digitalWrite(leds[i], HIGH);
  }
}

void toggleLedsOff() {
  Serial.println("+++++ [Toggling LEDs off] +++++");

  int leds[] = { led1, led2, led3, led4 };
  for (int i = 0; i < numLeds; ++i) {
    digitalWrite(leds[i], LOW);
  }
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
}

struct ButtonState {
  Frame previousFrame{};
  Frame currentFrame{};

  bool pressSequenceInProgress = false;
  bool startDetected = false;

  size_t exitFramesElapsed{ 0 };
  size_t enterFramesElapsed{ 0 };
};

using PressAction = void (*)(void);

void processButtonInput(ButtonState &buttonState, PressAction onPressStartAction, PressAction onPressEndAction) {
  if (!buttonState.pressSequenceInProgress) {  //only check if press sequence can be started
    if (buttonState.previousFrame.frameType == FrameType::STANDBYOFF && buttonState.currentFrame.frameType != FrameType::STANDBYOFF) {
      Serial.println("----- [Transition from low #0] -----");
      buttonState.pressSequenceInProgress = true;
    }
  } else {
    switch (buttonState.currentFrame.frameType) {
      case FrameType::STANDBYOFF:
        ++buttonState.exitFramesElapsed;
        buttonState.enterFramesElapsed = 0;
        break;
      case FrameType::STANDBYON:
      case FrameType::OFFTOON:
      case FrameType::ONTOOFF:
        ++buttonState.enterFramesElapsed;
        buttonState.exitFramesElapsed = 0;
        break;
    }

    if (buttonState.enterFramesElapsed >= minPressSequenceEnterConfirmationFrames && !buttonState.startDetected) {
      Serial.println("----- [Press sequence start detected #1] -----");
      buttonState.startDetected = true;
      if (onPressStartAction != nullptr)
        onPressStartAction();
    }

    if (buttonState.exitFramesElapsed >= minPressSequenceExitConfirmationFrames) {
      Serial.println("----- [Press sequence end detected #2] -----");

      if (onPressEndAction != nullptr && buttonState.startDetected)
        onPressEndAction();

      buttonState.pressSequenceInProgress = false;
      buttonState.exitFramesElapsed = 0;
      buttonState.enterFramesElapsed = 0;
      buttonState.startDetected = 0;
    }
  }

  buttonState.previousFrame = buttonState.currentFrame;
}

//frame state
auto pollStart = millis();

ButtonState incrementButtonState{};
ButtonState toggleButtonState{};

void loop() {

  if (millis() - pollStart >= frameDurationMs) {
    //process frame
    incrementButtonState.currentFrame.frameEndLevel = digitalRead(buttonIncrement);
    assignFrameType(incrementButtonState.currentFrame);

    // toggleButtonState.currentFrame.frameEndLevel = digitalRead(buttonToggle);
    // assignFrameType(toggleButtonState.currentFrame);

    processButtonInput(incrementButtonState, nullptr, &incrementLedCounter);
    // processButtonInput(toggleButtonState, &toggleLedsOn, &toggleLedsOff);

    incrementButtonState.currentFrame.frameStartLevel = digitalRead(buttonIncrement);
    // toggleButtonState.currentFrame.frameStartLevel = digitalRead(buttonToggle);

    pollStart = millis();
  }
}
