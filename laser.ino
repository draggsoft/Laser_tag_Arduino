// Do not remove the include below
//#include "ArduinoLaserTag.h"
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <TMRpcm.h>
#include <SPI.h>

// Definition of pinout for PCF8574T chip 8-bit I/O expander for I2C bus
#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define Rs_pin  0
#define Rw_pin  1
#define En_pin  2
#define BL_pin  3
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
//#define SD_ChipSelectPin 53  //using digital pin 4 on arduino nano 328, can use other pins

// DO NOT TOUCH
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Custom chars definition
unsigned char heartArray[8] = { 0x0, 0xa, 0x1f, 0x1f, 0xe, 0x4, 0x0 };
unsigned char ammoArray[8] = { 0x0, 0x7, 0xf, 0x1f, 0xf, 0x7, 0x0 };
unsigned char reticleArray[8] = { 0x0, 0xe, 0x15, 0x1f, 0x15, 0xe, 0x0 };
unsigned char skullArray[8] = { 0xe, 0x11, 0x1b, 0x15, 0xa, 0xe, 0x0 };
unsigned char clipArray[8] = { 0x1f, 0x11, 0x1f, 0x11, 0x1f, 0x11, 0x1f };
unsigned char *customCharsArray[8] = { heartArray, ammoArray, reticleArray, skullArray, clipArray, clipArray, clipArray, clipArray };
unsigned char heart = 0;
unsigned char ammo = 1;
unsigned char reticle = 2;
unsigned char skull = 3;
unsigned char clip = 4;
unsigned int lcdBlinkFrequency = 500;
boolean lcdBacklight = true;

// Arduino PINOUT:
const int buttonFirePin = 2;
const int buttonFireModePin = 3;
const int buttonReloadPin = 4;
const int ledMuzzleFlashPin = 5;
const int speakerPin = 9; 
const int chipSelect = 53;   // MEGA ONLY
// SD card attached to SPI bus as follows:
// CS -   pin 10
// MOSI - pin 11
// MISO - pin 12
// CLK -  pin 13

// Audio lib:
TMRpcm audio;

// Player variables:
unsigned char playerHP = 3;
unsigned char playerAmmo = 50;
unsigned char playerClips = 5;
unsigned char playerDeaths = 0;
unsigned char playerKills = 0;
// Logic variables:
unsigned long shotTime;
unsigned long blinkTime = 0;
unsigned long redrawTime;
unsigned long loopTime;
//unsigned long sumTimes = 0;
//unsigned int timesCounter = 0;
boolean shotFired = false;
boolean fullAuto = false;

void setup()
    {
    Serial.begin(9600);
    Serial.print("LCD init...                 ");
    // LCD initialization
    lcd.begin();
    lcd.backlight();
    lcd.setBacklight(HIGH);
    Serial.println("done");
    Serial.print("Writing custom chars...     ");
    // Custom chars write to memory
    for (int i = 0; i < 8; i++)
        lcd.createChar(i, customCharsArray[i]);
    // Cursor back to home
    Serial.println("done");
    lcd.home();
    lcd.setCursor(11, 0);
    Serial.print("Pinout init...               ");
    pinMode(buttonFirePin, INPUT);
    pinMode(buttonFireModePin, INPUT);
    pinMode(buttonReloadPin, INPUT);
    pinMode(ledMuzzleFlashPin, OUTPUT);
    pinMode(chipSelect, OUTPUT);
    Serial.println("done");
    Serial.print("Initializing SD card...      ");
    if (SD.begin(chipSelect)) Serial.println("done");
    else Serial.println("failed");
    Serial.print("Audio init...                ");
    audio.speakerPin = speakerPin;
    audio.setVolume(4);
    audio.quality(true);
    Serial.println("done");
    }

void toggleLCDBacklight()
    {
    if (!lcdBacklight)
        {
        lcd.setBacklight(HIGH);
        lcdBacklight = true;
        }
    else
        {
        lcd.setBacklight(LOW);
        lcdBacklight = false;
        }
    }

void lcdRedraw()
    {
    redrawTime = millis();
    lcd.setCursor(0, 0);
    for (unsigned char var = 0; var < playerHP; ++var)
        lcd.write(heart);
    lcd.setCursor(7, 0);
    lcd.write(skull);
    lcd.print(" " + String(playerDeaths));
    lcd.setCursor(12, 0);
    lcd.write(reticle);
    lcd.print(" " + String(playerKills));
    lcd.setCursor(0, 1);
//    for (int i = 0; i < playerAmmo / 2; i++)
//     {
//     lcd.write(0x02);
//     }
    if (fullAuto)
        {
        lcd.write(ammo);
        lcd.write(ammo);
        lcd.write(ammo);
        }
    else
        {
        lcd.print("-");
        lcd.write(ammo);
        //lcd.write("-");
        }
    lcd.setCursor(4, 1);
    if (playerAmmo > 9)
        {
        lcd.print(playerAmmo);
        }
    else if (playerAmmo > 0)
        {
        lcd.print(" ");
        lcd.print(playerAmmo);
        }
    else
        {
        if (millis() - blinkTime > lcdBlinkFrequency)
            {
            toggleLCDBacklight();
            blinkTime = millis();
            }
        lcd.print("NO AMMO");
        Serial.println("NO AMMO");
        }
    lcd.setCursor(12, 1);
    lcd.write(clip);
    lcd.print(" " + String(playerClips));
    }

void shoot()
    {
    if (millis() - shotTime < 66) return;                                // if weapon not yet ready - exit
    if (playerAmmo == 0)
        {
        if (millis() - shotTime > 300)
            {
            audio.play("empty2.wav");                                                // MAKE SUM NOIZ!
            shotTime = millis();
            }
        return;                                        // dont shoot if clip is empty
        }
    audio.play("shot2.wav");                                                // MAKE SUM NOIZ!
    shotFired = true;                                                    // save fact that shot has been fired
    shotTime = millis();                                                // saving time of last shot
    playerKills++;
    playerAmmo--;                                                        // decrease ammo
    digitalWrite(ledMuzzleFlashPin, HIGH);                                // turn muzzle led ON
    }

void reload()
    {
    if (playerClips > 0)                                                // if you still have ammo clips
        {
        audio.play("clip2.wav");                                                // MAKE SUM NOIZ!
        lcd.setCursor(4, 1);
        lcd.print("Reloading...");
        Serial.println("Reloading...");
        playerClips--;                                                    // decrease spare ammo
        playerAmmo = 50;                                                // reload
        delay(2000);
        audio.play("lock2.wav");                                                // MAKE SUM NOIZ!
        lcd.clear();
        }
    }

void loop()
    {
    delay(10); // debug to be removed
//    unsigned long loopStartTime = millis();
    if (digitalRead(buttonReloadPin) == HIGH) reload();
    if (digitalRead(buttonFireModePin) == HIGH)                            //
        {
        fullAuto = true;                                                // setting state for firing mode
//        Serial.println("setting fire mode to full auto.");
        }
    else
        {
//        Serial.println("setting fire mode to semi auto.");
        fullAuto = false;                                                //
        }

    if (digitalRead(buttonFirePin) == HIGH)                                // shooting:
        {
//        Serial.println("Trigger is HIGH");
        if (fullAuto)                                                    // if full auto - just shoot
            {
//            Serial.println("Firing in full auto.");
            shoot();
            }
        else if (!shotFired)
            {
            shoot();                                // if semi auto - shoot if you didnt yet shoot since trigger pulled
//            Serial.println("Firing in semi auto.");
            }
        }
    else
        {
        shotFired = false;                                                // if semi auto - set shotFire to false when releasing trigger
//            Serial.println("Trigger got LOW state");
        }

// timed polling:
    if (millis() - shotTime > 50) digitalWrite(ledMuzzleFlashPin, LOW);    // putting out muzzleFlash LED
    if (millis() - redrawTime > 100) lcdRedraw();                        // screen refresh 10 times per second

//    loopTime = millis() - loopStartTime;
//    Serial.println(loopTime);
//    sumTimes = sumTimes + (millis() - loopStartTime);
//    if (timesCounter == 9999)
//        {
//        timesCounter = 0;
//        unsigned long average = sumTimes / 10000;
//        Serial.println("main loop: " + String(average));                                            // serial debug: average loop execution time
//        sumTimes = 0;
//        }
//    else timesCounter++;
    }


