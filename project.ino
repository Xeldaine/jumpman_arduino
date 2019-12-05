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

//character's possible statuses
const static int STOPPED = 0;
const static int RUNNING = 1;
const static int JUMPING_MIDDLE = 2;
const static int JUMPING_HIGH = 3;
const static int JUMPING_FALLING = 4;

//time between the sprite shift (only used for graphics purposes)
const int runSpeed = 400;

//time duration of a jump
const int jumpSpeed = 200;

//time between each game frame
const int gameSpeed = 100;

//minimum time between each new generation of a terrain
const int terrainGenerationSpeed = 1000;

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

//score
int score;

//true if is not game over
boolean playing;

//method for initializing the game graphics
void initializeGraphics(){
  
  /* Each byte of this array is a piece of an LCD cell.
   * 8 bytes represent the entire cell.
   * Generated using this handy tool: https://maxpromer.github.io/LCD-Character-Creator/
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
    B11000
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

  //define the custom chars and prepare the screen
  initializeGraphics();
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  //set up the character's position
  lcd.setCursor(1,1);

  //draw the character
  lcd.write(byte(SPRITE_RUN1));

  //inizializations
  lastTime = millis();
  lastGenerationTime = millis();
  lastAdvanceTime = millis();
  gameStatus =  STOPPED;
  isOnTheLowerBoard = true;
  isOnTheUpperBoard = false;
  playing = true;
  score = millis()/1000;
}


//update the character's sprite
void updateChar(){
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

void updateCharacterWhileJumping(){
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


//function that shifts both the terrain boards by 1 position to left
void advanceTerrain(int* terrain, int newTerrain){
  for (int i = 0; i < TERRAIN_WIDTH; i++) {
    //if we are in the last position, add the new terrain and add a border, else shift 
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

void loop() {

  //game over
  if(!playing) return;
  
  //read the intensity of the sound
  int soundValue = digitalRead(SOUND_PIN);
  //debug
  //Serial.println(soundValue);

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
  score = millis()/1000;
  updateScore();
  checkIfIsGameOver();
}
