#include "iot.h"
#include <az_core.h>
#include <az_iot.h>
#include <Arduino.h>
#include <SerialLogger.h>
#include <mqtt_client.h>

extern az_iot_hub_client client;
extern esp_mqtt_client_handle_t mqtt_client;
extern bool connected;

static char twin_req_topic[128];
static char twin_patch_topic[100];

az_span topicSpan;
az_span printSpan;
az_span dataSpan;
az_span eventDataSpan;

#define INCOMING_DATA_BUFFER_SIZE 1024
static char incoming_data[INCOMING_DATA_BUFFER_SIZE];

void printTwinResponseDetails(az_iot_hub_client_twin_response response);

int twinGetReqId = 1;
// https://docs.microsoft.com/en-us/azure/iot-hub/troubleshoot-error-codes#404104-deviceconnectionclosedremotely
bool requestTwin()
{
  if(!connected||!twinGetReqId) {
    Serial.println("not connected, skip twin get");
    return false;
  }
  //az_span telemetry = AZ_SPAN_FROM_BUFFER(telemetry_payload);

  Logger.info("Requesting twin ...xx");

  char reqIdBuf[16];
  az_span reqIdSpan = AZ_SPAN_FROM_BUFFER(reqIdBuf);
  az_span outSpan;
  az_span_i32toa(reqIdSpan, twinGetReqId++, &outSpan);

  size_t s;
  if (az_result_failed(az_iot_hub_client_twin_document_get_publish_topic(
          &client, reqIdSpan, twin_req_topic, sizeof(twin_req_topic), &s)))
  {
    Logger.error("Failed az_iot_hub_client_properties_document_get_publish_topic");
    return false;
  }
  twin_req_topic[s] = 0;
  Serial.println("twintopic=");
  Serial.println(twin_req_topic);
  Serial.println(s);

  int ret = esp_mqtt_client_publish(
          mqtt_client,
          twin_req_topic,
          NULL,
          0,
          0,
          false);
  if(ret== -1)
  {
    Logger.error("Failed publishing twin req");
    return false;
  }
  else
  {
    Logger.info("Twin req published successfully"+String(ret));
    return true;
  }
  
}

void parseTwinResp(az_span twinResp) {
    az_span batchSizeToken = AZ_SPAN_LITERAL_FROM_STR("measureBatchSize");
    az_span sleepTimeSecToken = AZ_SPAN_LITERAL_FROM_STR("sleepTimeSec");
    az_span doSleepToken = AZ_SPAN_LITERAL_FROM_STR("doSleep");
    az_span versionToken = AZ_SPAN_LITERAL_FROM_STR("version");

    char desiredVersion[20];
    desiredVersion[0] = 0;
    int measureBatchSize;
    int sleepTimeSec;
    int doSleep;

    bool propertyFound = false;
    az_json_reader jr;
    az_json_reader_init(&jr, twinResp, NULL);
    az_json_reader_next_token(&jr);
    if (jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT) {
        Logger.println("invalid twin resp");
    }
    
    az_json_reader_next_token(&jr);

    char tokenBuf[30];
    while (!propertyFound && (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT))
    {
        az_span tokenSpan = AZ_SPAN_FROM_BUFFER(tokenBuf);
        az_span reminder = az_json_token_copy_into_span(&jr.token, tokenSpan);
        az_span_copy_u8(reminder, 0);
        Logger.println(tokenBuf);
        if (az_json_token_is_text_equal(&jr.token, versionToken))
        {
            az_json_reader_next_token(&jr);
            int versionLen;
            az_json_token_get_string(&jr.token, desiredVersion, sizeof(desiredVersion), &versionLen);
            desiredVersion[versionLen] = 0;
        } else if(az_json_token_is_text_equal(&jr.token, batchSizeToken)) {
            az_json_reader_next_token(&jr);
            az_json_token_get_int32(&jr.token, &measureBatchSize);
        } else if(az_json_token_is_text_equal(&jr.token, doSleepToken)) {
            az_json_reader_next_token(&jr);
            az_json_token_get_int32(&jr.token, &doSleep);
        }  else if(az_json_token_is_text_equal(&jr.token, sleepTimeSecToken)) {
            az_json_reader_next_token(&jr);
            az_json_token_get_int32(&jr.token, &sleepTimeSec);
        }
        
        az_json_reader_next_token(&jr);

    }
    Logger.println("version:", desiredVersion);
    Logger.println("measureBatchSize:", measureBatchSize);
    Logger.println("doSleep:", doSleep);
    Logger.println("sleepTimeSec:", sleepTimeSec);
    
    Logger.println("parse twin resp end");
}

bool handleTwinResp(esp_mqtt_event_handle_t event) {
    Logger.printBuf("topic: ", event->topic, event->topic_len);

    az_iot_hub_client_twin_response response; 
    if(az_result_failed(az_iot_hub_client_twin_parse_received_topic(&client, az_span_create((uint8_t*)event->topic, event->topic_len), &response))) {
        Serial.print("Failed event, topic=");Serial.println();
        return false;
    }

    printTwinResponseDetails(response);

    if(response.response_type==AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET) {
        Logger.printBuf("data: ", event->data, event->data_len);

        parseTwinResp(az_span_create((uint8_t*)event->data, event->data_len));

        return true;
    }

    return false;
}

void printTwinResponseDetails(az_iot_hub_client_twin_response response) {
    char printBuf[200];
    az_span printSpan = AZ_SPAN_FROM_BUFFER(printBuf);
    az_span rem = az_span_copy(printSpan, AZ_SPAN_LITERAL_FROM_STR("twin: resptype="));
    az_span_u32toa(rem, (int)response.response_type, &rem);
    rem = az_span_copy(rem, AZ_SPAN_LITERAL_FROM_STR(" reqid="));
    rem = az_span_copy(rem, response.request_id);
    rem = az_span_copy(rem, AZ_SPAN_LITERAL_FROM_STR(" st="));
    az_span_u32toa(rem, (int)response.status, &rem);
    rem = az_span_copy(rem, AZ_SPAN_LITERAL_FROM_STR(" ver="));
    rem = az_span_copy(rem, response.version);
    az_span_copy_u8(rem, 0);
    Serial.println(printBuf);
}
void sendTwinProp()
{
  char twin_payload[] = "{\"doSleep\":7}";

  //"$iothub/twin/PATCH/properties/reported/?$rid=patch_temp";

  Logger.info("Sending twin patch ...");
  char reqId[4];
  reqId[0] = '9';
  String(random(0, 100)).toCharArray(reqId+1, (sizeof reqId) -1 );
  reqId[3] = 0;

  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to reflect the
  // current values of the properties.
  size_t topic_len;
  if (az_result_failed(az_iot_hub_client_twin_patch_get_publish_topic(
          &client, AZ_SPAN_FROM_BUFFER(reqId), twin_patch_topic, sizeof(twin_patch_topic), &topic_len)))
  {
    Logger.error("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return;
  }
  twin_patch_topic[topic_len] = 0;
  Serial.print("topic: ");
  Serial.println(twin_patch_topic);
  Serial.println(twin_payload);

  if (esp_mqtt_client_publish(
          mqtt_client,
          twin_req_topic,
          twin_payload,
          strlen(twin_payload)-1,
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG)
      == -1)
  {
    Logger.error("Failed publishing");
  }
  else
  {
    Logger.info("Message published successfully");
  }
}
