/*
Author : Claire Kraft
Date Modified : Sept. 30 2022
What does this program do : this program is for lab 1 and uses three different types of threads
and PWMs to to create a glowing look to the LEDS and also allow them to glow at the same time 
5 seconds. 
*/

#include "CKraft_binaryutils.h"
#include "USBSerial.h"
#include "mbed.h"
#include <utility>

#define set (uint32_t *)0x50000508
#define clear (uint32_t *)0x5000050C
#define dir (uint32_t *)0x50000514
#define ping (uint8_t)16
//#define ping (uint8_t)4
#define pinr (uint8_t)24
#define pinb (uint8_t)6

Thread thread1;
Thread thread2;
Thread thread3;
Thread thread4;
USBSerial serial;

// hold the duty cycle that will be passed from thread to thread 
typedef struct {
    float duty; /* AD result of measured voltage */
    //uint32_t counter;   /* A counter value  */
} message_t;

// create a memory pool and queue for each thread to use 
MemoryPool<message_t, 16> mpoolv;
MemoryPool<message_t, 16> mpoolc;
MemoryPool<message_t, 16> mpools;
Queue<message_t, 9> queuev;
Queue<message_t, 9> queuec;
Queue<message_t, 9> queues;


// this proudcer thread creates the queues for each of the "comsumer" threads and allocates
// them to a memory pool to be recieved by the consumer threads
void Producer(void) {

    uint32_t iv = 0;
    uint32_t jv = 1;
    uint32_t ic = 0;
    uint32_t jc = 1;
    uint32_t is = 0;
    uint32_t js = 1;
    while (true) {

        //creates queue for the vanilla thread
        if (jv == 1) {
            if (iv < 10) {
                message_t *messagev = mpoolv.try_alloc();
                messagev->duty = iv * 0.1;
                queuev.put(messagev);
                iv++;
                thread_sleep_for(10);
            } else if (iv == 10) {
                jv = 0;
                iv--;
            }
        } else if (jv == 0) {
            if (iv > 0) {
                message_t *messagev = mpoolv.try_alloc();
                messagev->duty = iv * 0.1;
                queuev.put(messagev);
                iv--;
                thread_sleep_for(10);
            } else if (iv == 0) {
                jv = 1;
                iv++;
            }
        }

        // create the queue for the chocolate thread
        if (jc == 1) {
            if (ic < 10) {
                message_t *messagec = mpoolc.try_alloc();
                messagec->duty = ic * 0.1;
                queuec.put(messagec);
                ic++;
                thread_sleep_for(50);
            } else if (ic == 10) {
                jc = 0;
                ic--;
            }
        } else if (jc == 0) {
            if (ic > 0) {
                message_t *messagec = mpoolc.try_alloc();
                messagec->duty = ic * 0.1;
                queuec.put(messagec);
                ic--;
                thread_sleep_for(50);
            } else if (ic == 0) {
                jc = 1;
                ic++;
            }
        }

        // creates the queue for the strawberry thread
        if (js == 1) {
            if (is < 10) {
                message_t *messages = mpools.try_alloc();
                messages->duty = is * 0.1;
                queues.put(messages);
                is++;
                thread_sleep_for(100);
            } else if (is == 10) {
                js = 0;
                is--;
            }
        } else if (js == 0) {
            if (is > 0) {
                message_t *messages = mpools.try_alloc();
                messages->duty = is * 0.1;
                queues.put(messages);
                is--;
                thread_sleep_for(100);
            } else if (is == 0) {
                js = 1;
                is++;
            }
        }
        //thread_sleep_for(100);
    }
}

void Vanilla(void) {
    setbit(dir, ping);
    float duty_c;
    float light;
    float dark;
    uint32_t period = 10;

    while (true) {
        // check is something has been added to vanilla queue 
        osEvent evt = queuev.get(0);

        //if it has do this loop
        if (evt.status == osEventMessage) {
            message_t *messagev = (message_t *)evt.value.p;
            duty_c = messagev->duty;
            mpoolv.free(messagev);
        }
        // set the amount of time you want the LED to be on and off
        light = int(duty_c * period);
        dark = period - light;

        // actually turn on and off the LED
        setbit(set, ping); // turning on
        thread_sleep_for(light);
        setbit(clear, ping); // turning off
        thread_sleep_for(dark);
    }
}

void Chocolate(void) {
    // initiallize the PwmOut
    PwmOut led(LED_BLUE);
    //PwmOut led(p5);

    float dutyc;
    led.period_ms(30); // set the period to 0.1 seconds

    while (true) {
        // check is something has been added to the chocolate queue
        osEvent evt = queuec.get(0);

        // if it has do this loop 
        if (evt.status == osEventMessage) {
            message_t *messagec = (message_t *)evt.value.p;
            dutyc = messagec->duty;
            mpoolc.free(messagec);
        }

        // chage the duty cycle being used 
        led.write(dutyc); // 50% duty cycle, relative to period
        //while (1);
        //thread_sleep_for(10);
    }
}

void Strawberry(void) {

    //float dutys
    // initialize the things needed for the HAL PWM
    //p30
    
    uint32_t out_pins[4] = {LED_RED, NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED};
    nrf_pwm_pins_set(NRF_PWM2, out_pins);
    nrf_pwm_configure(NRF_PWM2, NRF_PWM_CLK_2MHz, NRF_PWM_MODE_UP, 1000);

    nrf_pwm_values_common_t num[] = {0};
    nrf_pwm_sequence_t seq = {.values = num, .length = NRF_PWM_VALUES_LENGTH(num), .repeats = 50, .end_delay = 0};

    nrf_pwm_enable(NRF_PWM0);
    nrf_pwm_sequence_set(NRF_PWM2, 0, &seq);
    nrf_pwm_decoder_set(NRF_PWM2, NRF_PWM_LOAD_COMMON, NRF_PWM_STEP_AUTO);  
    nrf_pwm_task_trigger(NRF_PWM2, NRF_PWM_TASK_SEQSTART0);
 
    while (true) {
        osEvent evt = queues.get(0);

        //if new thing added to the strawberry queue, then add that to the array being used to 
        //change the duty cycle with HAL
        if (evt.status == osEventMessage) {
            message_t *messages = (message_t *)evt.value.p;
            num[0] = (messages->duty) * 1000;
            seq = {.values = num, .length = NRF_PWM_VALUES_LENGTH(num), .repeats = 50, .end_delay = 0};
            mpools.free(messages);
        }

        // create a loop to keep the LED cycling through the duties in the array 
        if (nrf_pwm_event_check(NRF_PWM2, NRF_PWM_EVENT_SEQEND0)) {
            nrf_pwm_sequence_set(NRF_PWM2, 0, &seq);
            nrf_pwm_task_trigger(NRF_PWM2, NRF_PWM_TASK_SEQSTART0);
            nrf_pwm_event_clear(NRF_PWM2, NRF_PWM_EVENT_SEQEND0);
        }
        //thread_sleep_for(100);
    }
}

// main() runs in its own thread in the OS
int main() {

    //serial.printf("Initialize\r\n");
    // initialize threads
    thread1.start(Producer);
  
    thread3.start(Chocolate);
   
    thread2.start(Vanilla);
    
    thread4.start(Strawberry);
    
    while (true) {
        thread_sleep_for(100);
    }
}