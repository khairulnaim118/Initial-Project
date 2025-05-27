// Arduino Code
int flexpin = A0;
int xPin = A1;
int yPin = A2;

void setup() {
 Serial.begin(9600);
}

void loop() {
 int flexVal = analogRead(flexpin);
 int xVal = analogRead(xPin);
 int yVal = analogRead(yPin);

 Serial.print(flexVal);
 Serial.print(",");
 Serial.print(xVal);
 Serial.print(",");
 Serial.println(yVal);

 delay(1000);
}