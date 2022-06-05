#include "telemetry.h"
#include <Arduino.h>
#include "bme_sensor.h"

extern BMESensor *bmeSensor;
static uint32_t telemetry_send_count = 0;

void getTelemetryPayload(az_span payload, az_span* out_payload)
{
  char tempBuf[20];
  String(bmeSensor->readTemp()).toCharArray(tempBuf, sizeof tempBuf);
  az_span tempSpan = az_span_create_from_str(tempBuf);

  char pBuf[20];
  String(bmeSensor->readPressure()).toCharArray(pBuf, sizeof pBuf);
  az_span pSpan = az_span_create_from_str(pBuf);

  char hBuf[20];
  String(bmeSensor->readHumidity()).toCharArray(hBuf, sizeof hBuf );
  az_span hSpan = az_span_create_from_str(hBuf);

  // --------
  az_span original_payload = payload;

  payload = az_span_copy(payload, AZ_SPAN_FROM_STR("{\"id\":"));
  (void)az_span_u32toa(payload, telemetry_send_count++, &payload);

  payload = az_span_copy(payload, AZ_SPAN_FROM_STR(",\"ts\":"));
  time_t now = time(NULL);
  (void)az_span_u64toa(payload, now, &payload);
  
  payload = az_span_copy(payload, AZ_SPAN_FROM_STR(",\"t1\":"));
  payload = az_span_copy(payload, tempSpan);
  payload = az_span_copy(payload, AZ_SPAN_FROM_STR(",\"p\":"));
  payload = az_span_copy(payload, pSpan);
  payload = az_span_copy(payload, AZ_SPAN_FROM_STR(",\"h\":"));
  payload = az_span_copy(payload, hSpan);

  payload = az_span_copy(payload, AZ_SPAN_FROM_STR("}"));
  payload = az_span_copy_u8(payload, '\0');

  *out_payload = az_span_slice(original_payload, 0, az_span_size(original_payload) - az_span_size(payload) - 1);
  
  Serial.print("payload = ");
  Serial.println((char*)az_span_ptr(*out_payload));
}