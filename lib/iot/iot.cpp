
// Azure IoT SDK for C includes
#include "iot.h"
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>
#include <Arduino.h>
#include <SerialLogger.h>
#include "AzIoTSasToken.h"
#include <mqtt_client.h>


bool handleTwinResp(esp_mqtt_event_handle_t event) ;
bool handleC2d(esp_mqtt_event_handle_t event);

#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))

extern char* iothubHost;
extern char* iotDeviceId;
extern char* iotDeviceKey;

extern char* mqtt_broker_uri;

// without this api version, the twin api will not work
#define API_VER "/?api-version=2021-04-12"

static const int mqtt_port = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT;

// When developing for your own Arduino-based platform,
// please follow the format '(ard;<platform>)'. 
#define AZURE_SDK_CLIENT_USER_AGENT "c/" AZ_SDK_VERSION_STRING "(ard;esp32)"

// Memory allocated for the sample's variables and structures.
esp_mqtt_client_handle_t mqtt_client;
az_iot_hub_client client;

static char mqtt_client_id[128];
static char mqtt_username[128];
static char mqtt_password[200];
static uint8_t sas_signature_buffer[256];
static char telemetry_topic[128];
static uint8_t telemetry_payload[100];

bool connected = false;

// Auxiliary functions
#ifndef IOT_CONFIG_USE_X509_CERT
static AzIoTSasToken sasToken(
    &client,
    az_span_create_from_str(iotDeviceKey),
    AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
    AZ_SPAN_FROM_BUFFER(mqtt_password));
#endif // IOT_CONFIG_USE_X509_CERT

int cldMsgSubsId;
int twingetSubsId;
int twinPropSubsId;

int cldMsgSubOk = false;
int twingetSubsOk = false;
int twinPropSubsOk = false;
esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  
  char buf[1500];
  az_span sp = AZ_SPAN_FROM_BUFFER(buf);
  az_span remainder;
  switch (event->event_id)
  {
    int i, r;
     
    case MQTT_EVENT_ERROR:
      Logger.info("MQTT event MQTT_EVENT_ERROR");
      /*
      sp = az_span_copy(sp, AZ_SPAN_FROM_STR("data: "));
      sp = az_span_copy(sp, az_span_create((uint8_t*)event->data, event->data_len));

      sp = az_span_copy(sp, AZ_SPAN_FROM_STR(" topic: "));
      sp = az_span_copy(sp, az_span_create((uint8_t*)event->topic, event->topic_len));

      sp = az_span_copy(sp, AZ_SPAN_FROM_STR(" lasterr: "));
      az_span_i32toa(sp, event->error_handle->esp_tls_last_esp_err, &sp);

      sp = az_span_copy(sp, AZ_SPAN_FROM_STR(" esp_tls_stack_err: "));
      az_span_i32toa(sp, event->error_handle->esp_tls_stack_err, &sp);

      sp = az_span_copy(sp, AZ_SPAN_FROM_STR(" esp_tls_cert_verify_flags: "));
      az_span_i32toa(sp, event->error_handle->esp_tls_cert_verify_flags, &sp);

      sp = az_span_copy(sp, AZ_SPAN_FROM_STR(" error_type: "));
      az_span_i32toa(sp, event->error_handle->error_type, &sp);
      
      sp = az_span_copy(sp, AZ_SPAN_FROM_STR(" connect_return_code: "));
      az_span_i32toa(sp, event->error_handle->connect_return_code, &sp);

      sp = az_span_copy(sp, AZ_SPAN_FROM_STR(" esp_transport_sock_errno: "));
      az_span_i32toa(sp, event->error_handle->esp_transport_sock_errno, &sp);
*/
      az_span_copy_u8(sp, 0);
      Logger.info(buf);
      break;
    case MQTT_EVENT_CONNECTED:
      Logger.info("MQTT event MQTT_EVENT_CONNECTED");
      

      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
      if (r == -1)
      {
        Logger.error("Could not subscribe for cloud-to-device messages.");
      }
      else
      {
        Logger.info("Subscribed for cloud-to-device messages; message id:"  + String(r));
        cldMsgSubsId = r;
      }

      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1);
      if (r == -1)
      {
        Logger.error("Could not subscribe for twin resp messages.");
      }
      else
      {
        Logger.info("Subscribed for twin resp messages; message id:"  + String(r));
        twingetSubsId = r;
      }

      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_PROPERTIES_WRITABLE_UPDATES_SUBSCRIBE_TOPIC, 1);
      if (r == -1)
      {
        Logger.error("Could not subscribe for twin prop messages.");
      }
      else
      {
        Logger.info("Subscribed for twin prop messages; message id:"  + String(r));
        twinPropSubsId = r;
      }

      connected = true;
      break;
    case MQTT_EVENT_DISCONNECTED:
      Logger.info("MQTT event MQTT_EVENT_DISCONNECTED");
      connected = false;
      break;
    case MQTT_EVENT_SUBSCRIBED:
      Logger.info("MQTT event MQTT_EVENT_SUBSCRIBED msgid=" + String(event->msg_id));
      if(event->msg_id==cldMsgSubsId) {
        cldMsgSubOk = true;
      } else if(event->msg_id==twingetSubsId) {
        twingetSubsOk = true;
      } else if(event->msg_id==twinPropSubsId) {
        twinPropSubsOk = true;
      }
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      Logger.info("MQTT event MQTT_EVENT_UNSUBSCRIBED");
      break;
    case MQTT_EVENT_PUBLISHED:
      Logger.info("MQTT event MQTT_EVENT_PUBLISHED");
      break;
    case MQTT_EVENT_DATA:
      Logger.info("MQTT event MQTT_EVENT_DATA");
      Logger.printBuf("topic: ", event->topic, event->topic_len);
      
      if(handleTwinResp(event)) {    
        Logger.info("twin handled");
      } else if(handleC2d(event) ) {
        Logger.info("c2d handled");
      } else {
        Logger.info("msg not handled");
      }

      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      Logger.info("MQTT event MQTT_EVENT_BEFORE_CONNECT");
      break;
    default:
      Logger.error("MQTT event UNKNOWN");
      break;
  }

  return ESP_OK;
}

void initializeIoTHubClient()
{
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

  if (az_result_failed(az_iot_hub_client_init(
          &client,
          az_span_create((uint8_t*)iothubHost, strlen(iothubHost)),
          az_span_create((uint8_t*)iotDeviceId, strlen(iotDeviceId)),
          &options)))
  {
    Logger.error("Failed initializing Azure IoT Hub client");
    return;
  }

  size_t client_id_length;
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
  {
    Logger.error("Failed getting client id");
    return;
  }

  az_span username_span = AZ_SPAN_FROM_BUFFER(mqtt_username);
  az_span remainder = az_span_copy(username_span, az_span_create_from_str(iothubHost));
  remainder = az_span_copy_u8(remainder, '/');
  remainder = az_span_copy(remainder, az_span_create_from_str(iotDeviceId));
  remainder = az_span_copy(remainder, AZ_SPAN_LITERAL_FROM_STR(API_VER));
  remainder = az_span_copy_u8(remainder, 0);

  // if (az_result_failed(az_iot_hub_client_get_user_name(
  //         &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
  // {
  //   Logger.error("Failed to get MQTT clientId, return code");
  //   return;
  // }

  Logger.info("Client ID: " + String(mqtt_client_id));
  Logger.info("Username: " + String(mqtt_username));
  Logger.info("broker uri: " + String(mqtt_broker_uri));
}

int initializeMqttClient()
{
  #ifndef IOT_CONFIG_USE_X509_CERT
  if (sasToken.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0)
  {
    Logger.error("Failed generating SAS token");
    return 1;
  }
  #endif

  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));
  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_port;
  mqtt_config.client_id = mqtt_client_id;
  mqtt_config.username = mqtt_username;
  //mqtt_config.protocol_ver = MQTT_PROTOCOL_V_3_1_1;

  #ifdef IOT_CONFIG_USE_X509_CERT
    Logger.info("MQTT client using X509 Certificate authentication");
    mqtt_config.client_cert_pem = IOT_CONFIG_DEVICE_CERT;
    mqtt_config.client_key_pem = IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY;
  #else // Using SAS key
    mqtt_config.password = (const char*)az_span_ptr(sasToken.Get());
  #endif

  mqtt_config.keepalive = 30;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = mqtt_event_handler;
  mqtt_config.user_context = NULL;
  mqtt_config.cert_pem = (const char*)ca_pem;
  //mqtt_config.cert_len = caw_pem_len;
  mqtt_client = esp_mqtt_client_init(&mqtt_config);

  if (mqtt_client == NULL)
  {
    Logger.error("Failed creating mqtt client");
    return 1;
  }

  esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

  if (start_result != ESP_OK)
  {
    Logger.error("Could not start mqtt client; error code:" + start_result);
    return 1;
  }
  else
  {
    Logger.info("MQTT client started");
    return 0;
  }
}

extern void getTelemetryPayload(az_span payload, az_span* out_payload);

int sendTelemetry()
{
  Logger.info("Sending telemetry ...");

  if(!connected||!cldMsgSubOk) {
    Serial.println("not connected, skip telemetry");
    return 0;
  }
  az_span telemetry = AZ_SPAN_FROM_BUFFER(telemetry_payload);

  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to reflect the
  // current values of the properties.
  size_t topic_len;
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), &topic_len)))
  {
    Logger.error("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return 0;
  }
  telemetry_topic[topic_len] = 0;
  Serial.print("topic: ");
  Serial.println(telemetry_topic);

  getTelemetryPayload(telemetry, &telemetry);

  int ret = esp_mqtt_client_publish(
          mqtt_client,
          telemetry_topic,
          (const char*)az_span_ptr(telemetry),
          az_span_size(telemetry),
          0,
          DO_NOT_RETAIN_MSG);
  if (ret == -1)
  {
    Logger.error("Failed publishing");
    return 0;
  }
  else
  {
    Logger.info("Message published successfully: "+String(ret));
    return 1;
  }
}




#ifdef xxxyyy

#include <led.h>
#include <sevenseg.h>
#include "storage.h"
#include <log.h>
#include "local_config.h"
#include "AzIoTSasToken.h"
#include "SerialLogger.h"

#include <mqtt_client.h>

#include <az_core.h>
#include <azure_ca.h>

/*
#include <az_iot_hub_client.h>
#include <az_result.h>
#include <az_span.h>
*/

#include <esp_system.h>

#define AZURE_SDK_CLIENT_USER_AGENT "c/" AZ_SDK_VERSION_STRING "(ard;esp32)"
#define WEATHERSENSE_CLIENT_VERSION "v2.0"

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */

#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))

#define MESSAGE_MAX_LEN 256
#define MESSAGE_ACK_TIMEOUT_MS 10000
#define STATUS_MSG_MAX_LEN 100
#define TWIN_TIMEOUT 5000
#define SAS_TOKEN_DURATION_IN_MINUTES 60

#define MQTT_QOS0 0
#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0

extern int wakeCnt;

volatile bool hasIoTHub = false;

extern State *deviceState;
extern LedUtil *led;
extern SevenSeg *sevenSeg;
extern Storage *storage;

static bool messageSendingOn = true;
static bool statusRequested = false;

//unsigned long IotConn::sendTime = 0;

unsigned long connStartTime = 0;

static bool gotDeviceTwinReq = false;

static esp_mqtt_client_handle_t mqtt_client;
static az_iot_hub_client client;

static const char* host = IOT_CONFIG_IOTHUB_FQDN;
static const char* mqtt_broker_uri = "mqtts://" IOT_CONFIG_IOTHUB_FQDN;
static const char* device_id = IOT_CONFIG_DEVICE_ID;
static const int mqtt_port = 8883;

static char mqtt_client_id[128];
static char mqtt_username[128];
static char mqtt_password[200];
static uint8_t sas_signature_buffer[256];

static char telemetry_topic[128];
static uint8_t telemetry_payload[100];
static uint32_t telemetry_send_count = 0;

static az_span const twin_document_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("gettwin");
static az_span const twin_patch_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("reported_prop");
static az_span const directmsg_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("devicebound");
static az_span const methods_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("methods");

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
//bool _requestTwinGet(int requestId);

int twinGetId = -1;
int twinStatus = 0;

static AzIoTSasToken sasToken(
    &client,
    AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY),
    AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
    AZ_SPAN_FROM_BUFFER(mqtt_password));


void initializeIoTHubClient()
{
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);
  
  if (az_result_failed(az_iot_hub_client_init(
          &client,
          az_span_create((uint8_t*)host, strlen(host)),
          az_span_create((uint8_t*)device_id, strlen(device_id)),
          &options)))
  {
    logErr("Failed initializing Azure IoT Hub client");
    return;
  }

  size_t client_id_length;
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
  {
  logErr("Failed getting client id");
    return;
  }

  // Get the MQTT user name used to connect to IoT Hub
  if (az_result_failed(az_iot_hub_client_get_user_name(
          &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
  {
    logErr("Failed to get MQTT clientId, return code");
    return;
  }

  Serial.print("Client ID: "); Serial.println(mqtt_client_id);
  Serial.print("Username: " ); Serial.println(mqtt_username);
}

int initializeMqttClient()
{
  
  if (sasToken.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0)
  {
    logErr("Failed generating SAS token");
    return 1;
  } else {
    logMsg("SAS token generated");
    char tmpBuf[200];
    az_span_to_str(tmpBuf, 200, sasToken.Get());
    logMsg(tmpBuf);
  }

  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));
  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_port;
  mqtt_config.client_id = mqtt_client_id;
  mqtt_config.username = mqtt_username;
  mqtt_config.password = (const char*)az_span_ptr(sasToken.Get());
  mqtt_config.keepalive = 30;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = mqtt_event_handler;
  mqtt_config.user_context = NULL;
  mqtt_config.cert_pem = (const char*)ca_pem;

  mqtt_client = esp_mqtt_client_init(&mqtt_config);

  if (mqtt_client == NULL)
  {
    logErr("Failed creating mqtt client");
    return -1;
  }

  esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

  if (start_result != ESP_OK)
  {
    Serial.print("Could not start mqtt client; error code:");Serial.println( start_result);
    return -1;
  }
  
   logMsg("MQTT client started");

  return 0;
  
  
}

static az_iot_status _handleMethod(char *methodName, char *response, int response_size);


static bool _requestTwinGet() {
    Serial.print("req twin get ");
  //Logger.info("request twin get: "+String(requestId));
  char twin_document_topic_buffer[128];
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  Serial.println("calling get topic2");
  if (az_result_failed(az_iot_hub_client_twin_document_get_publish_topic(
      &client, twin_document_topic_request_id, twin_document_topic_buffer, sizeof(twin_document_topic_buffer), NULL)))
  {
    Serial.println("_requestTwinGet: Failed az_iot_hub_client_telemetry_get_publish_topic");
    return false;
  }
  Serial.print("_requestTwinGet: topic= ");
  Serial.println(twin_document_topic_buffer);

  const char* str = "";
  
  int msgId = esp_mqtt_client_publish(
          mqtt_client,
          twin_document_topic_buffer,
          NULL,
          0,
          1,
          NULL);
  if(msgId==0)
  {
    logErr("_requestTwinGet: Failed");
    return false;
  } else {
    logMsg("_requestTwinGet: msgId=", msgId);
  }
  return true;
}

static bool _replyTwinGet(az_span *msg) {
  char statusBuf[128];
  char twin_patch_topic_buffer[128];

    statusRequested = deviceState->updateState((char*)az_span_ptr(*msg));
    
    deviceState->getStatusString(statusBuf, sizeof(statusBuf));
    logMsgStr("_replyTwinGet: response: ", statusBuf);

    // publish status
    if (az_result_failed(az_iot_hub_client_twin_patch_get_publish_topic(
        &client,
        twin_patch_topic_request_id,
        twin_patch_topic_buffer,
        sizeof(twin_patch_topic_buffer),
        NULL)))
    {
      logErr("_replyTwinGet: Failed to get the Twin Patch topic.");
      return false;
    }
    logMsgStr("_replyTwinGet, sendtopic: ", twin_patch_topic_buffer);
    // Publish the reported property update.
    int msgId = 
    esp_mqtt_client_publish(
        mqtt_client,
        twin_patch_topic_buffer,
        statusBuf,
        0,
        MQTT_QOS1,
        NULL );
    if(msgId==0) 
    {
      logErr("_replyTwinGet: Failed publishing twin patch");
      return false;
    } else {
      logMsg("_replyTwinGet response sent, msgId=", msgId);
    }
    return true;

}

static bool _sendMethodResponse(az_iot_hub_client_method_request *method_request, az_iot_status status, az_span response) {
  char methods_response_topic_buffer[200];
  if( az_result_failed(az_iot_hub_client_methods_response_get_publish_topic(
      &client,
      method_request->request_id,
      (uint16_t)status,
      methods_response_topic_buffer,
      sizeof(methods_response_topic_buffer),
      NULL)))
  {
    logMsg("_sendMethodResponse: Failed to get the Methods Response topic");
    return false;
  }

  // Publish the method response.
  int msgId = esp_mqtt_client_publish(
      mqtt_client,
      methods_response_topic_buffer,
      (const char*)az_span_ptr(response),
      az_span_size(response),
      MQTT_QOS1,
      NULL);
  if(msgId ==0)
  {
    logMsg("_sendMethodResponse: Failed to publish the Methods response: MQTTClient return code ");
  } else {
    logMsg("_sendMethodResponse done, msgId=", msgId);
  }
}

static void _debugEvent(char *topic, char *msg) 
{
  Serial.print("topic=");Serial.println(topic);
  Serial.print("data=");Serial.println(msg);
}

static bool _is_twinGetReply(az_span *topic) {
  return (az_span_find(*topic, twin_document_topic_request_id)>0);
}

static bool _is_twinPatch(az_span *topic) {
  return (az_span_find( *topic, twin_patch_topic_request_id)>=0);
}
static bool _is_method(az_span *topic) {
  return (az_span_find( *topic, methods_topic_request_id)>=0);
}
static bool _is_c2d(az_span *topic) {
  return (az_span_find( *topic, directmsg_topic_request_id)>=0);
}
static uint8_t msgBuf[300];
static uint8_t topicBuf[200];
static char methodNameBuf[20];
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  int msgId;
  az_span topic = AZ_SPAN_FROM_BUFFER(topicBuf);
  az_span msg = AZ_SPAN_FROM_BUFFER(msgBuf);
  az_span method = AZ_SPAN_FROM_BUFFER(methodNameBuf);
  az_span slice;
  az_iot_hub_client_method_request methodReq;
  char responseBuf[100];
  int responseSize;
  
  logMsg("MQTT event ->", event->msg_id);

  slice = az_span_copy(topic, az_span_create((uint8_t*)event->topic, event->topic_len));
  az_span_ptr(slice)[0] = '\0';
  logMsgStr("\ttopic=", (char*)az_span_ptr(topic));

  slice = az_span_copy(msg, az_span_create((uint8_t*)event->data, event->data_len));
  az_span_ptr(slice)[0] = '\0';
  logMsgStr("\tdata=", (char*)az_span_ptr(msg));

  logMsg("\tdata len=", event->data_len);
  int i,r;
  
  switch (event->event_id)
  {
    case MQTT_EVENT_ERROR:
      logMsg("\tMQTT_EVENT_ERROR", event->error_handle->error_type);
      break;
    case MQTT_EVENT_CONNECTED:
      logMsg("\tMQTT_EVENT_CONNECTED");

      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
      if (r == -1)
      {
        Logger.error("Could not subscribe for cloud-to-device messages.");
      }
      else
      {
        Logger.info("Subscribed for cloud-to-device messages; message id:"  + String(r));
        twinGetId = r;
      }


      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1);
      if (r == -1)
      {
        Logger.error("Could not subscribe for twin response messages.");
      }
      else
      {
        Logger.info("Subscribed for twin response messages; message id:"  + String(r));
        
      }

      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, 1);
      if (r == -1)
      {
        Logger.error("Could not subscribe for client-methods messages.");
      }
      else
      {
        Logger.info("Subscribed for client-methods messages; message id:"  + String(r));
      }

      
      hasIoTHub = true;
      break;
    case MQTT_EVENT_DISCONNECTED:
      logMsg("\tMQTT_EVENT_DISCONNECTED");
      hasIoTHub = false;
      break;
    case MQTT_EVENT_SUBSCRIBED:
      logMsg("\tMQTT_EVENT_SUBSCRIBED");
      if(event->msg_id==twinGetId) {
        twinStatus = 1;
      }
      
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      logMsg("\tMQTT_EVENT_UNSUBSCRIBED");
      break;
    case MQTT_EVENT_PUBLISHED:
      logMsg("\tMQTT_EVENT_PUBLISHED");
      break;
    case MQTT_EVENT_DATA:
      logMsg("\tMQTT_EVENT_DATA");
      
      //_debugEvent( (char*)az_span_ptr(topic), (char*)az_span_ptr(msg));

      if(_is_twinGetReply(&topic)) {
        logMsg("\ttwin get  topic"); 
        _replyTwinGet( &msg);
      } else if( _is_twinPatch(&topic)) {
        logMsg("\ttwin patch  topic"); 
      } else if( _is_c2d(&topic)) {
        logMsg("\tc2d"); 
      } else if( _is_method(&topic)) {
        az_iot_hub_client_methods_parse_received_topic(&client, topic, &methodReq);
        az_span_to_str((char*)methodNameBuf, sizeof(methodNameBuf), methodReq.name);
        logMsgStr("\tdirectmethod, msg=", methodNameBuf);
        az_iot_status statCode = _handleMethod(methodNameBuf, responseBuf, sizeof(responseBuf));
        logMsgStr("\tdirectmethod, resp=", responseBuf);
        _sendMethodResponse(&methodReq, statCode, az_span_create_from_str(responseBuf));
      } else {
        logMsg("\tunexpected topic");
      }
      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      logMsg("\tMQTT_EVENT_BEFORE_CONNECT");
      break;
    default:
      logMsg("\tUNKNOWN");
      break;
  }
  return ESP_OK;
}

/*
static int twinMsgId;
static bool _subscribeTwin() 
{
  int msgId = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1);
  
  if (msgId == -1)
  {
    Serial.println("Could not subscribe device twin topic");
    return false;
  }
  twinMsgId = msgId;
  logMsg("!! _subscribeTwin: ", msgId);
  return true;
}

static bool _subscribeC2DMessage()
{
  int msgId = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, MQTT_QOS1);
  if (msgId == -1)
  {
    Serial.println("Could not subscribe C2D topic");
    return false;
  }
  logMsg("!! _subscribeC2DMessage: ", msgId);
  return  true;
}

static bool _subscribeMethods()
{
  int msgId = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, MQTT_QOS1);
  if (msgId == -1)
  {
    Serial.println("Could not subscribe METHODS topic");
    return false;
  }
  logMsg("!!_subscribeMethods: ", msgId);
  return  true;
}

*/

/*

void IotConn::close() 
{
  if(!hasIoTHub) return;
  esp_mqtt_client_disconnect(mqtt_client);
  unsigned long disconnStart = millis();
  while(hasIoTHub&&(int)(millis()-disconnStart)<5000) {
    delay(10);
  }
  (void)esp_mqtt_client_destroy(mqtt_client);

  hasIoTHub =false;

}
*/



/*
bool IotConn::subscribeTwin() {
  return _subscribeTwin();
}
bool IotConn::subscribeC2D() {
  return _subscribeC2DMessage();
}
bool IotConn::subscribeMethods() {
  return _subscribeMethods();
}
*/
bool requestTwinGet() {
  bool ret = false;
  if(twinStatus==1) {
    ret = _requestTwinGet();
    twinStatus = 2;
  } else if(twinStatus==0){
    logMsg("-----skipping twin get");
  }
  return ret;
}


bool sendData(char *msg) 
{
  if(!hasIoTHub) return false;

  logMsg("sendData: telemetry:");
  logMsg(msg);


  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to reflect the
  // current values of the properties.
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    logErr("sendData: Failed az_iot_hub_client_telemetry_get_publish_topic");
    return false;
  }
  az_span telemetry = az_span_create_from_str(msg);

  int msgId = esp_mqtt_client_publish(
          mqtt_client,
          telemetry_topic,
          (const char*)az_span_ptr(telemetry),
          az_span_size(telemetry),
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG);
  if(msgId==0)
  {
    logErr("sendData: Failed publishing");
    return false;
  }
  else
  {
    logMsg("sendData: published successfully, msgId=", msgId);
    return true;
  }
}

extern SevenSeg *sevenSeg;
extern LedUtil *led;
extern Storage *storage;
extern State *deviceState;


az_iot_status _handleMethod(char *methodName, char *response, int response_size)
{
  const char *statFmt = "{\"status\":%d,\"payload\":\"%s\"}";
  char payloadBuf[300];
  snprintf(payloadBuf, strlen(payloadBuf), "%s", "OK");
  logMsgStr("_handleMethod: ", (char*)methodName);
  
  az_iot_status iotStatus = AZ_IOT_STATUS_OK;
  int result = 200;
  if (strcmp(methodName, "meas") == 0)
  {
    logMsg("\tsend measurements");
    storage->getMeasurementString(payloadBuf, sizeof(payloadBuf));
  } 
  else if (strcmp(methodName, "led on") == 0)
  {
    logMsg("\tled on");
    led->ledOn();
  }
  else if (strcmp(methodName, "led off") == 0)
  {
    logMsg("\tled off");
    led->ledOff();
  }
  else if (strcmp(methodName, "7seg on") == 0)
  {
    logMsg("\tsevenseg on");
    deviceState->setSevenSegTime(false);
    deviceState->setSevenSegOn(true);
    if(sevenSeg!=NULL && sevenSeg->isConnected()) {
      sevenSeg->printHex(0xBEEF);
    }
  }
  else if (strcmp(methodName, "7seg off") == 0)
  {
    logMsg("\tsevenseg off");
    deviceState->setSevenSegTime(false);
    deviceState->setSevenSegOn(false);
    if(sevenSeg!=NULL && sevenSeg->isConnected()) {
      sevenSeg->clear();
    }
  }else if (strcmp(methodName, "7seg time") == 0)
  {
    logMsg("\tsevenseg time");
    deviceState->setSevenSegTime(true);
    deviceState->setSevenSegOn(true);
    if(sevenSeg!=NULL && sevenSeg->isConnected()) {
      sevenSeg->showTime();
    }
  }else if (strcmp(methodName, "stat") == 0)
  {
    const char *tpl = "wakeNum:%d, freeMem: %d, storedMeas: %d, freeMeasBuf: %d";
    snprintf(payloadBuf, sizeof(payloadBuf), tpl, 
      wakeCnt, (int)esp_get_free_heap_size(), storage->getNumStoredMeasurements(), storage->getRemainingLen()
    
    );
  }
  else
  {
    logMsgStr("_handleMethod: No method found: ", (char*)methodName);
    snprintf(payloadBuf, strlen(payloadBuf), "%s", "FAIL");
    result = 404;
    iotStatus = AZ_IOT_STATUS_BAD_REQUEST;
  }
  
  snprintf(response, response_size, statFmt, result, payloadBuf );

  return iotStatus;
}


#endif
