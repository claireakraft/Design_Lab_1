
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

typedef struct {
    float duty; /* AD result of measured voltage */
    //uint32_t counter;   /* A counter value  */
} message_t;

MemoryPool<message_t, 16> mpoolv;
MemoryPool<message_t, 16> mpoolc;
MemoryPool<message_t, 16> mpools;
Queue<message_t, 9> queuev;
Queue<message_t, 9> queuec;
Queue<message_t, 9> queues;

void Producer(void) {

    uint32_t iv = 0;
    uint32_t jv = 1;
    uint32_t ic = 0;
    uint32_t jc = 1;
    uint32_t is = 0;
    uint32_t js = 1;
    while (true) {
        //queue for the vanilla thread
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

        // queue for the chocolate thread
        if (jc == 1) {
            if (ic < 10) {
                //serial.printf("duty is %i\r\n", ic);
                message_t *messagec = mpoolc.try_alloc();
                messagec->duty = ic * 0.1;
                queuec.put(messagec);
                ic++;
                thread_sleep_for(10);
            } else if (ic == 10) {
                jc = 0;
                ic--;
            }
        } else if (jc == 0) {
            if (ic > 0) {
                //serial.printf("duty is %i\r\n", ic);
                message_t *messagec = mpoolc.try_alloc();
                messagec->duty = ic * 0.1;
                queuec.put(messagec);
                ic--;
                thread_sleep_for(10);
            } else if (ic == 0) {
                jc = 1;
                ic++;
            }
        }

        // queue for the strawberry thread

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
        osEvent evt = queuev.get(0);

        if (evt.status == osEventMessage) {
            message_t *messagev = (message_t *)evt.value.p;
            duty_c = messagev->duty;
            //serial.printf("message gotten %d\r\n", int(duty_c * 10));
            mpoolv.free(messagev);
        }
        light = int(duty_c * period);
        dark = period - light;

        setbit(set, ping); // turning on
        thread_sleep_for(light);
        setbit(clear, ping); // turning off
        thread_sleep_for(dark);
    }
}

void Chocolate(void) {
    PwmOut led(LED_BLUE);

    float dutyc;
    led.period(0.1f); // 1 second period

    while (true) {
        osEvent evt = queuec.get(0);

        if (evt.status == osEventMessage) {
            message_t *messagec = (message_t *)evt.value.p;
            dutyc = messagec->duty;
            //serial.printf("message gotten %d\r\n", int(dutyc * 10));
            mpoolc.free(messagec);
        }

        led.write(dutyc); // 50% duty cycle, relative to period
        //while (1);
        //thread_sleep_for(10);
    }
}

void Strawberry(void) {

    //float dutys;

    uint32_t out_pins[4] = {LED_RED, NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED};
    nrf_pwm_pins_set(NRF_PWM0, out_pins);
    nrf_pwm_configure(NRF_PWM0, NRF_PWM_CLK_2MHz, NRF_PWM_MODE_UP, 1000);
    nrf_pwm_values_common_t num[] = {0};
    nrf_pwm_sequence_t seq = {.values = num, .length = NRF_PWM_VALUES_LENGTH(num), .repeats = 50, .end_delay = 0};
    nrf_pwm_enable(NRF_PWM0);
    nrf_pwm_sequence_set(NRF_PWM0, 0, &seq);
    nrf_pwm_decoder_set(NRF_PWM0, NRF_PWM_LOAD_COMMON, NRF_PWM_STEP_AUTO);
    nrf_pwm_task_trigger(NRF_PWM0, NRF_PWM_TASK_SEQSTART0);

    while (true) {
        osEvent evt = queues.get(0);

        if (evt.status == osEventMessage) {
            message_t *messages = (message_t *)evt.value.p;
            num[0] = (messages->duty) * 100;
            seq = {.values = num, .length = NRF_PWM_VALUES_LENGTH(num), .repeats = 50, .end_delay = 0};
            mpools.free(messages);
        }

        if (nrf_pwm_event_check(NRF_PWM0, NRF_PWM_EVENT_SEQEND0)) {
            nrf_pwm_sequence_set(NRF_PWM0, 0, &seq);
            nrf_pwm_task_trigger(NRF_PWM0, NRF_PWM_TASK_SEQSTART0);
            nrf_pwm_event_clear(NRF_PWM0, NRF_PWM_EVENT_SEQEND0);
        }
        //thread_sleep_for(100);
    }
}

// main() runs in its own thread in the OS
int main() {

    //serial.printf("Initialize\r\n");

    thread1.start(Producer);
    thread_sleep_for(10);
    thread3.start(Chocolate);
    thread_sleep_for(10);
    thread2.start(Vanilla);
    thread_sleep_for(10);
    thread4.start(Strawberry);
    
    
    
    
    
    
    while (true) {
        thread_sleep_for(100);
    }
}