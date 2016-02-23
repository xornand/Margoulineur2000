/*
 * This is a free software developed by guigur aka jean margouin
 * 
 * I would not be in charge of your use of this software
 *  (c) guigur.com 2015
 */

 /*
  * TODO:
  * Read function for the D4
  * Write function for the D4
  */

# include "margoulinade.h"

PN532_I2C pn532_i2c(Wire);
PN532 nfc(pn532_i2c);

LiquidCrystal lcd(8, 13, 12, 11, 10, 9);

Encoder myEnc(2, 3);

uint8_t readLedPin = 7; //rouge
uint8_t writeLedPin = 6; //vert

uint8_t encButton = 5;

long oldPosition  = 0;
long cursorPosition  = 0;

int cycleMenu = 0;

byte KeyB_Buffer[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

String menuStrings[][2] =
{
  {{"1. read D3"},{"read the balance of the dormitory 3"}},
  {{"2. write D3"},{"write new balance of the dormitory 3"}},
  {{"3. read D4"},{"read the balance of the dormitory 4"}},
  {{"4. write D4"},{"write new balance of the dormitory 4"}},
  {{"5. About"},{"by guigur & oborotev"}},
};

void setup(void)
{
  pinMode(readLedPin, OUTPUT);
  pinMode(writeLedPin, OUTPUT);
  lcd.begin(16, 2);  

  pinMode(encButton, INPUT_PULLUP);
  lcd.print("Margoulineur2000");
  
  Serial.begin(115200);
  Serial.println("Looking for PN532...");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  
  if (! versiondata)
  {
    Serial.print("Didn't find PN53x board");
    lcd.setCursor(0,1);
    lcd.print("NFC MODULE ERROR");
    while (1);
  }
  
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  nfc.SAMConfig(); // configure board to read RFID tags
  
  lcd.setCursor(0,1);
  lcd.print("NFC OK");
  delay(1000);
  lcd.clear();
  }

void encoderMenu()
{
  int newPosition = myEnc.read();
  if ((newPosition != oldPosition) )
  {
    oldPosition = newPosition;
    if(oldPosition < 0)
    {
       myEnc.write(255);
    }
    if(oldPosition > 255)
    {
       myEnc.write(0);
    }
    Serial.println(newPosition / ENCODERSTEPS);
    cycleMenu = (abs(newPosition) / ENCODERSTEPS) / MENUELEMENTS;
    lcd.clear();
  }
}

void loop(void)
{
 byte relativePosition = (abs(oldPosition) / ENCODERSTEPS) - (cycleMenu * MENUELEMENTS);
  lcd.setCursor(0,0);
  lcd.print(menuStrings[relativePosition][0]);
  lcd.setCursor(0,1);
  lcd.print(menuStrings[relativePosition][1]);
  encoderMenu();
  if (!digitalRead(encButton))
   {
    Serial.println("BOUTON !");
    switch (relativePosition)
    {
      case 0:
        lcd.clear();
        nfc_read_write(3, false);
        break;
      case 1:
        lcd.clear();
        nfc_read_write(3, true);
        break;
      case 2:
        lcd.clear();
        nfc_read_write(4, false);
        break;
      case 3:
        lcd.clear();
        nfc_read_write(4, true);
        break;
    }
   }
}

int encoderWrite()
{
  int newPosition;
  myEnc.write(0);
  Serial.print("jeej");
  delay(500);
  oldPosition = -1;
  while(digitalRead(encButton))
  {
    newPosition = myEnc.read();
    if ((newPosition != oldPosition) )
    {
      if (newPosition < VALMIN * ENCODERSTEPS)
        {
          myEnc.write(VALMIN * ENCODERSTEPS);
          newPosition = 0;
        }
        else if (newPosition > VALMAX * ENCODERSTEPS)
        {
          myEnc.write(VALMAX * ENCODERSTEPS);
          newPosition = 40; 
        }
        else
         {
          lcd.clear();
          lcd.print("New balance :");
          lcd.setCursor(0,1);
          lcd.print(newPosition / ENCODERSTEPS);
          
          oldPosition = newPosition;
          Serial.println(newPosition / ENCODERSTEPS);  
         } 
    }
  }
  myEnc.write(0);
  oldPosition = -1;
  return (newPosition / ENCODERSTEPS);
}

void                nfc_read_write(byte dormitory, bool mode)
{
  t_nfc_handler     nfc_handler;

  if (mode) // We are in write mode
  {
    if (dormitory == 4)
    {
      lcd.print("Balance will be");
      lcd.setCursor(0,1);
      lcd.print("changed to 40...");
      lcd.setCursor(0,0);
      delay(1500);
    }
    else
    {
      nfc_handler.newBalance = encoderWrite() * 100;
      nfc_handler.balance[0] = nfc_handler.newBalance / 256;
      nfc_handler.balance[1] = nfc_handler.newBalance - (nfc_handler.balance[0] * 256);
    }
  }

  mode? digitalWrite(writeLedPin, HIGH) : digitalWrite(readLedPin, HIGH);
  lcd.clear();
  mode ? lcd.print("WRITING ...") : lcd.print("READING ...");
  lcd.setCursor(0,1);
  lcd.print("Scan your card");
  
  nfc_handler.success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfc_handler.uid, &nfc_handler.uidLength);

  if (nfc_handler.success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    lcd.clear();
    lcd.print("I Found a card !");
    Serial.print("  UID Length: ");
    Serial.print(nfc_handler.uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("  UID Value: ");
    for (uint8_t i = 0; i < nfc_handler.uidLength; i++)
    {
      Serial.print(nfc_handler.uid[i], HEX);
      Serial.print(' ');
    }
    Serial.println("");

    if (nfc_handler.uidLength == 4)
    {
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
      nfc_handler.KeyA_D4[0] = nfc_handler.uid[0];
      nfc_handler.KeyA_D4[1] = nfc_handler.uid[1];
      nfc_handler.KeyA_D4[2] = nfc_handler.uid[2];
      nfc_handler.KeyA_D4[3] = nfc_handler.uid[3];
      nfc_handler.KeyA_D4[4] = nfc_handler.uid[0] ^ nfc_handler.uid[1] ^ nfc_handler.uid[2] ^ nfc_handler.uid[3];
      nfc_handler.KeyA_D4[5] = nfc_handler.uid[0] + nfc_handler.uid[1] + nfc_handler.uid[2] + nfc_handler.uid[3];

      // Now we try to go through all 16 sectors (each having 4 blocks)
      // authenticating each sector, and then dumping the blocks
      for (nfc_handler.currentblock = 0; nfc_handler.currentblock < 64; nfc_handler.currentblock++)
      {
        // Check if this is a new block so that we can reauthenticate
        if (nfc.mifareclassic_IsFirstBlock(nfc_handler.currentblock))
            nfc_handler.authenticated = false;

        // If the sector hasn't been authenticated, do so first
        if (!nfc_handler.authenticated)
        {
          // Starting of a new sector ... try to to authenticate
          Serial.print("------------------------Sector ");Serial.print(nfc_handler.currentblock/4, DEC);Serial.println("-------------------------");
            Serial.print("keys used:");
           
          if (nfc_handler.currentblock >= 5 * 4 && nfc_handler.currentblock <= 6 * 4)
          {
            //Key display debug
            dormitory == 3 ?
                displayKeyDebug(nfc_handler.KeyA_D3_part2) :
                displayKeyDebug(nfc_handler.KeyA_D4);
            nfc_handler.success = dormitory == 3 ?
              nfc.mifareclassic_AuthenticateBlock (nfc_handler.uid,
                nfc_handler.uidLength,
                nfc_handler.currentblock,
                nfc_handler.keyNumber,
                nfc_handler.KeyA_D3_part2) :
              nfc.mifareclassic_AuthenticateBlock (nfc_handler.uid,
                nfc_handler.uidLength,
                nfc_handler.currentblock,
                nfc_handler.keyNumber,
                nfc_handler.KeyA_D4);
              
          }
          else
          {
            //Key display debug
            dormitory == 3 ?
                displayKeyDebug(nfc_handler.KeyA_D3_part1) :
                displayKeyDebug(nfc_handler.KeyA_D4);
            nfc_handler.success = dormitory == 3 ?
              nfc.mifareclassic_AuthenticateBlock (nfc_handler.uid,
                nfc_handler.uidLength,
                nfc_handler.currentblock,
                nfc_handler.keyNumber,
                nfc_handler.KeyA_D3_part1) :
              nfc.mifareclassic_AuthenticateBlock (nfc_handler.uid,
                nfc_handler.uidLength,
                nfc_handler.currentblock,
                nfc_handler.keyNumber,
                nfc_handler.KeyA_D4);
          }
          Serial.println("");
          if (nfc_handler.success)
            nfc_handler.authenticated = true;
          else
          {
            Serial.println("Authentication error");
            lcd.clear();
            lcd.print("key error");
          }
        }
        // If we're still not authenticated just skip the block
        if (!nfc_handler.authenticated)
        {
          Serial.print("Block ");Serial.print(nfc_handler.currentblock, DEC);Serial.println(" unable to authenticate");
        }
        else
        {
          // Authenticated ... we should be able to read the block now
          // Dump the data into the 'data' array
          nfc_handler.success = nfc.mifareclassic_ReadDataBlock(nfc_handler.currentblock, nfc_handler.data);
          if (nfc_handler.success)
          {
            // Read successful
            Serial.print("Block ");Serial.print(nfc_handler.currentblock, DEC);
            Serial.print(" ");
            // Dump the raw data
            nfc.PrintHexChar(nfc_handler.data, 16);
            if (!mode)
            {
                nfc_handler.offset = dormitory == 4 ? 0 : 7;
                if (nfc_handler.currentblock == 24)
                    nfc_handler.currentBalance = dormitory == 4 ?
                     (nfc_handler.data[nfc_handler.offset + 1] * 256) + nfc_handler.data[nfc_handler.offset] :
                     (nfc_handler.data[nfc_handler.offset - 1] * 256) + nfc_handler.data[nfc_handler.offset];
            }
            else
            {
                if (nfc_handler.currentblock >= 24 && nfc_handler.currentblock <= 26 && dormitory == 3)
                {
                    nfc_handler.offset = nfc_handler.currentblock == 25 ? 6 : 7;
                    nfc.mifareclassic_ReadDataBlock(nfc_handler.currentblock, nfc_handler.data);
                    nfc_handler.data[nfc_handler.offset - 1] = nfc_handler.balance[0];
                    nfc_handler.data[nfc_handler.offset] = nfc_handler.balance[1];
                    nfc_handler.success = nfc.mifareclassic_WriteDataBlock(nfc_handler.currentblock, nfc_handler.data);
                    if (nfc_handler.success)
                    {
                        Serial.print("Block ");Serial.print(nfc_handler.currentblock, DEC);
                        Serial.print(" ");
                        nfc.PrintHexChar(nfc_handler.data, 16);
                    }
                }
                else if (nfc_handler.currentblock == 24 || nfc_handler.currentblock == 26 && dormitory == 4)
                {
                  nfc.mifareclassic_ReadDataBlock(nfc_handler.currentblock, nfc_handler.data);
                  nfc_handler.data[0] = (uint8_t)0xA0;
                  nfc_handler.data[1] = (uint8_t)0x0F;
                  nfc_handler.data[7] = (uint8_t)0x00;
                  nfc_handler.data[10] = (uint8_t)0xCA;
                  nfc_handler.data[11] = (uint8_t)0x25;
                  nfc_handler.data[14] = (uint8_t)0x41;
                  nfc_handler.data[15] = (uint8_t)0x9F;
                  nfc_handler.success = nfc.mifareclassic_WriteDataBlock(nfc_handler.currentblock, nfc_handler.data);
                  if (nfc_handler.success)
                    {
                        Serial.print("Block ");Serial.print(nfc_handler.currentblock, DEC);
                        Serial.print(" ");
                        nfc.PrintHexChar(nfc_handler.data, 16);
                    }
                }
            }
          }
          else
          {
            // Oops ... something happened
            Serial.print("Block ");Serial.print(nfc_handler.currentblock, DEC);
            Serial.println(" unable to read this block");
            
          }
        }
      }
    }
    else
      Serial.println("Ooops ... this doesn't seem to be a Mifare Classic card!");
  }
  lcd.clear();
  lcd.print("Balance :");
  lcd.setCursor(0,1);
  if (!mode)
  {
    lcd.print(nfc_handler.currentBalance / 100);
    Serial.println(nfc_handler.currentBalance, DEC);
  }
  else
  {
    
    dormitory == 3 ? lcd.print(nfc_handler.newBalance / 100) : lcd.print("40");
    dormitory == 3 ? Serial.println(nfc_handler.newBalance, DEC) : Serial.println(40, DEC);
  }
  Serial.println("\n\nDONE ! waiting 4 the button");
  digitalWrite(readLedPin, LOW);
  wait4button();
  lcd.clear();
  Serial.print("end read = ");Serial.println(oldPosition);
  
  Serial.flush();
}

void            displayKeyDebug(byte *key)
{
    uint8_t     i = -1;

    while (++i < 6)
        Serial.print(key[i], HEX);
}

void wait4button()
{
  while(digitalRead(encButton))
  {
  }
  delay(500);
}
