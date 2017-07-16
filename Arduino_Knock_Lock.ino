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

const int forceSensorPin = A0;
const int pingSensorPin = 8;
const int knockDelayTime = 150; //time we delay before listening to another knock
const int knockTimeout = 4000; //after 4 seconds, timeout and start again

int initialForce;
int initialPulseDuration;

//STATE VARIABLES
bool userPresent = false;
int prevTime = 0;
int currentTime = 0;
int knockCode[10] = [600, 600, 300, 300, 0, 0, 0, 0, 0, 0];
int readKnock[10] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

void setup(){
	Serial.begin(9600);
	servo.attach(9);
	setupBaseData();
}

void loop(){
	
	while(!userPresent){
		Serial.println(measurePulse());
	}
	
	//TODO: poll until measured value ~1/3 of initial (person walked up to door)
	//only read from force sensor if NOT polling ultrasonic; someone has to be at door for pattern to work

	if(userPresent){
		do{
      //MEASURE FORCE
		} while()
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
	
}

void ledSadFace(){
	
}

void ledQuestionMark(){
	
}
