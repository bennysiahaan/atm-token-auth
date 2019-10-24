#include <EEPROM.h>

// Seed: KBCUQTKQKNCE4TCYJFHUONRVKU
uint8_t hmacKey1[]={ 0x50, 0x45, 0x48, 0x4d,
                     0x50, 0x53, 0x44, 0x4e,
                     0x4c, 0x58, 0x49, 0x4f,
                     0x47, 0x36, 0x35, 0x55,
                     0x00, 0x00, 0x00, 0x00};
// Seed: DFFRZ4XX56WZKKABQABZZZBC56
uint8_t hmacKey2[]={ 0x19, 0x4b, 0x1c, 0xf2,
                     0xf7, 0xef, 0xad, 0x95,
                     0x28, 0x01, 0x80, 0x03,
                     0x9c, 0xe4, 0x22, 0xef,
                     0x00, 0x00, 0x00, 0x00 };
// Seed: ABCRZ4XX56WZ2247YYYWXKKKVV
uint8_t hmacKey3[]={ 0x00, 0x45, 0x1c, 0xf2,
                     0xf7, 0xef, 0xad, 0x9d,
                     0x6b, 0x9f, 0xc6, 0x31,
                     0x6b, 0xa9, 0x4a, 0xad,
                     0x00, 0x00, 0x00, 0x00 };
// Seed: GJJPQAEX53UUSTRVLKMBTYBA77
uint8_t hmacKey4[]={ 0x32, 0x52, 0xf8, 0x00,
                     0x97, 0xee, 0xe9, 0x49,
                     0x4e, 0x35, 0x5a, 0x98,
                     0x19, 0xe0, 0x20, 0xff,
                     0x00, 0x00, 0x00, 0x00 };
// Seed: CDKRZ4XX56WZ2235
uint8_t hmacKey5[]={ 0x10, 0xd5, 0x1c, 0xf2,
                     0xf7, 0xef, 0xad, 0x9d,
                     0x6b, 0x7d, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00 };

void setup() {
  for (byte i = 0; i < 20; i++)
    EEPROM.write(i, hmacKey1[i]);
}

void loop() {
}
