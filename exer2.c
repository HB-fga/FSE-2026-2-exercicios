// Acende o led enquanto o botão está pressionado

#include <stdio.h>
#include <wiringPi.h>

int main (void)
{
  if (wiringPiSetup () == -1)
    return 1 ;

  pinMode (4, OUTPUT) ;         // aka BCM_GPIO pin 23
  pinMode (27, INPUT) ;         // aka BCM_GPIO pin 24

  digitalWrite (4, 0) ;       // Off

  pullUpDnControl(27, PUD_UP);
  for (;;)
  {
    int value = digitalRead(27);
    if(HIGH == value) // TURN OFF WHEN IDLE
    {
      digitalWrite (4, 0) ;       // Off
    }
    else // TURN ON WHEN PRESSED
    {
      digitalWrite (4, 1) ;
    }
  }
  return 0 ;
}
