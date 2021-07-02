#ifndef MOTOR_H_
#define MOTOR_H_

// *********************** //
// Assume F_CPU = 8 Mhz //

//*************************** //
// Motor is fixed as follows:
// Utilizes Timer 0
// Non-Inverting, Fast PWM
// Cannot go below 20% or above duty %80 cycle
// Frequency = 31.250 kHz
// **NO** prescaling utilized (due to 31 kHz)


// values for Compare register
#define DUTY_CYCLE_20 0x32
#define DUTY_CYCLE_80 0xCB 

void Motor_init() {
	// Initializes the waveform generation with assumptions lined out above
	// Does NOT start the motor
	
	DDRB |= (1 << 4) | (1 << 5); // open up OC0 and PORT5 (enable signal)
	PORTB &= ~(1<<5); // motor enable = 0 at first
	
	TCCR0 = 0;
	TCCR0 |= (1<<WGM00) | (1<<WGM01) | (1<<COM01) | (1<<CS02) | (1<<CS00); // set non-inverting fast-pwm (do not start timer)
	OCR0 = DUTY_CYCLE_20; // init to 20% duty cycle
}

void Motor_start() {
	PORTB |= (1<<5);
}

void Motor_stop() {
	PORTB &= ~(1<<5);
}

void Motor_setDutyCycle(uint8_t duty_cycle_byte) {
	// since we will be dealing with analog input,
	// duty_cycle_byte == 0 means 0V or Duty_Cycle = 0%
	// duty_cycle_byte == 0xFF means V_ref or Duty_Cycle = 100
	// Or in other words, our analog range is (0,V_ref)
	
	// we want our input to be clamped (no extremes), 
	// so once we pass 0.8*V_ref
	// it will get clamped (or in the same way for below 0.2*V_ref)
	
	
	if (duty_cycle_byte < DUTY_CYCLE_20) {
		OCR0 = DUTY_CYCLE_20;
	} else if (duty_cycle_byte > DUTY_CYCLE_80) {
		OCR0 = DUTY_CYCLE_80;
	} else {
		OCR0 = duty_cycle_byte;
	}
}

#endif