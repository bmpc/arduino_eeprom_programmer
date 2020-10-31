#define CHIP_ENABLED 50
#define WRITE_ENABLED 53
#define OUTPUT_ENABLED 52

#define IO0 22
#define IO7 29

#define D0 30
#define D14 44

#define EEPROM_SIZE 32768L
#define STREAM_BUFFER_SIZE 64

#define CR '\r'
#define LF '\n'
#define BS 0x08
#define DEL 0x7F

const byte cmdBufSize = 128;

int ctxCmd = 0;

char cmdData[cmdBufSize]; // an array to store the received cmd

boolean newCmd = false;

boolean streamComplete = false;
unsigned int streamAddress = 0;

byte buffer[STREAM_BUFFER_SIZE];
int bytesRead = 0;

void setup() {
    // disable control pins on init
    digitalWrite(CHIP_ENABLED, HIGH);
    digitalWrite(WRITE_ENABLED, HIGH);
    digitalWrite(OUTPUT_ENABLED, HIGH);

    // I think it is a good practice to write before configuring the pins
    pinMode(CHIP_ENABLED, OUTPUT);
    pinMode(WRITE_ENABLED, OUTPUT);
    pinMode(OUTPUT_ENABLED, OUTPUT);

    // Initialize address pins as OUTPUT
    for (int addr = D0; addr <= D14; addr++) 
    {
        digitalWrite(addr, LOW);
        pinMode(addr, OUTPUT);
    }
  
    Serial.begin(115200);
    while (!Serial) {
      ; // wait for serial port to connect. Needed for Native USB only
    }
  
    Serial.println("Welcome to the EEPROM programmer!");
    printMenu();
    
}

void loop() {
    if (ctxCmd == 2) { // process binary file
        streamEEPROMBytes();
    } else {
        recvCmdWithEndMarker();
    }
    
    if (newCmd == true) {
        processCmd();
    }
}

void printMenu() {
    Serial.println("Options:");
    Serial.println("\t1 - Read ROM");
    Serial.println("\t2 - Write ROM");
    Serial.println("\t3 - Read byte at address");
    Serial.println("\t4 - Write byte to address");
    Serial.println("\t5 - Erase ROM - 6502 NOP");
    Serial.println("\t6 - Erase ROM");
}

void recvCmdWithEndMarker() {
    static byte ndx = 0;
    
    char rc;
   
    while (Serial.available() > 0 && newCmd == false) {
        rc = Serial.read();
        Serial.print(rc);

        if (rc == BS || rc == DEL) { // doesn't handle tabs
          cmdData[ndx] = 0;
          if (ndx > 0) {
              ndx--;
              byte back[] = {BS,32, BS};
              Serial.write(back, 3);
          }
        } else if (rc != CR && rc != LF) {
            cmdData[ndx] = rc;
            ndx++;
            if (ndx >= cmdBufSize) {
                ndx = cmdBufSize - 1;
            }
        } else {
            Serial.println();
            cmdData[ndx] = '\0'; // terminate the string
            ndx = 0;
            newCmd = true;
        }
    }
}

void streamEEPROMBytes() {
    byte tempBuffer[STREAM_BUFFER_SIZE];    

    while (Serial.available() > 0 && streamAddress < EEPROM_SIZE) {
        int _bytesRead = Serial.readBytes(tempBuffer, STREAM_BUFFER_SIZE - bytesRead);
        
        memcpy(buffer + bytesRead, tempBuffer, _bytesRead);

        bytesRead += _bytesRead;

        if (bytesRead == STREAM_BUFFER_SIZE && ((streamAddress + bytesRead) <= EEPROM_SIZE)) {
            writeEEPROMBytePage(buffer, bytesRead);

            Serial.write(0x06);

            bytesRead = 0;
        }
    }
    
    if (streamAddress >= EEPROM_SIZE - 1) {
        delay(20);
        streamAddress = 0;
        ctxCmd = 0;
        bytesRead = 0;
        resetCtrPins();

        Serial.println("Finished writing EEPROM!");
    }
}

void processCmd() {
    if (ctxCmd == 0) {
        char op = cmdData[0];
        switch(op) {
        case '1':
            readEEPROM();
            break;
        case '2':
            ctxCmd = 2;
            Serial.println("Stream bytes >");
            break;
        case '3':
            Serial.println("Enter address >");
            ctxCmd = 3;
            break;
        case '4':
            Serial.println("Enter address = byte >");
            ctxCmd = 4;
            break;
        case '5':
            eraseEEPROM(0xEA);
            break;
        case '6':
            eraseEEPROM(0xFF);
            break;
        default:
            Serial.println("---- Invalid Option!\n");
            printMenu();
            break;
        }
    } else if (ctxCmd == 3) {
        char **p;
        unsigned int address = strtoul(cmdData, p, 10);
        if (p != NULL || address >= EEPROM_SIZE) {
            Serial.println("Invalid address!");
        } else {
          readEEPROMAt(address);
        }
        ctxCmd = 0;
        resetCtrPins();
    } else if (ctxCmd == 4) {
        String addressStr = String(strtok(cmdData, "="));
        addressStr.trim();
        unsigned int address = addressStr.toInt();
        if (address >= EEPROM_SIZE) {
            Serial.println("Invalid address!");
            ctxCmd = 0;
            return;
        }
        
        String byteStr = String(strtok(NULL, "="));
        byteStr.trim();
        unsigned int _byte = byteStr.toInt();

        writeEEPROMAt(address, _byte);
        ctxCmd = 0;
        resetCtrPins();
    } else {
      Serial.println("Continue with command!");
    }

    clearCmdBuffer();
}

void clearCmdBuffer() {
    newCmd = false;
    for (int i = 0; i < cmdBufSize; i++) {
       cmdData[i] = 0;
    }
}

void readEEPROM() {
    Serial.println("EEPROM Contents:");
    Serial.println("------------------------------------------------------------------");
    setReadMode();
    for (unsigned int addr = 0; addr < EEPROM_SIZE; addr++) {
      setEEPROMAddress(addr);
      byte b = readEEPROMByte();
      if (addr == 0 || (addr % 16) == 0) {
        Serial.println();
        displayAddress(addr);
      }
      displayByte(b);
    }
    resetCtrPins();
    
    Serial.println();
    Serial.println("------------------------------------------------------------------");
}

void readEEPROMAt(int address) {
    setReadMode();
    setEEPROMAddress(address);
    byte b = readEEPROMByte();
    displayByte(address, b);
}

byte readEEPROMByte() {
    byte b = 0;
    for (int d = IO7; d >= IO0; d--) {
        b <<= 1;
        b |= digitalRead(d);
    }
    return b;
}

void writeEEPROMAt(int address, byte _byte) {
    setWriteMode();
    setEEPROMAddress(address);
    writeEEPROMByte(_byte);
    displayByte(address, _byte);
}

void writeEEPROMByte(byte _byte) {
    for (int io = IO0, b = 0; io <= IO7; io++, b++) 
    {
        digitalWrite(io, bitRead(_byte, b));
    }
    
    digitalWrite(WRITE_ENABLED, HIGH);
    digitalWrite(WRITE_ENABLED, LOW);
    delayMicroseconds(1);
    digitalWrite(WRITE_ENABLED, HIGH);
    delay(20);
}

void writeEEPROMBytePage(byte page[], int size) {
    setWriteMode();

    for (int i = 0; i < size; i++) {
        setEEPROMAddress(streamAddress++);
        for (int io = IO0, b = 0; io <= IO7; io++, b++) 
        {
            digitalWrite(io, bitRead(page[i], b));
        }
        
        digitalWrite(WRITE_ENABLED, HIGH);
        digitalWrite(WRITE_ENABLED, LOW); 
        delayMicroseconds(1);
        digitalWrite(WRITE_ENABLED, HIGH);
    }
    
    delay(20);

    Serial.print(streamAddress + 1);
    Serial.print(" of ");
    Serial.print(EEPROM_SIZE);
    Serial.print(" (");
    Serial.print(((100L*streamAddress)/(EEPROM_SIZE - 1)));
    Serial.println("%)");
}

void eraseEEPROM(byte _byte) {
    Serial.println("Erasing EEPROM ");
    setWriteMode();
    for (unsigned int addr = 0; addr < EEPROM_SIZE; addr++) {
      setEEPROMAddress(addr);
      
      for (int io = IO0, b = 0; io <= IO7; io++, b++) 
      {
          digitalWrite(io, bitRead(_byte, b));
      }

      digitalWrite(WRITE_ENABLED, HIGH);
      digitalWrite(WRITE_ENABLED, LOW); 
      delayMicroseconds(1);
      digitalWrite(WRITE_ENABLED, HIGH);
      
      if ((addr + 1) % STREAM_BUFFER_SIZE == 0) {
          Serial.print(".");
          delay(20);
      }
      if (((addr + 1) % 4096) == 0) {
          Serial.println();
      }
    }
    resetCtrPins();
    
    Serial.println();
    Serial.println("EEPROM esased.");
}


void setEEPROMAddress(int address) {
    for (int addr = D0, b = 0; addr <= D14; addr++, b++) 
    {
        digitalWrite(addr, bitRead(address, b));
    }
}

void displayByte(int address, byte b) {
    char buffer[128];
    sprintf (buffer, "EEPROM[0x%04X] = 0x%02X", address, b);
    Serial.println(buffer);
}

void displayAddress(unsigned int addr) {
    char buffer[6];
    sprintf (buffer, "[0x%04X .. 0x%04X] ", addr, addr+15);
    Serial.print(buffer);
}

void displayByte(byte b) {
    char buffer[6];
    sprintf (buffer, "%02X ", b);
    Serial.print(buffer);
}

void setReadMode() {
    digitalWrite(CHIP_ENABLED, LOW);
    digitalWrite(OUTPUT_ENABLED, LOW);
    digitalWrite(WRITE_ENABLED, HIGH);

    // set pin mode for IO pins
    for (int d = IO0; d <= IO7; d++) {
        pinMode(d, INPUT);
    }
}

void setWriteMode() {
    digitalWrite(CHIP_ENABLED, LOW);
    digitalWrite(OUTPUT_ENABLED, HIGH);

    // set pin mode for IO pins
    for (int d = IO0; d <= IO7; d++) {
        pinMode(d, OUTPUT);
    }
}

void resetCtrPins() {
    digitalWrite(CHIP_ENABLED, HIGH);
    digitalWrite(OUTPUT_ENABLED, HIGH);
    digitalWrite(WRITE_ENABLED, HIGH);
}
