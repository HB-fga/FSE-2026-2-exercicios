// Acende e apaga as 3 cores do led RGB

#include <stdio.h>
#include <wiringPi.h>

int main (void) {
  if (wiringPiSetup () == -1)
    return 1;

  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  digitalWrite(4, 1);
  digitalWrite(5, 1);
  digitalWrite(6, 1);

  for (;;)
  {
    digitalWrite(4, 0);
    delay(500);
    digitalWrite(4, 1);
    delay(500);

    digitalWrite(5, 0);
    delay(500);
    digitalWrite(5, 1);
    delay(500);

    digitalWrite(6, 0);
    delay(500);
    digitalWrite(6, 1);
    delay(500);
  }

  return 0;
}