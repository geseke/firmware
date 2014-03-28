//////////////////////////////////////////////////////////////////////////////
// Knitty Project
//
// Author: ptflea, schinken
//

#define INT_0         0
#define INT_ENCODER   INT_0

//////////////////////////////////////////////////////////////////////////////
// General purpose definitions

#define PIN_IFDR      3          // Green
#define PIN_CSENSE    2          // Yellow
#define PIN_CREF      4          // White
#define PIN_SEL       9          // Blue,  Pattern
#define PIN_NEEDLE    PIN_SEL

#define DIRECTION_UNKNOWN       0
#define DIRECTION_LEFT_RIGHT   -1
#define DIRECTION_RIGHT_LEFT    1

char currentDirection = DIRECTION_UNKNOWN;
char lastDirection = DIRECTION_UNKNOWN;

signed int currentCursorPosition = -15;
signed int leftEndCursorPosition = 0;
unsigned int currentPatternIndex = 0;

unsigned char knitPattern[255] = {0};

//////////////////////////////////////////////////////////////////////////////
// Knitty Serial Protocol

// Receive commands
#define COM_CMD_PATTERN      'P'
#define COM_CMD_CURSOR       'C'
#define COM_CMD_SEPERATOR    ':'

// Send commands
#define COM_CMD_PATTERN_END  'E'
#define COM_CMD_DIRECTION    'D'
#define COM_CMD_DEBUG        'V'

#define COM_CMD_PLOAD_END    '\n'

// Parser states
#define COM_PARSE_CMD      0x01
#define COM_PARSE_SEP      0x02
#define COM_PARSE_PLOAD    0x03


unsigned char parserState = COM_PARSE_CMD;

unsigned char parserReceivedCommand = 0;
char parserReceivedPayload[255] = {0};
size_t parserReceivedBytes = 0;


unsigned char patternLength = 0;


void setup() {
  Serial.begin(115200);
  Serial.println("Hallo Knitty, hello World!");

  // Setup PHOTO SENSOR pin change interrupt
  pinMode(PIN_CSENSE, INPUT);
  pinMode(PIN_CREF, INPUT);
  attachInterrupt(INT_ENCODER, interruptPinChangeEncoder, CHANGE);
  
  // Setup Needle
  pinMode(PIN_NEEDLE, OUTPUT);  
  digitalWrite(PIN_NEEDLE, LOW);
}

void executeCommand(unsigned char cmd, char *payload, size_t length) {
  
  switch(cmd) {
    case COM_CMD_PATTERN:
      memset(knitPattern, 0,255);
      
      for(unsigned char i = 0; i < length; i++) {
        knitPattern[i] = (payload[i] == '1')? 1 : 0;
      }
      
      patternLength = length;
      
      break;
      
    case COM_CMD_CURSOR:
      currentCursorPosition = atoi(payload);
      break;
  }
}

void parserSerialStream() {
  
  if (Serial.available() == 0) {
    return;
  }
  
  char buffer = Serial.read();
    
  switch(parserState) {
      
    case COM_PARSE_CMD:
        if(buffer == COM_CMD_PATTERN || buffer == COM_CMD_CURSOR) {
          parserState = COM_PARSE_SEP;
          parserReceivedCommand = buffer;
          parserReceivedBytes = 0;
        }
      break;
      
    case COM_PARSE_SEP:
      
      // We're awaiting a seperator here, if not, back to cmd
      if(buffer == COM_CMD_SEPERATOR) {
        parserState = COM_PARSE_PLOAD;
        break;
      }
        
      parserState = COM_PARSE_CMD;
      break;
        
    case COM_PARSE_PLOAD:
      
      if(buffer == COM_CMD_PLOAD_END) {
        // Everything is read, execute command
        parserReceivedPayload[parserReceivedBytes+1] = '\0';
        
        executeCommand(parserReceivedCommand, parserReceivedPayload, parserReceivedBytes);
        parserState = COM_PARSE_CMD;
        
        Serial.println("OK");
        break;
      }
        
      parserReceivedPayload[parserReceivedBytes] = buffer;
      parserReceivedBytes++;
          
      break;        
  } 
}

void loop() {
  parserSerialStream();
}

void setNeedleByCursor(char cursorPosition) {
 
  // Just to be sure that we never exceed the pattern
  if(cursorPosition > patternLength-1 && cursorPosition < 0) {
    return;
  }
 
  setNeedle(knitPattern[cursorPosition]);
} 

void setNeedle(char state) {  
  if(state == 1) {
    digitalWrite(PIN_NEEDLE, HIGH);
  } else {
    digitalWrite(PIN_NEEDLE, LOW); 
  }
}

void interruptPinChangeEncoder() {
  
  char currentPinChangeValue = digitalRead(PIN_CSENSE);
  char currentPinChangeOppositeValue = digitalRead(PIN_CREF);
  
  // Determine direction
  if(currentPinChangeValue == currentPinChangeOppositeValue) {
    currentDirection = DIRECTION_LEFT_RIGHT;
  } else {
    currentDirection = DIRECTION_RIGHT_LEFT;
  }
  
  // RTL = 1, LTR = -1
  currentCursorPosition += currentDirection; 
  
  // Check if we're in range of our needles
  if((currentDirection == DIRECTION_RIGHT_LEFT && currentCursorPosition > 0) ||
     (currentDirection == DIRECTION_LEFT_RIGHT && currentCursorPosition <= leftEndCursorPosition)) {

    if(currentPatternIndex >= patternLength-1) {
         
       setNeedle(0);
       currentPatternIndex = 0;
       
       if(currentDirection == DIRECTION_RIGHT_LEFT) {
         leftEndCursorPosition = currentCursorPosition-1;
       }
       
    } else {
          
      if(currentDirection == DIRECTION_RIGHT_LEFT && currentPinChangeValue == 0) {
        setNeedleByCursor(currentPatternIndex);
        currentPatternIndex++;
      }
      
      if(currentDirection == DIRECTION_LEFT_RIGHT && currentPinChangeValue == 1) {
        setNeedleByCursor(currentPatternIndex);
        currentPatternIndex++;
      }
    }
  }
  
  if(lastDirection != currentDirection) {
    lastDirection = currentDirection;
    currentPatternIndex = 0;
  }
}
