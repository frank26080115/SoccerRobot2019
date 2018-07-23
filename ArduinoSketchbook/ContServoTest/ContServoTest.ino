#include <BookWorm.h>

void setup() {
  // put your setup code here, to run once:
  BookWorm.begin();

  BookWorm.delayWhileFeeding(3000);
}

void loop() {
  // put your main code here, to run repeatedly:
  int i;

  for (i = 0; i < 500; i++)
  {
    BookWorm.move(i, i);
    #ifdef ENABLE_WEAPON
    BookWorm.spinWeapon(1000 + (i * 2));
    #endif
    BookWorm.delayWhileFeeding(2);
  }

  for (; i >= 0; i--)
  {
    BookWorm.move(i, i);
    #ifdef ENABLE_WEAPON
    BookWorm.spinWeapon(1000 + (i * 2));
    #endif
    BookWorm.delayWhileFeeding(2);
  }

  BookWorm.delayWhileFeeding(1000);

  for (i = 0; i < 500; i++)
  {
    BookWorm.move(-i, -i);
    #ifdef ENABLE_WEAPON
    BookWorm.spinWeapon(1000 + (i * 2));
    #endif
    BookWorm.delayWhileFeeding(2);
  }

  for (; i >= 0; i--)
  {
    BookWorm.move(-i, -i);
    #ifdef ENABLE_WEAPON
    BookWorm.spinWeapon(1000 + (i * 2));
    #endif
    BookWorm.delayWhileFeeding(2);
  }

  BookWorm.delayWhileFeeding(1000);
}
