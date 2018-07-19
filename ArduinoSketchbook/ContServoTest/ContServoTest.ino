#include <BookWorm.h>

void setup() {
  // put your setup code here, to run once:
  BookWorm.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  int i;

  for (i = 0; i < 500; i++)
  {
    BookWorm.move(i, i);
  }

  for (; i >= 0; i++)
  {
    BookWorm.move(i, i);
  }

  delay(1000);

  for (i = 0; i < 500; i++)
  {
    BookWorm.move(-i, -i);
  }

  for (; i >= 0; i++)
  {
    BookWorm.move(-i, -i);
  }

  delay(1000);
}
