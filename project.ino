#include <LiquidCrystal.h>

//initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//indexes of the custom characters
#define SPRITE_RUN1 1
#define SPRITE_RUN2 2
#define SPRITE_JUMP 3
#define SPRITE_UPPER_JUMP 4
#define SPRITE_LOWER_JUMP 5

//terrain constants
#define TERRAIN_WIDTH 16
#define SPRITE_TERRAIN_EMPTY 0
#define GROUND 6
#define GROUND_RIGHT 7
#define GROUND_LEFT 8

//game speed value for each difficulty
#define EASY_GAME_SPEED 200
#define MEDIUM_GAME_SPEED 100
#define HARD_GAME_SPEED 50

//terrain generation speed value for each difficulty
#define EASY_GENERATION_SPEED 800
#define MEDIUM_GENERATION_SPEED 600
#define HARD_GENERATION_SPEED 400

//character's possible statuses
const static int STOPPED = 0;
const static int RUNNING = 1;
const static int JUMPING_MIDDLE = 2;
const static int JUMPING_HIGH = 3;
const static int JUMPING_FALLING = 4;

//time between the sprite shift (only used for graphics purposes)
const int runSpeed = 400;

//time duration of a jump
int jumpSpeed = EASY_GAME_SPEED;
//time between each game frame
int gameSpeed = EASY_GAME_SPEED;

//minimum time between each new generation of a terrain
int terrainGenerationSpeed = EASY_GENERATION_SPEED;

//variable containing the time in millisec. of the last update
unsigned long lastTime;

//variable containing the time in millisec. of the last terrain generation
unsigned long lastGenerationTime;

//variable containing the time in millisec. of the last terrain advance
unsigned long lastAdvanceTime;

//variables used to monitor the character's status
int gameStatus;
boolean isOnTheLowerBoard;
boolean isOnTheUpperBoard;

//array that models the status of the upper part of the display
static int terrainUpper[TERRAIN_WIDTH];

//array that models the status of the lower part of the display
static int terrainLower[TERRAIN_WIDTH];

//default sound sensor pin
const int SOUND_PIN = 13;

//default buzzer pin
const int BUZZER_PIN = 10;

//score
int score;

//true if is not game over
boolean playing;

//game over track constants
const int GAME_OVER_TRACK_LENGTH = 3;
int gameOverTrack[] = {1245,622,311};
int gameOverTrackTempo[] = {500, 500, 500};
int gameOverTrackPauses[] = {200, 200, 200};


//method for initializing the game graphics
void initializeGraphics(){
  
  /* Each byte of this array is a piece of an LCD cell.
   * 8 bytes represent the entire 5x8 cell
   * Generated using this tool: https://maxpromer.github.io/LCD-Character-Creator/
   */
  static byte graphics[] = {
    // Run position 1
    B01100,
    B01100,
    B00000,
    B01110,
    B11100,
    B01100,
    B11010,
    B10011,
    // Run position 2
    B01100,
    B01100,
    B00000,
    B01100,
    B01100,
    B01100,
    B01100,
    B01110,
    // Jump
    B01100,
    B01100,
    B00000,
    B11110,
    B01101,
    B11111,
    B10000,
    B00000,
    // Upper Jump
    B00000,
    B00000,
    B00000,
    B00000,
    B01100,
    B01100,
    B00000,
    B11110,
    // Lower Jump
    B01101,
    B11111,
    B10000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    // Ground
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    // Ground right
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    // Ground left
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
  };
    
  int i;
  for (i = 0; i < 8; i++) {
    lcd.createChar(i + 1, &graphics[i * 8]);
  }
  for (i = 0; i < TERRAIN_WIDTH; i++) {
    terrainUpper[i] = SPRITE_TERRAIN_EMPTY;
    terrainLower[i] = SPRITE_TERRAIN_EMPTY;
  }
}

void setup() {

  //debug
  Serial.begin(115200);
  
  //set up the sound sensor input pin
  pinMode(SOUND_PIN, INPUT);

  //set up the buzzer output pin
  pinMode(BUZZER_PIN,OUTPUT);

  //define the custom chars and prepare the screen
  initializeGraphics();
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  //set up the character's position
  lcd.setCursor(1,1);

  //draw the character
  lcd.write(byte(SPRITE_RUN1));

  //initializations
  lastTime = millis();
  lastGenerationTime = millis();
  lastAdvanceTime = millis();
  gameStatus =  STOPPED;
  isOnTheLowerBoard = true;
  isOnTheUpperBoard = false;
  playing = true;
  score = millis()/1000;
}

void updateChar(){
  /*
 * this function updates 
 * the character's sprite
 * periodically, giving
 * the "running" effect
 */
  if (gameStatus == RUNNING ){
    lcd.setCursor(1,1);
    lcd.write(byte(SPRITE_RUN1));
    gameStatus = STOPPED;
  }
  else if(gameStatus == STOPPED){
    lcd.setCursor(1,1);
    lcd.write(byte(SPRITE_RUN2));
    gameStatus = RUNNING;
  }
  lastTime = millis();
}


boolean isContinuingJumping = false;
/*
 * since the sound sensor isn't too
 * precise, if the sound is recorded
 * even once within a brief timelapse,
 * this code keeps the character on 
 * the upper part of the board. The
 * boolean allows to keep track of that.
 */
void updateCharacterWhileJumping(){
  /*
   * this function decides which animation
   * to perform basing on the state of the
   * character when it is on the upper part
   * of the board
   */
    if(gameStatus == STOPPED || gameStatus == RUNNING){
      startJumpingUpAnimation();
    }
    else if(gameStatus == JUMPING_MIDDLE){
      unsigned long currentMillis = millis();
      if(currentMillis-lastTime >= gameSpeed){
        startJumpingAnimation();
      }
    }
    else if(gameStatus == JUMPING_HIGH){
      //continue jumping
      isContinuingJumping = true;
    }
    else if(gameStatus == JUMPING_FALLING){
      unsigned long currentMillis = millis();
      if(currentMillis-lastTime >= gameSpeed){
        resetRunning();
      }
    }
}

void updateCharacterWhileGrounded(){
  /*
   * this function decides which animation
   * to perform basing on the state of the
   * character when it is on the lower part
   * of the board
   */
    if(gameStatus == STOPPED){
      unsigned long currentMillis = millis();
      if(currentMillis-lastTime >= runSpeed){
        updateChar();
      }
    }
    else if(gameStatus == RUNNING){
      unsigned long currentMillis = millis();
      if(currentMillis-lastTime >= runSpeed){
        updateChar();
      }
    }
    else if(gameStatus == JUMPING_MIDDLE){
      unsigned long currentMillis = millis();
      if(currentMillis-lastTime >= gameSpeed){
        startJumpingAnimation();
      }
    }
    else if(gameStatus == JUMPING_HIGH){
      unsigned long currentMillis = millis();
      if(currentMillis-lastTime >= jumpSpeed){
        if(isContinuingJumping){
          isContinuingJumping = false;
          lastTime = millis();
        }
        else endJumpingAnimation();
      }
    }
    else if(gameStatus == JUMPING_FALLING){
      unsigned long currentMillis = millis();
      if(currentMillis-lastTime >= gameSpeed){
        resetRunning();
      }
    }
}

//starts the jump animation
void startJumpingUpAnimation(){
  lastTime = millis();
  gameStatus = JUMPING_MIDDLE;
  isOnTheLowerBoard = true;
  isOnTheUpperBoard = true;
  lcd.setCursor(1,0);
  lcd.write(byte(SPRITE_UPPER_JUMP));
  lcd.setCursor(1,1);
  lcd.write(byte(SPRITE_LOWER_JUMP));
}

//keeps the character on air
void startJumpingAnimation(){
  lastTime = millis();
  gameStatus = JUMPING_HIGH;
  isOnTheLowerBoard = false;
  isOnTheUpperBoard = true;
  lcd.setCursor(1,1);
  lcd.write(' ');
  lcd.setCursor(1,0);
  lcd.write(byte(SPRITE_JUMP));
}

//makes the character fall 
void endJumpingAnimation(){
  lastTime = millis();
  gameStatus = JUMPING_FALLING;
  isOnTheLowerBoard = true;
  isOnTheUpperBoard = true;
  lcd.setCursor(1,0);
  lcd.write(byte(SPRITE_UPPER_JUMP));
  lcd.setCursor(1,1);
  lcd.write(byte(SPRITE_LOWER_JUMP));
}

//restores the running state of the character
void resetRunning(){
  lastTime = millis();
  isOnTheLowerBoard = true;
  isOnTheUpperBoard = false;
  lcd.setCursor(1,0);
  lcd.write(' ');
  gameStatus = RUNNING;
  lcd.setCursor(1,1);
  lcd.write(byte(SPRITE_RUN1));
}

//function that randomly generates a new terrain 
int generateTerrain(){
  unsigned long currentTime = millis();
  if(currentTime - lastGenerationTime >= terrainGenerationSpeed){
    //random generate one among the 2 terrain blocks
    lastGenerationTime = millis();
    int r = random(2);
    if(r==0){
      return SPRITE_TERRAIN_EMPTY;
    }
    else return GROUND;
  }
  else return -1; //it is not the time for a new terrain yet
}


//function that shifts both the terrain boards by 1 position to the left
void advanceTerrain(int* terrain, int newTerrain){
  for (int i = 0; i < TERRAIN_WIDTH; i++) {
    //if we are in the last position, add the new terrain, else shift 
    terrain[i] = (i == TERRAIN_WIDTH - 1)? newTerrain : terrain[i+1];
  }

  //border fixes
  if(terrain[TERRAIN_WIDTH-1]==GROUND) terrain[TERRAIN_WIDTH-2] = GROUND_RIGHT;
  if(terrain[TERRAIN_WIDTH - 2] == GROUND){
    terrain[TERRAIN_WIDTH - 3] = GROUND_RIGHT;
    terrain[TERRAIN_WIDTH -1] = GROUND_LEFT;
  } 
}

//function that randomly decides how to update the boards
void updateTerrain(){
  
  int newTerrainYAxis = random(2);
  int newTerrain = generateTerrain();

  //it is not the time yet
  if(newTerrain ==-1) {
    advanceTerrain(terrainLower, SPRITE_TERRAIN_EMPTY);
    advanceTerrain(terrainUpper, SPRITE_TERRAIN_EMPTY);
  }
  //update the lower board
  else if(newTerrainYAxis == 0){
    advanceTerrain(terrainLower, newTerrain);
    advanceTerrain(terrainUpper, SPRITE_TERRAIN_EMPTY);
  }
  else{ //update the upper board
    advanceTerrain(terrainLower, SPRITE_TERRAIN_EMPTY);
    advanceTerrain(terrainUpper, newTerrain);
  }
}

//fuction that renders the 2 boards on the LCD
void renderBoard(int* terrainLow, int* terrainUp){
  for(int i = 0; i < TERRAIN_WIDTH; i++){
    //if we are in the character position
    if(i == 1 && isOnTheLowerBoard && isOnTheUpperBoard){
      //do nothing
      continue;
    }
    else if(i == 1 && isOnTheLowerBoard){
      //update only the upper board
      lcd.setCursor(i,0);
      if(terrainUp[i]==SPRITE_TERRAIN_EMPTY) lcd.write(' ');
      else lcd.write(byte(terrainUp[i]));
    }
    else if(i == 1 && isOnTheUpperBoard){
      //update only the lower board
      lcd.setCursor(i,1);
      if(terrainLow[i]==SPRITE_TERRAIN_EMPTY) lcd.write(' ');
      else lcd.write(byte(terrainLow[i]));
    }
    else{
      //update both
      lcd.setCursor(i,0);
      if(terrainUp[i]==SPRITE_TERRAIN_EMPTY) lcd.write(' ');
      else lcd.write(byte(terrainUp[i]));
      lcd.setCursor(i,1);
      if(terrainLow[i]==SPRITE_TERRAIN_EMPTY) lcd.write(' ');
      else lcd.write(byte(terrainLow[i]));
    }
  }
}

void updateScore(){
  int ratio = 0;
  int temp = score;
  while(temp >= 10){
    temp = temp/10;
    ratio+=1;
  }
  lcd.setCursor(TERRAIN_WIDTH - 1 - ratio, 0);
  lcd.print(score);
}

void checkIfIsGameOver(){
  if(isOnTheLowerBoard){
    if(terrainLower[1]==GROUND) playing = false;
  }
  if(isOnTheUpperBoard){
    if(terrainUpper[1]==GROUND) playing = false;
  }
}

void printGameOver(){
  delay(500);
  lcd.clear();
  lcd.print("Game over!");
  lcd.setCursor(0,1);
  lcd.print("Score: ");
  lcd.print(score);
}

void startGameOverTrack(){
  for(int i=0; i < GAME_OVER_TRACK_LENGTH; i++){
    tone(BUZZER_PIN, gameOverTrack[i], gameOverTrackTempo[i]);
    delay(gameOverTrackPauses[i]);
  }
}

//keeps track of the last reset time
unsigned long previousMillis = 0;

//used to write only once to the LCD when game over is reached
boolean alreadyPrintedGameOver = false;

//called when the game has been restarted
void restartTheGame(){
  //clear the screen
  lcd.clear();

  //draw the character
  lcd.write(byte(SPRITE_RUN1));

  //restores the graphics
  initializeGraphics();
  
  //initializations
  previousMillis = millis();
  lastTime = millis() - previousMillis;
  lastGenerationTime = millis()- previousMillis;
  lastAdvanceTime = millis()- previousMillis;
  gameStatus =  STOPPED;
  isOnTheLowerBoard = true;
  isOnTheUpperBoard = false;
  playing = true;
  alreadyPrintedGameOver = false;
  score = 0;
  gameSpeed = EASY_GAME_SPEED;
  jumpSpeed = EASY_GAME_SPEED;
  terrainGenerationSpeed = EASY_GENERATION_SPEED;
}

void loop() {

  //read the intensity of the sound
  int soundValue = digitalRead(SOUND_PIN);
  
  //game over condition
  if(!playing) {
    if(!alreadyPrintedGameOver){
      printGameOver();
      alreadyPrintedGameOver = true;
      startGameOverTrack();
      delay(1000); //used to avoid that previous sound activates the sensor
      return;
    }
    else{
      if(soundValue == 1){
        restartTheGame();
      }
      return;
    }
  }

  //manage the run and jump animations of the character according to the recorded sound
  if(soundValue == 1){
    updateCharacterWhileJumping();
  }
  else{
    updateCharacterWhileGrounded();
  }

  //if it is the time, update the board shifting the terrain left and creating a new block
  long currentTime = millis();
  if(currentTime - lastAdvanceTime >= gameSpeed){
    updateTerrain();
    renderBoard(terrainLower, terrainUpper);
    lastAdvanceTime = millis();
  }

  //score updating
  score = (millis()-previousMillis)/1000;
  if(score == 20){
    //difficulty shifts to medium
    gameSpeed = MEDIUM_GAME_SPEED;
    jumpSpeed = MEDIUM_GAME_SPEED;
    terrainGenerationSpeed = MEDIUM_GENERATION_SPEED;
  }
  else if (score == 50){
    //difficulty shifts to hard
    gameSpeed = HARD_GAME_SPEED;
    jumpSpeed = HARD_GAME_SPEED;
    terrainGenerationSpeed = HARD_GENERATION_SPEED;
  }
  updateScore();
  checkIfIsGameOver();
}
