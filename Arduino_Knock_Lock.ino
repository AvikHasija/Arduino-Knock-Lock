//192 Security project 

//pins for interrupts: 2, 3

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

