#include <Arduino.h>
#include "Pitches.h"
#ifdef USING_BUZZER

    #include <math.h>
    #include "driver/ledc.h"
    class Simple_Buzzer {
    private:
        int buzzer_pin;
        int channel;
        const ledc_mode_t   mode  = LEDC_LOW_SPEED_MODE;
        const ledc_timer_t  timer = LEDC_TIMER_0;
        const ledc_channel_t ch   = LEDC_CHANNEL_0;
        const ledc_timer_bit_t res = LEDC_TIMER_12_BIT; // 12-bit 
        const uint32_t duty_max = (1u << LEDC_TIMER_12_BIT) - 1;
        const uint32_t duty_50  = duty_max / 2;         // 50% 

        void noTone_buzzer()
        {
            ledcDetach(buzzer_pin);
            ledcWrite(buzzer_pin, 0); 
        }
        
        void tone_buzzer(uint16_t frequency, uint16_t duration, uint8_t volume)// should add notone_buzzer after that
        {
            if (frequency == 0) 
            {
                // 0hz stop the buzzer
                ledc_stop(mode, ch, 0);
                return;
            }
            
            // setup buzzer
            ledc_set_freq(mode, timer, frequency);
            float duty = ((float)duty_50*(float)volume/100.0f*0.3f);
            ledc_set_duty(mode, ch, (uint32_t)duty);
            ledc_update_duty(mode, ch);
            delay(duration);

            // stop buzzer
            ledc_stop(mode, ch, 0);
            
        }
    public:
        void single_beep_tone(int sound_Hz, int duration)
        {
            //tone(buzzer_pin, sound_Hz, duration);
            // to distinguish the notes, set a minimum time between them.
            // the note's duration + 30% seems to work well:
            //delay(duration);
            // stop the tone playing:
            //noTone(buzzer_pin);


            if (sound_Hz == 0) 
            {
                // 0hz stop the buzzer
                ledc_stop(mode, ch, 0);
                return;
            }

            // setup buzzer
            ledc_set_freq(mode, timer, sound_Hz);
            ledc_set_duty(mode, ch, duty_50);
            ledc_update_duty(mode, ch);
            delay(duration);

            // stop buzzer
            ledc_stop(mode, ch, 0);
        }
        void play_melody_tone(int* melody, int melody_size, float* noteDurations)
        {
            // iterate over the notes of the melody:
            for (int thisNote = 0; thisNote < melody_size; thisNote++) {
                // to calculate the note duration, take one second divided by the note type.
                //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
                float noteDuration = 800 / noteDurations[thisNote];
                //tone(buzzer_pin, melody[thisNote], noteDuration);

                // to distinguish the notes, set a minimum time between them.
                // the note's duration + 30% seems to work well:
                single_beep_tone(melody[thisNote], (int)noteDuration);
                float pauseBetweenNotes = noteDuration * 1.3f;
                delay(pauseBetweenNotes);
                // stop the tone playing:
                //noTone(buzzer_pin);
                //single_beep_tone(1, (int)20);
                //delay(pauseBetweenNotes);
            }
        }
        void initialized(int pin, int _channel)
        {
            buzzer_pin=pin;
            channel = _channel;
            //ledcSetup(channel, 6000, 8);
            //ledcAttachChannel(buzzer_pin,6000,12,channel);
            //ledcAttach(buzzer_pin, 6000, 12);


            // setup config
            ledc_timer_config_t tcfg = {};
            tcfg.speed_mode      = mode;
            tcfg.duty_resolution = res;
            tcfg.timer_num       = timer;
            tcfg.freq_hz         = 1000; 
            tcfg.clk_cfg         = LEDC_AUTO_CLK;
            ledc_timer_config(&tcfg);

            //  binding channel with GPIO
            ledc_channel_config_t ccfg = {};
            ccfg.gpio_num   = buzzer_pin;
            ccfg.speed_mode = mode;
            ccfg.channel    = ch;
            ccfg.intr_type  = LEDC_INTR_DISABLE;
            ccfg.timer_sel  = timer;
            ccfg.duty       = 0;  
            ccfg.hpoint     = 0;
            ledc_channel_config(&ccfg);
            ledc_fade_func_install(0);
        }



        void single_beep_ledc_fade(int sound_Hz, int duration, float cycle)
        {
            
            float step_quantity=80.0f;
            float duration_steps = ((float)duration/step_quantity);
            float volume=0.0f;
            float volume_max=30.0f;
            float volume_step=volume_max/step_quantity;
            for(int i=0;i<step_quantity;i++)
            {
                volume=volume_max*(float)sin(PI*cycle*(float)((float)(i+1)/step_quantity));
                constrain(volume,0.0f,volume_max);
                tone_buzzer(sound_Hz,duration_steps,(uint8_t)(volume));
            }
            delay(10);
            //noTone_buzzer();
            /*
            ledc_set_freq(mode, timer, 0);
            ledc_set_duty(mode, ch, 0);
            ledc_update_duty(mode, ch);
            */
            ledc_stop(mode, ch, 0);
        }
        void InitializedSound(int PedalID)
        {
            switch(PedalID)
            {
                case PEDAL_ID_CLUTCH:
                    delay(500);
                    single_beep_ledc_fade(NOTE_D4, 3072, 1);
                break;
                case PEDAL_ID_BRAKE:
                    single_beep_ledc_fade(NOTE_A4, 3072, 1);
                break;
                case PEDAL_ID_THROTTLE:
                    single_beep_ledc_fade(NOTE_A4, 3072, 1);
                break;
                default:
                    single_beep_tone(770, 100);
                break;

            }
        }
    };
#else
    class Simple_Buzzer
    {
        public:
            inline void initialized(int, int) {}
            inline void single_beep_tone(int, int) {}
            inline void play_melody_tone(int *, int, float *) {}
            inline void single_beep_ledc_fade(int, int, float) {}
            inline void InitializedSound(int) {}
    };
    int BuzzerPin = 1;
#endif
extern Simple_Buzzer Buzzer;