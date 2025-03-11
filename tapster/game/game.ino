#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

#define MODE1_PIN 26
#define MODE2_PIN 27
#define TRIGGER_PIN 25

#include <TFT_eSPI.h> 
TFT_eSPI tft = TFT_eSPI();
#define CENTRE 240

struct CountScores {
  int l_easy;
  int l_normal;
  int l_hard;
  int l_jarnO;
};
CountScores current_l = {0, 0, 0, 0};

int Easy[10];
int Normal[10];
int Hard[10];
int JarnO[10];

int Patterns[24][4] = {
    {1, 2, 3, 4}, {1, 2, 4, 3}, {1, 3, 2, 4}, {1, 3, 4, 2},
    {1, 4, 2, 3}, {1, 4, 3, 2}, {2, 1, 3, 4}, {2, 1, 4, 3},
    {2, 3, 1, 4}, {2, 3, 4, 1}, {2, 4, 1, 3}, {2, 4, 3, 1},
    {3, 1, 2, 4}, {3, 1, 4, 2}, {3, 2, 1, 4}, {3, 2, 4, 1},
    {3, 4, 1, 2}, {3, 4, 2, 1}, {4, 1, 2, 3}, {4, 1, 3, 2},
    {4, 2, 1, 3}, {4, 2, 3, 1}, {4, 3, 1, 2}, {4, 3, 2, 1}
};

struct TapsterGame {
    int cur_pattern[4];
    float randomTimes[4];
};
TapsterGame tapster_instance = {{0,0,0,0}, {0.0,0.0,0.0,0.0}};

bool firstDraw = true;
int isChoosen = false;
int gameMode = 0;
int score = 0;
int penalty = 3;

struct Bar {
    int x, y;
    int width, height;
    int fillHeight;
    float activationTime;
};

Bar gameBars[4] = {
    {67, 50, 75, 200, 0, 0},   // Bar 1
    {157, 50, 75, 200, 0, 0},  // Bar 2
    {247, 50, 75, 200, 0, 0},  // Bar 3
    {337, 50, 75, 200, 0, 0}   // Bar 4
};
int bar_position = 0;
int previous_bar = -1;

void setup() {
    Serial.begin(115200);
    pinMode(MODE1_PIN, INPUT_PULLUP);
    pinMode(MODE2_PIN, INPUT_PULLUP);
    pinMode(TRIGGER_PIN, INPUT_PULLUP);
    tft.init();
    tft.setRotation(3); 
}

void loop() {
    StartNewGame();
    switch (gameMode) {
        case 0:
            Serial.println("Easy");
            TapSter(3.0, 5.0);
            break;
        case 1:
            Serial.println("normal");
            TapSter(2.0, 4.0);
            break;
        case 2:
            Serial.println("hard");
            TapSter(1.5, 3.0);
            break;
        case 3:
            Serial.println("jarn o ");
            TapSter(1.0, 2.0);
            break;
        default:
            Serial.println("No mode selected");
            break;
    }
    ScoreLogs();
    delay(1000);
}

void StartNewGame(){
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("Tapster Game", CENTRE, 20, 2);
    isChoosen = false;

    while(!isChoosen){
      DisplayStartGame();
      if (digitalRead(MODE1_PIN) == LOW){
        if (gameMode<3){
          gameMode+=1;
          Serial.println(gameMode);
        }
        else if (gameMode==3){
          gameMode=0;
        }
        delay(300);
      }
      if (digitalRead(MODE2_PIN) == LOW){
        if (gameMode>0){
          gameMode-=1;
          Serial.println(gameMode);
        }       
        else if (gameMode==0){
          gameMode=3;
        }
        delay(300);
      }
      if (digitalRead(TRIGGER_PIN) == LOW){
        isChoosen = true;
      }
    }
    delay(1000);
    ClearScreen();
}

int RandomPatterns(){
  return random(0,23);
}

float RandomGenerator(float minDelay, float maxDelay) {
    return random((int)(minDelay * 1000), (int)(maxDelay * 1000)) / 1000.0;
}

void SetUpTapSter(float MinVal, float MaxVal) {
    int rand_pattern = RandomPatterns();
    for (int i = 0; i < 4; i++) {
        tapster_instance.cur_pattern[i] = Patterns[rand_pattern][i];
    }

    for (int i = 0; i < 4; i++) {
        tapster_instance.randomTimes[i] = RandomGenerator(MinVal, MaxVal);
    }
}

void TapSter(float MinVal, float MaxVal) {
    bool gameRunning = true;
    while (gameRunning) {
        DisplayMonitor();
        DrawBars(bar_position);
        SetUpTapSter(MinVal, MaxVal);
        Serial.print("Pattern: ");
        for (int i = 0; i < 4; i++) {
            Serial.print(tapster_instance.cur_pattern[i]);
            Serial.print(" ");
        }
        Serial.println();

        for (int i = 0; i < 4; i++) {
            int current_bar = tapster_instance.cur_pattern[i];
            float randomSec = tapster_instance.randomTimes[i];
            Serial.print("Wait for ");
            Serial.print(randomSec, 2);
            Serial.print(" sec... (Bar ");
            Serial.print(current_bar);
            Serial.println(")");
            int startTime = millis();
            float timeElapsed = 0;
            bool triggered = false;
            int lastLoggedSecond = 0;

            while ((millis() - startTime) < (randomSec * 1000)) {
              SwitchBars();
              timeElapsed = (millis() - startTime) / 1000.0;
              int currentSecond = (int)timeElapsed;
              if (currentSecond > lastLoggedSecond) {
                  lastLoggedSecond = currentSecond;
                  Serial.print("Current countdown: ");
                  Serial.println(currentSecond);
              }
              float progress = timeElapsed / randomSec;
              bool is_hover = (bar_position == (current_bar - 1));
              FillTargetBars(current_bar - 1, is_hover, progress);

              if (timeElapsed > (randomSec * 0.8) && digitalRead(TRIGGER_PIN) == LOW) {
                  Serial.println("✔ Correct Timing! +1 Score");
                  score++;
                  triggered = true;
                  ClearFilledBars(current_bar - 1);
                  DisplayMonitor();
                  delay(100);
                  break;
              }

              if (digitalRead(TRIGGER_PIN) == LOW && timeElapsed < (randomSec * 0.8)) {
                  Serial.println("❌ Mistake! Clicked too early! -1 Score");
                  if (penalty > 0) {
                    penalty--;                    
                  }
                  ClearFilledBars(current_bar - 1);
                  DisplayMonitor();
                  delay(100);
                  break;
              }
          }
          ClearFilledBars(current_bar - 1);
          if (penalty == 0) {
              Serial.println("🎉 Game Over! You lost!");
              gameRunning = false;
              ScoreStoring(score, gameMode);
              score = 0;
              penalty = 3;
              isChoosen = false;
              bar_position=0;
              previous_bar=-1;
              firstDraw=true;
              break;
          }
          delay(100);
        }
    }
}

void ScoreStoring(int _score, int _mode ){
      switch (_mode) {
        case 1:
            if (current_l.l_easy < 10) {
                Easy[current_l.l_easy] = _score;
                current_l.l_easy++;
            }
            break;
        case 2:
            if (current_l.l_normal < 10) {
                Normal[current_l.l_normal] = _score;
                current_l.l_normal++;
            }
            break;
        case 3:
            if (current_l.l_hard < 10) {
                Hard[current_l.l_hard] = _score;
                current_l.l_hard++;
            }
            break;
        case 4:
            if (current_l.l_jarnO < 10) {
                JarnO[current_l.l_jarnO] = _score;
                current_l.l_jarnO++;
            }
            break;
        default:
            Serial.println("No mode selected");
            break;
      }  
}

void SortDescending(int arr[], int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (arr[j] < arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

void ScoreLogs() {
    Serial.println("\n🏆 Score Logs 🏆");
    // switch case gameMode to logs specific gameMode each log can moved with these two pin #define MODE1_PIN 26,  #define MODE2_PIN 27 same as previos cases 
    // having in the end exit button going to brake the ScoreLogs and set gameMode = 0 
    if (current_l.l_easy > 0) {
        SortDescending(Easy, current_l.l_easy);
        Serial.println("---- Easy Mode ----");
        for (int i = 0; i < current_l.l_easy; i++) {
            Serial.print("Game ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(Easy[i]);
        }
    } else {
        Serial.println("No games played in Easy Mode.");
    }

    if (current_l.l_normal > 0) {
        SortDescending(Normal, current_l.l_normal);
        Serial.println("---- Normal Mode ----");
        for (int i = 0; i < current_l.l_normal; i++) {
            Serial.print("Game ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(Normal[i]);
        }
    } else {
        Serial.println("No games played in Normal Mode.");
    }

    if (current_l.l_hard > 0) {
        SortDescending(Hard, current_l.l_hard);
        Serial.println("---- Hard Mode ----");
        for (int i = 0; i < current_l.l_hard; i++) {
            Serial.print("Game ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(Hard[i]);
        }
    } else {
        Serial.println("No games played in Hard Mode.");
    }

    if (current_l.l_jarnO > 0) {
        SortDescending(JarnO, current_l.l_jarnO);
        Serial.println("---- JarnO Mode ----");
        for (int i = 0; i < current_l.l_jarnO; i++) {
            Serial.print("Game ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(JarnO[i]);
        }
    } else {
        Serial.println("No games played in JarnO Mode.");
    }

    Serial.println("\n🏁 End of Score Logs 🏁\n");
}

void ClearScreen() {
    tft.fillScreen(TFT_BLACK);  // Clear the display
}

void DisplayStartGame(){
    String modes[] = {"Easy", "Normal", "Hard", "Jarn O"};
    int previousGameMode = -1;
    for (int i = 0; i < 4; i++) {
      tft.setTextSize(2);
      if (gameMode != previousGameMode) {  
        for (int i = 0; i < 4; i++) {
            if (i == gameMode) {
                tft.setTextColor(TFT_RED, TFT_RED);
            } else {
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
            }
            int textWidth = tft.textWidth(modes[i], 2)-20;
            int textHeight = 8 * 4;
            int xStart = (SCREEN_WIDTH - textWidth) / 2;
            int yUnderline =  82 + textHeight + (i * 60);
            tft.drawCentreString(modes[i], CENTRE, 80 + (i * 60), 2);
            if (i == gameMode) {
                for (int thickness = 0; thickness < 3; thickness++) {
                    tft.drawLine(CENTRE - textWidth, yUnderline + thickness, 
                    CENTRE + textWidth, yUnderline + thickness, TFT_RED);
                }
            } else {
                for (int thickness = 0; thickness < 3; thickness++) {
                    tft.drawLine(CENTRE - textWidth, yUnderline + thickness, 
                    CENTRE + textWidth, yUnderline + thickness, TFT_BLACK);
                }
            }
        }
        previousGameMode = gameMode;
      }
    }
}

void FillTargetBars(int current_bar, bool is_hover, float progress) {
    if (progress < 0.0) progress = 0.0;
    if (progress > 1.0) progress = 1.0;

    int percentage = (int)(progress * 100);
    int stepHeight = gameBars[current_bar].height / 5;
    int currentStep = 0;

    switch (percentage) {
        case 0 ... 19:
            currentStep = 1;
            break;
        case 20 ... 39:
            currentStep = 2;
            break;
        case 40 ... 59:
            currentStep = 3;
            break;
        case 60 ... 79:
            currentStep = 4;
            break;
        case 80 ... 100:
            currentStep = 5;
            break;
        default:
            break;
    }
    tft.fillRect(gameBars[current_bar].x + 1, 
                 gameBars[current_bar].y + 1, 
                 gameBars[current_bar].width - 2, 
                 gameBars[current_bar].height - 2, 
                 TFT_BLACK);
    int startY = gameBars[current_bar].y + (stepHeight * (currentStep - 1));
    tft.fillRect(gameBars[current_bar].x + 1, startY, 
                 gameBars[current_bar].width - 2, stepHeight, 
                 is_hover ? TFT_RED : TFT_WHITE);
    delay(100);
}

// old switchbar
// void SwitchBars(){
//     int new_position = bar_position;
//     if (digitalRead(MODE1_PIN) == LOW){
//         new_position = (bar_position + 1) % 4;
//         delay(100);
//     }
//     if (digitalRead(MODE2_PIN) == LOW){
//         new_position = (bar_position - 1 + 4) % 4;
//         delay(100);
//     }

//     if (new_position != bar_position) {
//         bar_position = new_position;
//         DrawBars(bar_position);
//     }
// }


void SwitchBars() {
    int new_position = bar_position;
    
    if (digitalRead(MODE1_PIN) == LOW) {
        new_position = (bar_position + 1) % 4;  // Move to the next bar
        delay(100);
    }
    if (digitalRead(MODE2_PIN) == LOW) {
        new_position = (bar_position - 1 + 4) % 4;  // Move to the previous bar
        delay(100);
    }

    if (new_position != bar_position) {
        ClearUpArrow(gameBars[bar_position].x + (gameBars[bar_position].width / 2), 
                   gameBars[bar_position].y + gameBars[bar_position].height + 5, 
                   15);
        bar_position = new_position;
        DrawBars(bar_position);
        DrawUpArrow(gameBars[bar_position].x + (gameBars[bar_position].width / 2), 
                    gameBars[bar_position].y + gameBars[bar_position].height + 5, 
                    15, TFT_RED);
    }
}

void ClearFilledBars(int current_bar){
    tft.fillRect(gameBars[current_bar].x + 1, 
                 gameBars[current_bar].y + 1, 
                 gameBars[current_bar].width - 2, 
                 gameBars[current_bar].height - 2, 
                 TFT_BLACK);
}

void DrawBars(int c_bar) {
    if (firstDraw) {  
        for (int i = 0; i < 5; i++) {
            DrawThickRect(gameBars[i].x, gameBars[i].y, gameBars[i].width, gameBars[i].height, TFT_WHITE, 3);
            tft.fillRect(gameBars[i].x, gameBars[i].y + 
                         (gameBars[i].height - gameBars[i].fillHeight), 
                         gameBars[i].width, gameBars[i].fillHeight, TFT_RED);
        }
        firstDraw = false;
    }
    if (previous_bar != -1 && previous_bar != c_bar) { 
        DrawThickRect(gameBars[previous_bar].x, gameBars[previous_bar].y, 
                     gameBars[previous_bar].width, gameBars[previous_bar].height, TFT_WHITE, 3);
    }
    DrawThickRect(gameBars[c_bar].x, gameBars[c_bar].y, gameBars[c_bar].width, gameBars[c_bar].height, TFT_RED, 3);
    previous_bar = c_bar;
}

void DrawThickRect(int x, int y, int w, int h, uint16_t color, int thickness) {
    for (int i = 0; i < thickness; i++) {
        tft.drawRect(x - i, y - i, w + (2 * i), h + (2 * i), color);
    }
}

void DisplayMonitor() {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.fillRect(10, 5, 280, 20, TFT_BLACK);
    tft.setCursor(10, 5);
    tft.print("Score: ");
    tft.print(score);

    int heartSize = 20;
    int startX = 160;
    int startY = 5;  
    int spacing = 15;

    for (int i = 0; i < penalty; i++) {
        int x = startX + (i * spacing);
        DrawHeart(x, startY, heartSize, TFT_RED);
    }
}

void DrawHeart(int x, int y, int size, uint16_t color) {
    int radius = size / 4;
    tft.fillCircle(x - radius, y + radius, radius, color);
    tft.fillCircle(x + radius, y + radius, radius, color);
    tft.fillTriangle(x - size / 2, y + radius, 
                     x + size / 2, y + radius, 
                     x, y + size, color);
}

void DrawUpArrow(int x, int y, int size, uint16_t color) {
    tft.fillTriangle(x - size, y, 
                     x + size, y, 
                     x, y - (size * 1.5), color);  // **Increase arrow height**
}

void ClearUpArrow(int x, int y, int size) {
    // **Erase arrow by drawing it in black**
    tft.fillTriangle(x - size, y, 
                     x + size, y, 
                     x, y - (size * 1.5), TFT_BLACK);
}
