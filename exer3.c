// Aumenta ou diminui a intensidade do led ao girar o encoder

#include <stdio.h>
#include <wiringPi.h>
#include <softPwm.h>

// #define RED_PIN X
// #define GREEN_PIN X
#define BLUE_PIN 4

#define CLK_PIN 29
#define DT_PIN  28

volatile int counter = 50;
volatile int last_clk_state;

void rotation(void) 
{
  int clk_state = digitalRead(CLK_PIN);
  int dt_state = digitalRead(DT_PIN);

  if (clk_state != last_clk_state) 
  {
    if (clk_state != dt_state)
    {
      if(counter >=1)
      {
        counter-=1;
        printf("horario: %d \n", counter);
      }
    }
    else
    {
      if(counter <= 99)
      {
        counter+=1;
        printf("anti-horario: %d \n", counter);
      }
    }
  }
  softPwmWrite(BLUE_PIN, counter);
  last_clk_state = clk_state;
}

int main (void)
{
  printf ("Raspberry Pi blink\n") ;

  if (wiringPiSetup () == -1)
    return 1 ;


  softPwmCreate(BLUE_PIN, 0, 100);

  // pinMode(BLUE_PIN, PWM_OUTPUT);  //  BLUE  aka BCM_GPIO pin 23

  // pinMode (4, INPUT);  //
  // pinMode (4, INPUT);  //

  // pwmSetMode(PWM_MODE_MS);   // Mark-Sp ace mode
  // pwmSetRange(1024);         // Resolution
  // pwmSetClock(400);          // Frequency
  
  pinMode(DT_PIN, INPUT);  // DT   aka BCM_GPIO pin 20
  pinMode(CLK_PIN, INPUT); // CLK  aka BCM_GPIO pin 21

  // digitalWrite (4, 0) ;       // Off
  // digitalWrite (5, 1) ;       // Off
  // digitalWrite (6, 1) ;       // Off

  pullUpDnControl(DT_PIN, PUD_UP);
  pullUpDnControl(CLK_PIN, PUD_UP);

  last_clk_state = digitalRead(CLK_PIN);

  if (wiringPiISR(CLK_PIN, INT_EDGE_BOTH, &rotation) < 0)
  {
    // fprintf("deu ruim\n");
    return 1;
  }

  while(1)
  {
    // pwmWrite(BLUE_PIN, counter);
    delay(1000);
  }

  return 0 ;
}
