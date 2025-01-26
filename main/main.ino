// founding assumptions:
// + The press state does not change in between the frames
// + The frame duration is much shorter than one can press and release a button -> if such does occur, then it is definitely noise
// + The state of button will eventually stabilize over time when it is not pressed

enum class FrameType {
  STANDBYOFF,  //the frame began and ended with button released
  STANDBYON,   //the frame began and ended with button pressed
  OFFTOON,     //transition from released to pressed occurred during frame
  ONTOOFF,     //transition from pressed to released occurred during frame
};

struct Frame {
  int frameStartLevel{ 0 };
  int frameEndLevel{ 0 };
  FrameType frameType{ FrameType::STANDBYOFF };
};

//this should be fine-tuned
constexpr size_t frameDurationMs = 40;
constexpr size_t minPressSequenceExitConfirmationFrames = 5;
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
  pinMode(buttonIncrement, INPUT);
  pinMode(buttonToggle, INPUT);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);

  Serial.begin(9600);
  Serial.println("Hello) I hate when someone plugs buttons in me.");
}

void incrementLedCounter() {
  static uint8_t currentCount = 0;
  ++currentCount;

  Serial.println(String("+++++ [Count to display:") + String(currentCount) + String(" ] +++++ "));

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

//determines what happened during the frame
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

struct ButtonStateStatistics {
  size_t numOns{ 0 };
  size_t numOffs{ 0 };
  int pressStart{ 0 };
};

//keeps track of the state of the button
struct ButtonState {
  Frame previousFrame{};
  Frame currentFrame{};

  bool pressSequenceInProgress = false;
  bool startDetected = false;

  size_t exitFramesElapsed{ 0 };
  size_t enterFramesElapsed{ 0 };

  ButtonStateStatistics stats{};
};

using PressAction = void (*)(void);

//this function essentilly does debouncing
void processButtonInput(ButtonState &buttonState, PressAction onPressStartAction, PressAction onPressEndAction) {
  if (!buttonState.pressSequenceInProgress) {  //only check if press sequence can be started
    if (buttonState.previousFrame.frameType == FrameType::STANDBYOFF && buttonState.currentFrame.frameType != FrameType::STANDBYOFF) {
      Serial.println("\n----- [Transition from low to noise #0] -----");
      buttonState.pressSequenceInProgress = true;
      buttonState.stats.pressStart = millis();
    }
  } else {
    switch (buttonState.currentFrame.frameType) {
      case FrameType::STANDBYOFF:
        ++buttonState.exitFramesElapsed;
        buttonState.enterFramesElapsed = 0;
        if (buttonState.previousFrame.frameType == FrameType::STANDBYON)
          ++buttonState.stats.numOffs;
        break;
      case FrameType::OFFTOON:
        ++buttonState.stats.numOns;
      case FrameType::ONTOOFF:
        ++buttonState.stats.numOffs;
      case FrameType::STANDBYON:
        ++buttonState.enterFramesElapsed;
        buttonState.exitFramesElapsed = 0;
        if (buttonState.previousFrame.frameType == FrameType::STANDBYOFF)
          ++buttonState.stats.numOns;
        break;
    }

    //below we determine if the button has indeed been pressed
    if (buttonState.enterFramesElapsed >= minPressSequenceEnterConfirmationFrames && !buttonState.startDetected) {
      Serial.println("\n----- [Press sequence start detected #1] -----");
      buttonState.startDetected = true;
      if (onPressStartAction != nullptr)
        onPressStartAction();
    }

    //and here we determine if its digital level has already stabilized to infer the end of press
    if (buttonState.exitFramesElapsed >= minPressSequenceExitConfirmationFrames) {
      Serial.println("\n----- [Press sequence end detected #2] -----");

      Serial.println("\n????? Statistics ?????");
      if (onPressEndAction != nullptr && buttonState.startDetected) {
        onPressEndAction();
        Serial.println(String("===== [Total press duration: ") + String(millis() - buttonState.stats.pressStart) + String("] ====="));
      }

      Serial.println(String("===== [Perceived button presses: ") + String(buttonState.stats.numOns) + String("] ====="));
      Serial.println(String("===== [Perceived button releases: ") + String(buttonState.stats.numOffs) + String("] ====="));

      buttonState.pressSequenceInProgress = false;
      buttonState.exitFramesElapsed = 0;
      buttonState.enterFramesElapsed = 0;
      buttonState.startDetected = 0;
      buttonState.stats = ButtonStateStatistics{};
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
    //records the end levels
    incrementButtonState.currentFrame.frameEndLevel = digitalRead(buttonIncrement);
    assignFrameType(incrementButtonState.currentFrame);

    toggleButtonState.currentFrame.frameEndLevel = digitalRead(buttonToggle);
    assignFrameType(toggleButtonState.currentFrame);

    //updates the buttons
    processButtonInput(incrementButtonState, nullptr, &incrementLedCounter);
    processButtonInput(toggleButtonState, &toggleLedsOn, &toggleLedsOff);

    //records the start levels
    incrementButtonState.currentFrame.frameStartLevel = digitalRead(buttonIncrement);
    toggleButtonState.currentFrame.frameStartLevel = digitalRead(buttonToggle);

    pollStart = millis();
  }
}
