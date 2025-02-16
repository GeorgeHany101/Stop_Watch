#include <avr/io.h>
#include <avr/interrupt.h>

#define up 1
#define down 0

#define stop_timer() (TIMSK &= ~(1 << OCIE1A))

unsigned char digit[6] = {0};
unsigned char mode = up;
unsigned char currentD = 5;
volatile unsigned char interCount = 0;
unsigned char pausedFlag = 0;
unsigned char flag[7] = {0};

void start_timer(void) {
    TCNT1 = 0;
    TCCR1A = 0;
    TCCR1B = 0b00001101;
    OCR1A = 3125;
    TIMSK |= (1 << OCIE1A);
}

void count_up(void) {
    char limit[] = {9, 9, 5, 9, 5, 9};
    char i;
    for (i = 5; i >= 0; i--) {
        if (++digit[i] > limit[i]) {
            digit[i] = 0;
        }
        else {
            return;
        }
    }
}

void count_down(void) {
    char limit[] = {9, 9, 5, 9, 5, 9};
    char i;
    for (i = 5; i >= 0; i--) {
        if (digit[i] == 0) {
            digit[i] = limit[i];
        }
        else {
            digit[i]--;
            return;
        }
    }
}

void check_bottom(void) {
    if (mode == down && digit[0] == 0 && digit[1] == 0 && digit[2] == 0 && \
        digit[3] == 0 && digit[4] == 0 && digit[5] == 0) {
        PORTD |= (1 << 0);
        stop_timer();
    }
}

void modify_digit(char flagNum, char left, char right, char direction) {
    if (flag[flagNum] == 0 && pausedFlag == 1) {
        if (direction == 1) {  // Increment
        	if (left == 9 && right == 9){
        		return;
        	}
            if (digit[right] == 9) {
                if (digit[left] < 9) digit[left]++;
                digit[right] = 0;
            }
            else {
                digit[right]++;
            }
        }
        else {  // Decrement
        	if (left == 0 && right == 0){
        		return;
        	}
            if (digit[right] == 0) {
                if (digit[left] > 0) digit[left]--;
                digit[right] = 9;
            }
            else {
                digit[right]--;
            }
        }
        flag[flagNum] = 1;
    }
}

ISR(TIMER1_COMPA_vect) {
    interCount++;
}

ISR(INT0_vect) {
	char i;
    for (i = 0; i < 6; i++) {
    	digit[i] = 0;
    }
}

ISR(INT1_vect) {
    stop_timer();
    pausedFlag = 1;
}

ISR(INT2_vect) {
    start_timer();
    pausedFlag = 0;
}

void main(void) {
    SREG |= (1 << 7);
    GICR |= (1 << INT1) | (1 << INT0) | (1 << INT2);
    MCUCR |= (1 << ISC11) | (1 << ISC10) | (1 << ISC01);
    MCUCR &= ~(1<<ISC00);
    MCUCSR &= ~(1<<ISC2);

    DDRA = 0x3F;
    DDRC = 0x0F;
    DDRD |= (1 << 4) | (1 << 5);
    DDRD &= ~((1 << 2) &~ (1 << 3));
    PORTD |= (1 << 2);
    DDRB = 0;
    PORTB = 0xFF;

    PORTD |= (1 << 4);
    PORTD &= ~(1 << 5);

    pausedFlag = 0;
    start_timer();

    while (1) {
        PORTA = (1 << currentD);
        PORTC = digit[currentD];
        currentD = (currentD + 1) % 6;

        if (interCount == 5) {
            interCount = 0;
            PORTD &= ~(1 << 0);

            if (mode == up) {
                PORTD |= (1 << 4);
                PORTD &= ~(1 << 5);
                PORTD &= ~(1 << 0);
                count_up();
            }
            else {
                PORTD &= ~(1 << 4);
                PORTD |= (1 << 5);
                count_down();
            }
        }

        // Mode Toggle Button
        if (!(PINB&(1<<7))){
        		if (flag[0] == 0){
        			if (pausedFlag == 1){
        				flag[0] = 1;
        				mode ^= (1<<0);
                        PORTD ^= (1 << 4);
                        PORTD ^= (1 << 5);
        			}
        		}
        }
    	else
    		flag[0] = 0;

        // Increment/Decrement Buttons
        if (!(PINB & (1 << 1)))
        	modify_digit(1, 0, 1, up);  // Increment hours
        else
        	flag[1] = 0;

        if (!(PINB & (1 << 0)))
        	modify_digit(2, 0, 1, down);  // Decrement hours
        else
        	flag[2] = 0;

        if (!(PINB & (1 << 4)))
        	modify_digit(3, 2, 3, up);  // Increment minutes
        else
        	flag[3] = 0;

        if (!(PINB & (1 << 3)))
        	modify_digit(4, 2, 3, down);  // Decrement minutes
        else
        	flag[4] = 0;

        if (!(PINB & (1 << 6)))
        	modify_digit(5, 4, 5, up);  // Increment seconds
        else
        	flag[5] = 0;

        if (!(PINB & (1 << 5)))
        	modify_digit(6, 4, 5, down);  // Decrement seconds
        else
        	flag[6] = 0;

        check_bottom();
    }
}
