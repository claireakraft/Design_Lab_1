
#include "mbed.h"
#include "CKraft_binaryutils.h"
#include "USBSerial.h"


#define set (uint32_t *)0x50000508
#define clear (uint32_t *)0x5000050C
#define dir (uint32_t *)0x50000514
#define ping (uint8_t)16
#define pinr (uint8_t)24
#define pinb (uint8_t)6

Thread thread1;
Thread thread2;
USBSerial serial;

typedef struct {
    float    duty;   /* AD result of measured voltage */
    //uint32_t counter;   /* A counter value  */
} message_t;

MemoryPool<message_t, 16> mpool;
Queue<message_t, 9> queue;

void Producer(void){

    uint32_t i = 0;
    while (true) {
        if(i < 10){
        serial.printf("the number is %i\r\n", i);    
        message_t *message = mpool.try_alloc();
        message->duty = i*0.1;
        //message->counter = i;
        queue.put(message);
        i++; 
        thread_sleep_for(1000);
        }
        else if(i==10){
            i=0;
        }

    }
}

void Vanilla(void){
    float duty_c;
    float light;
    float dark;
    uint32_t period = 10;

    while(true){
        osEvent evt = queue.get(0);
        
        if (evt.status == osEventMessage) {
            message_t *message = (message_t *)evt.value.p;
            duty_c = message->duty;
            serial.printf("message gotten %d\r\n", int(duty_c*10));
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


// main() runs in its own thread in the OS
int main(){ 
    setbit(dir, ping);
    serial.printf("Initialize\r\n");
    thread1.start(Producer);
    thread2.start(Vanilla);

    while (true) {
        serial.printf("in main\r\n");
        thread_sleep_for(1000);

    }
}

