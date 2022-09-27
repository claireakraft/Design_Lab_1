
#include "CKraft_binaryutils.h"
#include "USBSerial.h"
#include "mbed.h"
#include <utility>

#define set (uint32_t *)0x50000508
#define clear (uint32_t *)0x5000050C
#define dir (uint32_t *)0x50000514
#define ping (uint8_t)16
#define pinr (uint8_t)24
#define pinb (uint8_t)6

Thread thread1;
Thread thread2;
Thread thread3;
Thread thread4;
USBSerial serial;
//PwmOut led(LED_BLUE);

typedef struct {
    float duty; /* AD result of measured voltage */
    //uint32_t counter;   /* A counter value  */
} message_t;

MemoryPool<message_t, 16> mpool;
Queue<message_t, 9> queue;

void Producer(void) {

    uint32_t i = 0;
    uint32_t j = 1;
    while (true) {
        if (j == 1){
            serial.printf("in j = 1\r\n");
            if (i < 10) {
                serial.printf("the number is %i\r\n", i);
                message_t *message = mpool.try_alloc();
                message->duty = i * 0.1;
                queue.put(message);
                i++;
                thread_sleep_for(100);
            } 
            else if (i == 10){
                j = 0;
                serial.printf("j is %i\r\n", j);
                i--;
            }
        }
        else if(j == 0){
            serial.printf("in j = 0\r\n");
            if (i > 0) {
                serial.printf("the number is %i\r\n", i);
                message_t *message = mpool.try_alloc();
                message->duty = i * 0.1;
                queue.put(message);
                i--;
                thread_sleep_for(100);
            }
            else if(i == 0){
                j = 1;
                i++;
            }

        }
        
    }
}

void Vanilla(void) {
    float duty_c;
    float light;
    float dark;
    uint32_t period = 10;

    while (true) {
        osEvent evt = queue.get(0);

        if (evt.status == osEventMessage) {
            message_t *message = (message_t *)evt.value.p;
            duty_c = message->duty;
            serial.printf("message gotten %d\r\n", int(duty_c * 10));
            mpool.free(message);
        }
        light = int(duty_c * period);
        dark = period - light;

        setbit(set, ping); // turning on
        thread_sleep_for(light);
        setbit(clear, ping); // turning off
        thread_sleep_for(dark);
    }
}

void Chocolate(void){
    float dutyc;
    //led.period(1.0f);      // 1 second period
    while(true){
        osEvent evt = queue.get(0);

        if (evt.status == osEventMessage) {
            message_t *message = (message_t *)evt.value.p;
            dutyc = message->duty;
            mpool.free(message);
        }
        
        //led.write(dutyc);      // 50% duty cycle, relative to period
        
    }

}

void Strawberry(void){
    
    //float dutys;
 
    uint32_t out_pins[4] = {LED_RED, NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED};
    nrf_pwm_pins_set(NRF_PWM0, out_pins);
    nrf_pwm_configure(NRF_PWM0, NRF_PWM_CLK_2MHz,NRF_PWM_MODE_UP, 1000);
    nrf_pwm_values_common_t num[] = {0};
    nrf_pwm_sequence_t seq = {.values = num, .length= NRF_PWM_VALUES_LENGTH(num), .repeats= 50, .end_delay = 0 };
    nrf_pwm_enable(NRF_PWM0);
    nrf_pwm_sequence_set(NRF_PWM0, 0, &seq);
    nrf_pwm_decoder_set(NRF_PWM0, NRF_PWM_LOAD_COMMON, NRF_PWM_STEP_AUTO);
    nrf_pwm_task_trigger(NRF_PWM0, NRF_PWM_TASK_SEQSTART0);

    while(true){
        osEvent evt = queue.get(0);
        
        if (evt.status == osEventMessage) {
            message_t *message = (message_t *)evt.value.p;
            num[0] = (message->duty)*100;
            seq = {.values = num, .length= NRF_PWM_VALUES_LENGTH(num), .repeats= 50, .end_delay = 0 };
            mpool.free(message);
        }

        if(nrf_pwm_event_check(NRF_PWM0, NRF_PWM_EVENT_SEQEND0)){
            nrf_pwm_sequence_set(NRF_PWM0, 0, &seq);
            nrf_pwm_task_trigger(NRF_PWM0, NRF_PWM_TASK_SEQSTART0);
            nrf_pwm_event_clear(NRF_PWM0, NRF_PWM_EVENT_SEQEND0);
        }
        thread_sleep_for(100);
    } 

}



// main() runs in its own thread in the OS
int main() {
    //setbit(dir, ping);
    serial.printf("Initialize\r\n");
    thread1.start(Producer);
    //thread2.start(Vanilla);
    //thread3.start(Chocolate);
    thread4.start(Strawberry);

    while (true) {
        thread_sleep_for(100);
    }
}
