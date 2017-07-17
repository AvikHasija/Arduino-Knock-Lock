/*
	POLLING: Ultrasonic 'Ping' sensor
		-in loop, trigger input pulse sent out by sensor
		-get baseline reading of duration after first few run throughs
		-keep polling for duration
		-When ~80cm from sensor (computation needed) trigger sequence (LEDs - make question mark)

	INTERRPUT: Button
		-When clicked, trigger ISR
		-output command to change knock pattern
		-store time b/w each click (knock) in array
		-find some way to exit (leave for 3 seconds? use timer to achieve this)
		-give feedback to user (print pattern with beeps?)

*/

/*
	General Program Flow:
		-Start polling ultrasonic sensor
		-Wait until object is substantially closer to sensor (>50% of distance)
			-Stop polling for distance (bool inRange = true or something)

		-Output LED grid signifying waiting for input
			-IF right, unlock with servo, smiley face output
			-ELSE (wrong), sad face output
		-(Lock door after a period of time (~8/10s)? Think about this).
		
		-If button is pressed, go into password change mode - TRIGGER ISR
		ISR:
			- turn on LED inside door to indicate pass change mode
			- fetch timer value at each click (release? - pull down/up thing)
			- subtract from previous value, store timer duration in array. Use this to match with array

*/

/*
	INPUTS:
		-Button inside door for changing pattern (Maybe two - one to trigger start, other to input. This way, can press trigger to stop input, rather than timing out).
		-Force sensor
		-PING sensor
	OUTPUTS:
		-LED grid
			Patterns: Smile, sad face, quesion mark
		-Servo motor
			-Unlock, relock
*/

//START OF CODE
#include <Servo.h>

Servo servo;
unsigned long custom_millis = 0;

//PINS
const int forceSensorPin = A0;
const int pingSensorPin = 6;
const int patternSetVerifyLED = 0;
const int interruptButton = 2; //white
const int patternButton = 3; //red

const int LEDMatrixOutputOne = 9;
const int LEDMatrixOutputTwo = 11;
const int LEDMatrixOutputThree = 13;
const int LEDMatrixInputOne = 7;
const int LEDMatrixInputTwo = 10;
const int LEDMatrixInputThree = 12;
const int LEDMatrixInputFour = 8;

const int knockDelayTime = 150; //time we delay before listening to another knock
const int knockTimeout = 8000; //after 8 seconds, timeout and start again
const int maxPatternSize = 10;
const double knockAllowedError = 0.25;

int initialPulseDistance;

bool userPresent = false;
bool setPattern = false; //True when triggered in ISR - can set pattern
bool savePattern = true; //always opposite to setPattern
int currentPulseDistance = 0;
unsigned long prevKnockTime = 0;
unsigned long currentKnockTime = 0;
int currentKnock = 0;
int knockCode[maxPatternSize] = {0, 600, 600, 300, 300, 0, 0, 0, 0, 0};
int tempKnockCode[maxPatternSize] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int readKnock[maxPatternSize] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void setup(){
	Serial.begin(9600);
	servo.attach(5);

  pinMode(forceSensorPin, INPUT);
  pinMode(interruptButton, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptButton), patternState, RISING);
  pinMode(patternButton, INPUT);
  pinMode(patternSetVerifyLED, OUTPUT);
  
  pinMode(LEDMatrixOutputOne, OUTPUT);
  pinMode(LEDMatrixOutputTwo, OUTPUT);
  pinMode(LEDMatrixOutputThree, OUTPUT);
  pinMode(LEDMatrixInputOne, INPUT);
  pinMode(LEDMatrixInputTwo, INPUT);
  pinMode(LEDMatrixInputThree, INPUT);
  pinMode(LEDMatrixInputFour, INPUT);

  //TIMER SETUP
  TCCR2A = 0;
  TCCR2B = (_BV(CS22)); //prescale is clk/64 --> 16MHz / 64 = 250kHz hz
  TIMSK2 |= (7 << 0); //Enabling bit 0 (TOIE2) of timer 2 (TIMSK2) enables overflow interrupts
  OCR2B = 249; //Timer 2 will overflow when it reaches 249 (250 clock cycles), and triggers the ISR. This is exactly 1ms.

	setupBaseDistance();
}

void loop(){
	
	while(!userPresent && !setPattern){ //poll until user is present
    currentPulseDistance = measurePulse();
    if((double)currentPulseDistance <= (double) (initialPulseDistance * 0.3)){
      userPresent = true;
    }
    Serial.print("Knock code is: ");
    for(int i=0; i<maxPatternSize; ++i){
      Serial.print(knockCode[i]);
      Serial.print(" ");
    }
    Serial.println("");
	}

	if(userPresent && !setPattern){
    Serial.println("User detected!");
    readKnockPattern(custom_millis); //read pattern in, sending current time as start time
	}

 if(setPattern){
  Serial.println("Set Pattern");
    if(digitalRead(patternButton) == HIGH){
        currentKnockTime = custom_millis;
        Serial.print("Input detected at: ");
        Serial.println(currentKnockTime);

        if(currentKnock == 0){
          tempKnockCode[currentKnock] = 0;
        } else {
          tempKnockCode[currentKnock] = (currentKnockTime - prevKnockTime);
        }

        prevKnockTime = currentKnockTime;
        currentKnock++;
        delay(knockDelayTime);
    }
 }

 if(savePattern){
    if(tempKnockCode[1] !=0){ //User has entered code
      Serial.println("Saving Pattern");
      
      //Replace existing code with newly entered code and print code to LED to give feedback to user
      for(int i = 0; i < maxPatternSize; ++i){
        knockCode[i] = tempKnockCode[i]; //replace saved value with temp value
      
        if(i != 1 && knockCode[i] !=0){
          digitalWrite(patternSetVerifyLED, HIGH);
        }

        delay(knockCode[i]);

        //ALSO: clear temp knock code array
        tempKnockCode[i] = 0;
      }
    }

    //reset all variables used to set pattern
    resetKnockVariables();
 }
 
}

ISR(TIMER2_COMPB_vect){
  custom_millis++; //increment millis variable, as 1ms has passed
  TCNT2 = 0; //reset timer, so it starts again from 0 (count from 0-249 again)
}

void patternState(){
  setPattern = !setPattern;
  savePattern = !savePattern;
}

void readKnockPattern(unsigned long startTime){ //startTime used to determine how long to wait before time out.
    Serial.print("Starting knock timer at: ");
    Serial.println(startTime);
    ledDots(); //turn on led dots while waiting for pattern input
    while((custom_millis-startTime < knockTimeout) && (currentKnock < maxPatternSize)) {
      if(measureForce() > 50){
        currentKnockTime = custom_millis;
        Serial.print("Knock detected at: ");
        Serial.println(currentKnockTime);

        if(currentKnock == 0){
          readKnock[currentKnock] = 0;
        } else {
          readKnock[currentKnock] = (currentKnockTime - prevKnockTime);
        }

        prevKnockTime = currentKnockTime;
        currentKnock++;
        delay(knockDelayTime);
      }
    }

    if(readKnock[1] != 0){ //if second value in array isnt 0, we know the user has entered at least two knocks
      Serial.println("Knock pattern inputted. Checking against password.");

      if(checkPattern()){
        Serial.println("Correct pattern! :)");
        ledSmileyFace(1000, custom_millis); //happy face for one second
        
        unlockDoor();
        delay(5000); //keep door unlocked for 5s, then lock again for security
        lockDoor();
        
      } else { //wrong pattern
        Serial.println("Incorrect pattern :(");
        ledSadFace(1000, custom_millis); //sad face for one second
      }
    } else {
      Serial.println("Timed out! No knocks detected.");
    }
   
    resetKnockVariables();
}

void resetKnockVariables(){
  Serial.println("Resetting variables");
  userPresent = false;
  currentKnockTime = 0;
  currentKnock = 0;

  for(int i = 0; i<maxPatternSize; ++i){
      readKnock[i] = 0; //Clear input array
    }
}

boolean checkPattern(){
  int enteredKnocks = 0;
  int passwordKnocks = 0;
  int enteredTimes = 0;
  int passwordTimes = 0;

  for(int i = 0; i<maxPatternSize; ++i){
    if(readKnock[i] != 0){
      enteredKnocks++;
    }
    if(knockCode[i] != 0){
      passwordKnocks++;
    }
    enteredTimes += readKnock[i];
    passwordTimes += knockCode[i];
  }

  double allowedLowerBound = (double) (passwordTimes - (passwordTimes*knockAllowedError));
  double allowedUpperBound = (double) (passwordTimes + (passwordTimes*knockAllowedError));

  if(enteredKnocks != passwordKnocks){
    return false;
  } else if ((enteredTimes >= allowedLowerBound) && (enteredTimes <= allowedUpperBound)){
    //within the set range
    return true;
  } else{
    return false;
  }
}

int measurePulse(){
	//declare function variable
	int currentPulseDuration;

	//initially, sensor needs to send output of 5 microseconds to send a pulse
	pinMode(pingSensorPin, OUTPUT);
	//low output first to ensure clean high with delay
	digitalWrite(pingSensorPin, LOW);
	delayMicroseconds(3);

	//5 us high pulse to activate ping
	digitalWrite(pingSensorPin, HIGH);
	delayMicroseconds(5);
	digitalWrite(pingSensorPin, LOW);

	//change pin to input to recieve pulse
	pinMode(pingSensorPin, INPUT);
	currentPulseDuration = pulseIn(pingSensorPin, HIGH);

  Serial.print("Polling for distance: ");
  Serial.println(currentPulseDuration);

	//Wait 1.5 seconds between each measurement to avoid error
	delay(1500);

	return currentPulseDuration;
}

int measureForce(){
	int currentForce;

	currentForce = analogRead(forceSensorPin);
	return currentForce;
}

void setupBaseDistance(){
	//Use third data measurement as initial duration to negate any errors from the first few readings
	for(int i = 0; i < 3; ++i){
		initialPulseDistance = measurePulse();
	}
    Serial.print("The intial measured distance is: ");
    Serial.print(initialPulseDistance);
    Serial.println(". We will assume someone has approached the door when the distance is ~1/3 this value");
}

void unlockDoor(){
	//Use servo to unlock door
	servo.writeMicroseconds(2300);
}

void lockDoor(){
	//Use servo to lock door
	servo.writeMicroseconds(0);	
}


//LED MATRIX METHODS
void ledSmileyFace(unsigned long duration, unsigned long startTime){
  Serial.println(startTime);
  while (custom_millis-startTime < duration){
	  digitalWrite(LEDMatrixInputTwo, HIGH );
    digitalWrite(LEDMatrixOutputOne, LOW );
    digitalWrite(LEDMatrixInputTwo, LOW );
    digitalWrite(LEDMatrixOutputOne, HIGH );
  
    digitalWrite(LEDMatrixInputThree, HIGH );
    digitalWrite(LEDMatrixOutputOne, LOW );
    digitalWrite(LEDMatrixInputThree, LOW );
    digitalWrite(LEDMatrixOutputOne, HIGH );
  
    digitalWrite(LEDMatrixInputOne, HIGH );
    digitalWrite(LEDMatrixOutputTwo, LOW );
    digitalWrite(LEDMatrixInputOne, LOW );
    digitalWrite(LEDMatrixOutputTwo, HIGH);
  
    digitalWrite(LEDMatrixInputFour, HIGH );
    digitalWrite(LEDMatrixOutputTwo, LOW );
    digitalWrite(LEDMatrixInputFour, LOW );
    digitalWrite(LEDMatrixOutputTwo, HIGH);
  
    digitalWrite(LEDMatrixInputTwo, HIGH );
    digitalWrite(LEDMatrixOutputThree, LOW );
    digitalWrite(LEDMatrixInputTwo, LOW );
    digitalWrite(LEDMatrixOutputThree, HIGH);
  
    digitalWrite(LEDMatrixInputThree, HIGH );
    digitalWrite(LEDMatrixOutputThree, LOW );
    digitalWrite(LEDMatrixInputThree, LOW );
    digitalWrite(LEDMatrixOutputThree, HIGH);
  }
}

void ledSadFace(unsigned long duration, unsigned long startTime){
  while(custom_millis-startTime < duration){
    digitalWrite(LEDMatrixInputTwo, HIGH );
    digitalWrite(LEDMatrixOutputOne, LOW );
    digitalWrite(LEDMatrixInputTwo, LOW );
    digitalWrite(LEDMatrixOutputOne, HIGH );
  
    digitalWrite(LEDMatrixInputThree, HIGH );
    digitalWrite(LEDMatrixOutputOne, LOW );
    digitalWrite(LEDMatrixInputThree, LOW );
    digitalWrite(LEDMatrixOutputOne, HIGH );
  
    digitalWrite(LEDMatrixInputOne, HIGH );
    digitalWrite(LEDMatrixOutputThree, LOW );
    digitalWrite(LEDMatrixInputOne, LOW );
    digitalWrite(LEDMatrixOutputThree, HIGH);
  
    digitalWrite(LEDMatrixInputFour, HIGH );
    digitalWrite(LEDMatrixOutputThree, LOW );
    digitalWrite(LEDMatrixInputFour, LOW );
    digitalWrite(LEDMatrixOutputThree, HIGH);
  
    digitalWrite(LEDMatrixInputTwo, HIGH );
    digitalWrite(LEDMatrixOutputTwo, LOW );
    digitalWrite(LEDMatrixInputTwo, LOW );
    digitalWrite(LEDMatrixOutputTwo, HIGH);
  
    digitalWrite(LEDMatrixInputThree, HIGH );
    digitalWrite(LEDMatrixOutputTwo, LOW );
    digitalWrite(LEDMatrixInputThree, LOW );
    digitalWrite(LEDMatrixOutputTwo, HIGH);
  }
}

void ledDots(){
	  digitalWrite(LEDMatrixInputOne, HIGH );
    digitalWrite(LEDMatrixOutputTwo, LOW );
    delay (500);
    digitalWrite(LEDMatrixInputOne, LOW );
    digitalWrite(LEDMatrixOutputTwo, HIGH );

    digitalWrite(LEDMatrixInputTwo, HIGH );
    digitalWrite(LEDMatrixOutputTwo, LOW );
    delay (500);
    digitalWrite(LEDMatrixInputTwo, LOW );
    digitalWrite(LEDMatrixOutputTwo, HIGH );

    digitalWrite(LEDMatrixInputThree, HIGH );
    digitalWrite(LEDMatrixOutputTwo, LOW );
    delay (500);
    digitalWrite(LEDMatrixInputThree, LOW );
    digitalWrite(LEDMatrixOutputTwo, HIGH );

    digitalWrite(LEDMatrixInputFour, HIGH );
    digitalWrite(LEDMatrixOutputTwo, LOW );
    delay (500);
    digitalWrite(LEDMatrixInputFour, LOW );
    digitalWrite(LEDMatrixOutputTwo, HIGH );
}

void turnLedMatrixOff() {
  digitalWrite(LEDMatrixInputOne, LOW);
  digitalWrite(LEDMatrixInputTwo, LOW);
  digitalWrite(LEDMatrixInputThree, LOW);
  digitalWrite(LEDMatrixInputFour, LOW);

  digitalWrite(LEDMatrixOutputOne, LOW);
  digitalWrite(LEDMatrixOutputTwo, LOW);
  digitalWrite(LEDMatrixOutputThree, LOW);
}


