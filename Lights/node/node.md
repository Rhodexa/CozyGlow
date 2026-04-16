A node contains a simple brain:

Two modes of data streaming:
Receive Broadcast:
	Stream:
		<key string><data byte><data byte><data byte><data byte><data byte>...
		
Receive Config Unicast: (CSV-like, preferably lowercase.) Which is better? Eg: "offset=7,keylen=6,key=Trixie" or "offset,7\nkeylen,6\nkey,Trixie\n"
	Configures:
		Offset into the stream to read
		Key String
	
	Polls Telemetry:
		Current Battery Level
		Temperature
		Current Offset
		Pinmap table (useful for future maintenance)
		
Reading the broadcast stream just means:
Read from byte 0 and compare with key. If they match, read data from offset (indexed after the key, so offset+len(key) -> offset=7,keylen=6 -> 13 ->  "Trixie84wr31s6df" -> "6")
If you need 5 bytes, you read from len(key)+offset through len(key)+offset+5
This allows keys to change without needing to reset offsets

Upon parsing, data should then be presented to a framebuffer where the rest of the software (i.e. PWM generators) can read them and update
Visual update should happen upon parsing


So, in the config of each unit's firmware, we expect to see
* pinmap
There might be different uses for pins
Ideally a single light would contain:
	Power LEDs: Red, Green, Blue, Cold White, Warm White, and only some have UV, most don't... like 90% don't
	Motors: Might use Servos or Steppers
	Temperature Sensor (most don't have that yet... again future expansion)
	Battery voltage metering: Not precise, more like to tell "fine" or "about to die".
		Lights will more often run on PSUs though. So really we care about Main rail voltage, not percentage
		
	PWM should be pretty fast, in the few kHz range. Also, good resolution is great for low level light. We will use a gamma LUT so brightness response looks more linear
	
	All of these things could be configured at compile time... IDK.
	We do have a lot of compute power though. perhaps we can keep a pin configuration set or arrays
	
	Concept example?
	pins=[] //available pins; index <-> pin map
	led_pins[6] = [0, 1, 2, 3, 4, -1] // up to 6 LED channels. Channel 5 (usually UV is not use)
	motor_type=[STEPPER, SERVO] or motor_type=[NONE, NONE] or [SERVO, NONE], etc
	motor_pin=[[5, 6, 7, 8], [9, -1, -1, -1]] // Servos only use1 pin
	
	this is quite complicated huh?
	
	Still... we are discussing two different things here:
		Comms layer: ESP-NOW protocol we use — this writes to a data frame buffer for the hardware control layer
		Hardware control layer: Controls PWM, Motors, Monitoring... etc
		
	So, technically, these could also be modules/libraries we can tape together with shared memory.

	comms layer could send a signal to the control layer to update its state
		control.update([frame data]) // data copy overhead here is negligible compared to everything else.
			No light will use more than 8 channels. 16 at best. That's still only 16 bytes. At 30Hz, even 60Hz, that's still nothing-
			
			
		
		
		
	
	
	