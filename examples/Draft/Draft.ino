#include "mbedtls/md5.h"
#include <Arduino.h>
#include <MD5Builder.h>

void setup() {
  Serial.begin(115200);
  delay(2000);

  const char* data = "Hello World";

  {
    uint8_t md5[16];
    mbedtls_md5_context _ctx;
    mbedtls_md5_init(&_ctx);
    mbedtls_md5_starts(&_ctx);
    mbedtls_md5_update(&_ctx, (const unsigned char*)data, strlen(data));
    mbedtls_md5_finish(&_ctx, md5);
    char output[33];
    for (int i = 0; i < 16; i++) {
      sprintf_P(output + (i * 2), PSTR("%02x"), md5[i]);
    }
    Serial.println(String(output));
  }

  {
    MD5Builder md5;
    md5.begin();
    md5.add(data, strlen(data);
    md5.calculate();
    char output[33];
    md5.getChars(output);
    Serial.println(String(output));
  }
}

void loop() {
}
