#include "Pitches.h"
#include <Arduino.h>
#include <math.h>
class Simple_Buzzer {
private:
    int buzzer_pin;
    int channel;
    void noTone_buzzer()
    {
        ledcDetachPin(buzzer_pin);
        ledcWrite(channel, 0); 
    }
    void tone_buzzer(uint16_t frequency, uint16_t duration, uint8_t volume)// should add notone_buzzer after that
    {
        ledcSetup(channel, frequency, 12);
        ledcAttachPin(buzzer_pin, channel);
        ledcWrite(channel, volume);
        if (duration) {
            delay(duration);
            //noTone_buzzer();
        } 
    }
public:
    void single_beep_tone(int sound_Hz, int duration)
    {
        tone(buzzer_pin, sound_Hz, duration);
        // to distinguish the notes, set a minimum time between them.
        // the note's duration + 30% seems to work well:
        delay(duration);
        // stop the tone playing:
        noTone(buzzer_pin);
    }
    void play_melody_tone(int* melody, int melody_size, float* noteDurations)
    {
        // iterate over the notes of the melody:
        for (int thisNote = 0; thisNote < melody_size; thisNote++) {
            // to calculate the note duration, take one second divided by the note type.
            //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
            float noteDuration = 1000.0f / noteDurations[thisNote];
            tone(buzzer_pin, melody[thisNote], noteDuration);
            // to distinguish the notes, set a minimum time between them.
            // the note's duration + 30% seems to work well:
            float pauseBetweenNotes = noteDuration * 1.3f;
            delay(pauseBetweenNotes);
            // stop the tone playing:
            noTone(buzzer_pin);
        }
    }
    void initialized(int pin, int _channel)
    {
        buzzer_pin=pin;
        channel = _channel;
        ledcSetup(channel, 6000, 8);
        ledcAttachPin(buzzer_pin, channel);
    }



    void single_beep_ledc_fade(int sound_Hz, int duration, float cycle)
    {
        int step_quantity=80;
        int duration_steps = (duration/step_quantity);
        uint32_t volume=0;
        int volume_step=255/step_quantity;
        for(int i=0;i<step_quantity;i++)
        {
            volume=(i+1)*volume_step*sin(PI*cycle*((float)(i+1)/(float)step_quantity));
            tone_buzzer(sound_Hz,duration_steps,volume);
        }
        noTone_buzzer();
        
        /*
        ledcWriteTone(channel, sound_Hz);
        int steps = (duration/100);
        uint32_t dutyCycle=0;
        int dutyCycle_step=2^8/2/100;
        
        {
            dutyCycle=dutyCycle_step*(i+1);
            ledcWrite(channel, dutyCycle);
            delay(steps);
        }
        */

    }




};
Simple_Buzzer Buzzer;