// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "SerialLogger.h"
#include <time.h>
#include <az_span.h>

#define UNIX_EPOCH_START_YEAR 1900

static void writeTime()
{
  struct tm* ptm;
  time_t now = time(NULL);

  ptm = gmtime(&now);

  Serial.print(ptm->tm_year + UNIX_EPOCH_START_YEAR);
  Serial.print("/");
  Serial.print(ptm->tm_mon + 1);
  Serial.print("/");
  Serial.print(ptm->tm_mday);
  Serial.print(" ");

  if (ptm->tm_hour < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_hour);
  Serial.print(":");

  if (ptm->tm_min < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_min);
  Serial.print(":");

  if (ptm->tm_sec < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_sec);
}

SerialLogger::SerialLogger() { Serial.begin(SERIAL_LOGGER_BAUD_RATE); }

void SerialLogger::info(String message)
{
  writeTime();
  Serial.print(" [INFO] ");
  Serial.println(message);
}

void SerialLogger::error(String message)
{
  writeTime();
  Serial.print(" [ERROR] ");
  Serial.println(message);
}

void SerialLogger::printBuf(char *header, char *data, int len) {
    printSpan(header,  az_span_create((uint8_t*)data, len));
}

void SerialLogger::printSpan(char *header, az_span span) {
    char buf[1024];
    az_span bufspan = AZ_SPAN_FROM_BUFFER(buf);
    az_span rem = az_span_copy(bufspan, az_span_create_from_str(header));
    rem = az_span_copy(rem, span);
    az_span_copy_u8(rem, 0);
    writeTime();
    Serial.print(" ");
    Serial.println(buf);
}

void SerialLogger::println(char *str) {
  writeTime();
  Serial.print(" ");
  Serial.println(str);    
}

void SerialLogger::println(char *str1, char *str2) {
  writeTime();
  Serial.print(" ");Serial.print(str1);
  Serial.print(" ");Serial.println(str2);
}

void SerialLogger::println(char *str1, int i) {
  writeTime();
  Serial.print(" ");Serial.print(str1);
  Serial.print(" ");Serial.println(i);
}


SerialLogger Logger;
