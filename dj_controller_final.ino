/* Dj controller
 *  
 *  for arduino mega 2560
 *  
 *  created 2015
 *  
 *  designed and writen by ASH
 *  
 *  taalabash@gmail.com
 *  
 *  this frimware can hanle number of buttons that can be changed by a mode key
 *  2 shift buttons to change hot cue mode for each deck
 *   
 */

#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

//_____________// MIDI ////__________________________________________________________________________________________________________

//0, 1, 2, 3, 4, are token for mode keys don't use them ... !!!! hot cues , loops , slicer , rolls , samplers
//5 for mode shift keys  and shift keys are only for hot cues (on mode4 ) so we can call them "delete hot cue" and the msgs while pressing shift key will be sent to channel 5
//                         ___
//6 for keys                  |
//7 for faders                |
//8 for encoders              |--->  as defined below  ||
//9 listen channel         ___|                        \/

#define MIDI_BUTTON_CHANNEL  6
#define MIDI_FADER_CHANNEL   7   //fader on the last multiplexer will send to the mode channel...
#define MIDI_ENCODER_CHANNEL 8
#define MIDI_LISTEN_CHANNEL  9 



//_____________// FADER & POTIS ////__________________________________________________________________________________________________

#define NR_OF_MULTIPLEXERS 4
const int pinmultiplexer[NR_OF_MULTIPLEXERS] = { A0,A1,A2,A3 }; //faders on the last multiplexer (on A3 here) will be sent to the mode channel
                                                                //input 4 and 5 in the last multiplexer will send its values to modechannel 1
                                                                //input 6 and 7 in the last multiplexer will send its values to modechannel 2
int faderState[NR_OF_MULTIPLEXERS][8] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0}
} ;

int s0 = 8 ;
int s1 = 9 ; 
int s2 = 10 ;
                                  
int r0 ;
int r1 ;
int r2 ;

#define FADER_CHANGE_MIN 8



//_____________// BUTTONS ////________________________________________________________________________________________________________

#define KEYMATRIX_ROWS 10
#define KEYMATRIX_COLS 4         // must stay 4 because all keys are arranged based on 4 rows arrangements 
#define KEYMATRIX_DEBOUNCE 10

//row 0 ,1 for mode 1 buttons.
//row 2 ,3 for mode 2 buttons.
//other pins are for other buttons.

byte keymatrixRowPins[KEYMATRIX_ROWS] = {23 ,25 ,27 ,29 ,31 , 33, 35, 37, 39, 41}; 
byte keymatrixColPins[KEYMATRIX_COLS] = {22 , 24 , 26 , 28};

boolean keymatrixStates[KEYMATRIX_ROWS][KEYMATRIX_COLS];
unsigned long keymatrixDebounce[KEYMATRIX_ROWS][KEYMATRIX_COLS];


byte mode1pin = A4 ;  //this pin must be an analog input. because we are using less digital pins by using an analog pin with multiple resistors and switches
byte mode2pin = A5 ;  //this pin must be an analog input. 

int shift1pin = 6;
int shift2pin = 7;


//_______________// ENCODER //_____________________________________________________________


#define ENCODER_L_PIN_A 18
#define ENCODER_L_PIN_B 19

#define ENCODER_R_PIN_A 20
#define ENCODER_R_PIN_B 21




volatile byte encoderLPos = 0;
volatile byte encoderRPos = 0;

//volatile unsigned long encoderLtstamp = 0;
//volatile unsigned long encoderRtstamp = 0;

Encoder leftEncoder(ENCODER_L_PIN_A, ENCODER_L_PIN_B);
Encoder rightEncoder(ENCODER_R_PIN_A, ENCODER_R_PIN_B);

//_________________________________________________________________________________________________________________________________________________________________________________________
//_________________________________________________________________________________________________________________________________________________________________________________________



void setup() {

  
 
  Serial.begin(31250);

  
          initKeymatrix() ;
          initmodekeys() ;
          initfaders() ;
          initshiftkeys();
}





void loop() {
     checkEncoders();
     readKeymatrix() ;
     checkFaders() ;
     checkEncoders();

}






//_________________________________________________________________________________________________________________________________________________________________________________________
//_________________________________________________________________________________________________________________________________________________________________________________________
void initKeymatrix() {
  
  // clear arrays
  for (byte r=0; r<KEYMATRIX_ROWS; r++) {
    for (byte c=0; c<KEYMATRIX_COLS; c++) {
      keymatrixStates[r][c]=0;
      keymatrixDebounce[r][c]=0;
    }
  }
  
  //configure column pin modes and states
  for (byte c=0; c<KEYMATRIX_COLS; c++) {
      pinMode(keymatrixColPins[c],OUTPUT);
      digitalWrite(keymatrixColPins[c],LOW);
  }
  
  //configure row pin modes and states
  for (byte r=0; r<KEYMATRIX_ROWS; r++) {
      pinMode(keymatrixRowPins[r],INPUT);
      digitalWrite(keymatrixRowPins[r],HIGH);  // Enable the internal 20K pullup resistors for each row pin.
  }
  
}


//__________________________________________________________________________________________________

void readKeymatrix() {

  // run through cols
  for( int c=0; c<KEYMATRIX_COLS; c++) {
    digitalWrite(keymatrixColPins[c], LOW);
    
    // check rows
    for( int r=0; r<KEYMATRIX_ROWS; r++) {
      boolean currentValue = (digitalRead(keymatrixRowPins[r])<1);
      if (currentValue != keymatrixStates[r][c]) {

        // debounce
        unsigned long currentTime = millis();
        if(currentTime-KEYMATRIX_DEBOUNCE > keymatrixDebounce[r][c]) {
          
          int keyNr = r*KEYMATRIX_COLS+c;
         
          if(r==0 || r==1 )
            {sendmode1KeyMessage(keyNr, currentValue);}
          else if(r==2 || r==3)
            {sendmode2KeyMessage(keyNr, currentValue);}
          else
            {sendKeyMessage(keyNr, currentValue);}
            
          keymatrixStates[r][c] = currentValue;
          keymatrixDebounce[r][c] = currentTime;
        }
      }
    }

    // reset col
    digitalWrite(keymatrixColPins[c], HIGH);
  }

}


//____________________________________________________________________________________________________________________


void sendKeyMessage(int keyNr, boolean value) {
  if (value) {
    MIDI.sendNoteOn(byte(keyNr), 1, MIDI_BUTTON_CHANNEL);
  } else {
    MIDI.sendNoteOff(byte(keyNr), 1, MIDI_BUTTON_CHANNEL);
  }
}

void sendmode1KeyMessage(int keyNr, boolean value) {
  if (value) {
    MIDI.sendNoteOn(byte(keyNr), 1, mode1channel());
  } else {
    MIDI.sendNoteOff(byte(keyNr), 1, mode1channel());
  }
}

void sendmode2KeyMessage(int keyNr, boolean value) {
  if (value) {
    MIDI.sendNoteOn(byte(keyNr), 1, mode2channel());
  } else {
    MIDI.sendNoteOff(byte(keyNr), 1, mode2channel());
  }
}




//_________________________________________________________________________________________________________________________________________________________________________________________
//_________________________________________________________________________________________________________________________________________________________________________________________
void initshiftkeys(){
   pinMode(shift1pin ,INPUT_PULLUP);
   pinMode(shift2pin ,INPUT_PULLUP);
   }


//_________________________________________________________________________________________________________________________________________________________________________________________
//_________________________________________________________________________________________________________________________________________________________________________________________
void initmodekeys(){
  pinMode(mode1pin,INPUT);
  pinMode(mode2pin,INPUT);
}

//_________________________________________________________________________________________________________

int mode1channel(){
  int c ;
  int val = analogRead(mode1pin);
  int m = map(val , 0 , 1023 , 0 , 4 );  //we have 5 modes from 0 to 4
  
  if(m == 4 && digitalRead(shift1pin)== LOW) //if we are in mode 4 and shift key is pressed send msgs to channel 5
  {c=5;}
  else{c = m;}
  return c ;
}

int mode2channel(){
  int c ;
  int val = analogRead(mode2pin);
  int m = map(val , 0 , 1023 , 0 , 4 );  //we have 5 modes from 0 to 4
 
  if(m == 4 && digitalRead(shift2pin)== LOW) //if we are in mode 4 and shift key is pressed send msgs to channel 5
  {c=5;}
  else{c = m;}
  return c ;
}
//_________________________________________________________________________________________________________________________________________________________________________________________
//_________________________________________________________________________________________________________________________________________________________________________________________

void checkFaders() {
  for( int j=0; j<NR_OF_MULTIPLEXERS; j++ ) // for each multiplexer
  { for (int i=0; i<8; i++) {   //do the multiplexer loop (taken from arduino.cc site)
       r0 = bitRead(i,0);       
       r1 = bitRead(i,1);       
       r2 = bitRead(i,2);       
       
       digitalWrite(s0, r0);
       digitalWrite(s1, r1);
       digitalWrite(s2, r2);

          int fdrValue = analogRead(pinmultiplexer[j]); //then read the value on the current multiplexer pin
          
          if( fdrValue != faderState[j][i] ) {                                      // and decide what channel you need to send the value to .
            if( abs(fdrValue - faderState[j][i]) > FADER_CHANGE_MIN) {
              
               if(j==(NR_OF_MULTIPLEXERS-1) && i>3 && i<=5){ //input 4 and 5 in the last multiplexer will send its values to modechannel 1
                 byte note = ((j*8)+i+20);
                 MIDI.sendControlChange(note, byte(fdrValue/8), mode1channel()); // value bis 127
                 faderState[j][i] = fdrValue;}
              
               else if(j==(NR_OF_MULTIPLEXERS-1) && i>5 && i<=7){  //input 6 and 7 in the last multiplexer will send its values to modechannel 2
                 byte note = ((j*8)+i+20);
                 MIDI.sendControlChange(note, byte(fdrValue/8), mode2channel()); // value bis 127
                 faderState[j][i] = fdrValue;}
                             
               else {byte note = ((j*8)+i);
                 MIDI.sendControlChange(note, byte(fdrValue/8), MIDI_FADER_CHANNEL); // value bis 127
                 faderState[j][i] = fdrValue;}
                      }  }  }  }  }
                    

    
//____________________________________________________________________________________

void initfaders()
{ pinMode(s0, OUTPUT);    // s0
  pinMode(s1, OUTPUT);    // s1
  pinMode(s2, OUTPUT);    // s2

}


//________________________________________________________________________________________


void checkEncoders() {
  byte newPos = leftEncoder.read();
  if (newPos != encoderLPos) {
    if (newPos > encoderLPos){
    MIDI.sendControlChange(1, byte( newPos % 128 ), MIDI_ENCODER_CHANNEL); 
    encoderLPos = newPos;}
    if (newPos < encoderLPos){
    MIDI.sendControlChange(2, byte( newPos % 128 ), MIDI_ENCODER_CHANNEL); 
    encoderLPos = newPos;}
  }
  newPos = rightEncoder.read();
  if (newPos != encoderRPos) {
    if (newPos > encoderRPos){
    MIDI.sendControlChange(3, byte(newPos % 128 ), MIDI_ENCODER_CHANNEL);
    encoderRPos = newPos;}
    if (newPos < encoderRPos){
    MIDI.sendControlChange(4, byte( newPos % 128 ), MIDI_ENCODER_CHANNEL); 
    encoderRPos = newPos;}
  }
}
//________________________________________________________________________________________

