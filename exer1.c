/*
 * blink.c:
 *      blinks the first LED
 *      Gordon Henderson, projects@drogon.net
 */

#include <stdio.h>
#include <wiringPi.h>

int main (void)
{
  printf ("Raspberry Pi blink\n") ;

  if (wiringPiSetup () == -1)
    return 1 ;

  pinMode (4, OUTPUT) ;         // aka BCM_GPIO pin 23
  pinMode (5, OUTPUT) ;         // aka BCM_GPIO pin 24
  pinMode (6, OUTPUT) ;         // aka BCM_GPIO pin 25

  digitalWrite (4, 1) ;       // Off
  digitalWrite (5, 1) ;       // Off
  digitalWrite (6, 1) ;       // Off
  for (;;)
  {
    digitalWrite (4, 0) ;       // Off
    delay (500) ;               // wait
    digitalWrite (4, 1) ;       // On
    delay (500) ;

    digitalWrite (5, 0) ;
    delay (500) ;
    digitalWrite (5, 1) ;
    delay (500) ;

    digitalWrite (6, 0) ;
    delay (500) ;
    digitalWrite (6, 1) ;
    delay (500) ;
  }
  return 0 ;
}