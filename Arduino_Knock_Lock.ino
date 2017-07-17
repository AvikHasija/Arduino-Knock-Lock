//PLANNING
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

//START OF CODE LOL
#include <Servo.h>

Servo servo;
unsigned long custom_millis = 0;

//PINS
const int forceSensorPin = A0;
const int pingSensorPin = 8;
const int LEDMatrixOutputOne = 9;
const int LEDMatrixOutputTwo = 11;
const int LEDMatrixOutputThree = 13;
const int LEDMatrixInputOne = 7;
const int LEDMatrixInputTwo = 10;
const int LEDMatrixInputThree = 12;
const int LEDMatrixInputFour = 8;

const int knockDelayTime = 150; //time we delay before listening to another knock
const int knockTimeout = 10000; //after 4 seconds, timeout and start again
const int maxPatternSize = 10;
const double knockAllowedError = 0.25;

int initialPulseDuration;

//STATE VARIABLES
bool userPresent = true;
int prevKnockTime = 0;
int currentKnockTime = 0;
int lastEvent;
int currentKnock = 0;
int knockCode[maxPatternSize] = {0, 600, 600, 300, 300, 0, 0, 0, 0, 0};
int readKnock[maxPatternSize] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void setup(){
	Serial.begin(9600);
	servo.attach(9);
  
  pinMode(LEDMatrixOutputOne, OUTPUT);
  pinMode(LEDMatrixOutputTwo, OUTPUT);
  pinMode(LEDMatrixOutputThree, OUTPUT);

  pinMode(forceSensorPin, INPUT);
  pinMode(LEDMatrixInputOne, INPUT);
  pinMode(LEDMatrixInputTwo, INPUT);
  pinMode(LEDMatrixInputThree, INPUT);
  pinMode(LEDMatrixInputFour, INPUT);

  //TIMER SETUP
  TCCR2A = 0;
  TCCR2B = ( _BV(CS22)); //prescale is clk/64 --> 16MHz / 64 = 250kHz hz
  TIMSK2 |= (7 << 0); //Enabling bit 0 (TOIE2) of timer 2 (TIMSK2) enables overflow interrupts
  OCR2B = 249; //Timer 2 will overflow when it reaches 249 (250 clock cycles), and triggers the ISR. This is exactly 1ms.

	setupBaseData();
}

void loop(){
	
	while(!userPresent){
		Serial.println(measurePulse());
	}
	
	//TODO: poll until measured value ~1/3 of initial (person walked up to door)
	//only read from force sensor if NOT polling ultrasonic; someone has to be at door for pattern to work

	if(userPresent){
    for(int i = 0; i<maxPatternSize; ++i){
      readKnock[i] = 0; //Clear array
    }
    readKnockPattern();
	}
}

ISR(TIMER2_COMPB_vect){
  custom_millis++; //increment millis variable, as 1ms has passed
  TCNT2 = 0; //reset timer, so it starts again from 0 (count from 0-249 again)
}

void readKnockPattern(){
  lastEvent = custom_millis;
  Serial.print("WHAT");
  Serial.println(custom_millis-lastEvent);
    while((custom_millis-lastEvent < knockTimeout) && (currentKnock < maxPatternSize)) {
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

    Serial.println("Knock pattern inputted. Checking against password.");
    Serial.print("PATTERN CHECK: ");
    Serial.println(checkPattern());

    //TODO: Reset variables
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

	//Wait 1.5 seconds between each measurement to avoid error
	delay(1500);

	return currentPulseDuration;
}

int measureForce(){
	int currentForce;

	currentForce = analogRead(forceSensorPin);
	return currentForce;
}

void setupBaseData(){
	//Use third data measurement as initial duration to negate any errors from the first few readings
	for(int i = 0; i < 3; ++i){
		initialPulseDuration = measurePulse();
	}

	//initialForce = measureForce();
}

void unlockDoor(){
	//Use servo to unlock door
	//servo.writeMicroseconds(NUMBER);
}

void lockDoor(){
	//Use servo to lock door
	//servo.writeMicroseconds(NUMBER);	
}


//LED MATRIX METHODS
void ledSmileyFace(){
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

void ledSadFace(){
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

void ledQuestionMark(){
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
