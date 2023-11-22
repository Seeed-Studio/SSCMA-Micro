# AT Protocol Specification v2023.11.22


## Transmission Layer

- USART Serial (Stateless)


## Interface Design

### Command Lexical Format

- Command header: `AT+`
- Command body: `<String>`
    - Command tag (optional): `<String>@`
    - Command name: `<String>`
    - Command arguments (optional): `=<Any>...`
- Command terminator: `\r`

Note:

1. Each character of the command should be a ASCII `char8_t`.
1. Command length is mutable, max length is limited to `4096 - 1` (include the terminator), due to safety factors and resource limitations.

#### Tagging

All `AT` commands support tagging with a `@` delimiter.

```
AT+<Tag:String>@<Body:String>\r
```

Example;
- Request: `AT+10@ID?\r`
- Response:

    ```json
    \r{
      "type": 0,
      "name": "10@ID?",
      "code": 0,
      "data": 657103005
    }\n
    ```

### Command Types

- Read-only operation: `AT+<String>?\r`
- Execute operation: `AT+<String>!\r` or `AT+<String>=<Any>,<Any>...\r`
- Config operation: `AT+T<String>=<Any>\r`
- Reserved operation: `AT+<String>\r` or `AT+<String>=<Any>\r`

Note: The type `<Any>` means a number in string format or a quoated string (include escape characters).

### Response Lexical Format

- Response header: `\r`
- Response body: `<JSON:String>`
- Response terminator: `\n`

Note:

1. Each character of the response should be a ASCII `char8_t`.
1. Each reply is the smallest primitive unit within all the reply contents, and we have taken measures to ensure that the results of different synchronous or asynchronous commands' replies do not overlap or interfere.
1. However, for stateless unreliable protocols, we cannot guarantee that certain data will not be modified or lost during the transmission process.

### Response Types

#### Normal Reply

- Operation response: `\r<JSON:String>\n`
- Event response: `\r<JSON:String>\n`
- Logging response: `\r<JSON:String>\n`

Common format of normal replies:

```json
\r{
  "type": <ResponseType:Unsigned>,
  "name": "<CommandName:String>",
  "code": <ResponseCode:Integer>,
  "data": <Any>
}\n
```

#### Unhandled Reply

- System stdout, stderr or crash log: `<String>\n...`

### Guidelines

1. **Read-only operation**:
    - Must have a sync/async **Operation reply**.
1. **Execute operation** or **Config operation**:
    - Must have a sync/async **Operation reply**.
    - May have a sync/async **Event reply**.
    - Must have a sync **Logging reply** when error occured before the execution.
1. **Reserved operation**:
    - May have sync/async replies with no specified reply types.
1. **Non-Operation** while monitoring the device outputs:
    - May recieve **Event reply**.
    - May recieve **Unhandled reply**.


#### Graph

```
            Receive Request
                    |
                    v
            Parse AT+ Request
                    |
  Parse CMD <-------+------> Logging Response
      |     Success     Fail       ^
      |                            |
      +----------------------------+
      | Unknown CMD or ARGC Mismatch
      |
      | Success
      |
      v        +-> Operation Response
  Execute CMD -+
               +----------------------> Event Response(s)
```


## Intereaction Examples

### Read-only operation

#### Get device ID

Request: `AT+ID?\r`

Response:

```json
\r{
  "type": 0,
  "name": "ID?",
  "code": 0,
  "data": 657103005
}\n
```

#### Get device name

Request: `AT+NAME?\r`

Response:

```json
\r{
  "type": 0,
  "name": "NAME?",
  "code": 0,
  "data": "Seeed Studio XIAO (ESP32-S3)"
}\n
```

#### Get device status

Request: `AT+STAT?\r`

Response:

```json
\r{
  "type": 0,
  "name": "STAT?",
  "code": 0,
  "data": {
    "boot_count": 1631,
    "is_ready": 1
  }
}\n
```

#### Get version deatils

Request: `AT+VER?\r`

Response:

```json
\r{
  "type": 0,
  "name": "VER?",
  "code": 0,
  "data": {
    "software": "2023.10.10",
    "hardware": "1"
  }
}\n
```

Note:

1. All version info is store in strings.
1. The `hardware` version is chip revision on ESP32 port.

#### Get available algorithms

Request: `AT+ALGOS?\r`

Response:

```json
\r{
  "type": 0,
  "name": "ALGOS?",
  "code": 0,
  "data": [
    {
      "type": 4,
      "categroy": 3,
      "input_from": 1
    },
    {
      "type": 3,
      "categroy": 1,
      "input_from": 1
    },
    {
      "type": 2,
      "categroy": 2,
      "input_from": 1
    },
    {
      "type": 1,
      "categroy": 1,
      "input_from": 1
    }
  ]
}\n
```

Note: `"input_from": <SensorType:Unsigned>`.

#### Get available models

Request: `AT+MODELS?\r`

Response:

```json
\r{
  "type": 0,
  "name": "MODELS?",
  "code": 0,
  "data": [
    {
      "id": 2,
      "type": 3,
      "address": 5242880,
      "size": 267024
    },
    {
      "id": 5,
      "type": 3,
      "address": 4194304,
      "size": 267024
    }
  ]
}\n
```

Note: `"type": <AlgorithmType:Unsigned>`.

#### Get current model info

Request: `AT+MODEL?\r`

Response:

```json
\r{
  "type": 0,
  "name": "MODEL?",
  "code": 0,
  "data": {
    "id": 2,
    "type": 3,
    "address": 5242880,
    "size": 267024
  }
}\n
```

Note: `"type": <AlgorithmType:Unsigned>`.

#### Get available sensors

Request: `AT+SENSORS?\r`

Response:

```json
\r{
  "type": 0,
  "name": "SENSORS?",
  "code": 0,
  "data": [
    {
      "id": 1,
      "type": 1,
      "state": 1
    }
  ]
}\n
```

Note: `"type": <SensorType:Unsigned>`.

#### Get current sensor info

Request: `AT+SENSOR?\r`

Response:

```json
\r{
  "type": 0,
  "name": "SENSOR?",
  "code": 0,
  "data": {
    "id": 1,
    "type": 1,
    "state": 1
  }
}\n
```

Note: `"type": <SensorType:Unsigned>`.

#### Get sample status

Request: `AT+SAMPLE?\r`

Response:

```json
\r{
  "type": 0,
  "name": "SAMPLE?",
  "code": 0,
  "data": 0
}\n
```

Note: `"data": 0` means not sampling, `"data": 1` means sampling.

#### Get invoke status

Request: `AT+INVOKE?\r`

Response:

```json
\r{
  "type": 0,
  "name": "INVOKE?",
  "code": 0,
  "data": 0
}\n
```

Note: `"data": 0` means not invoking, `"data": 1` means invoking.

#### Get info string from device flash

Request: `AT+INFO?\r`

Response:

```json
\r{
  "type": 0,
  "name": "INFO?",
  "code": 0,
  "data": {
    "crc16_maxim": 43073,
    "info": "Hello World!"
  }
}\n
```

#### Get score threshold

Request: `AT+TSCORE?\r`

Response:

```json
\r{
  "type": 0,
  "name": "TSCORE?",
  "code": 0,
  "data": 60
}\n
```

1. Available while invoking using a specified algorithm.
1. Response `data` is the last valid config value.

#### Get IoU threshold

Request: `AT+TIOU?\r`

Response:

```json
\r{
  "type": 0,
  "name": "TIOU?",
  "code": 0,
  "data": 55
}\n
```

Note:

1. Available while invoking using a specified algorithm.
1. Response `data` is the last valid config value.


#### Get action info (Experimental)

Request: `AT+ACTION?\r`

Response:

```json
\r{
  "type": 0,
  "name": "ACTION?",
  "code": 0,
  "data": {
    "crc16_maxim": 19753,
    "action": "((max_score(target,0)>=80)&&led(1))||led(0)"
  }
}\n
```

Note: `crc16_maxim` is calculated on action string.

#### Get WiFi status

Request: `AT+WIFI?\r`

Response:

```json
\r{
  "type": 0,
  "name": "WIFI?",
  "code": 0,
  "data": {
    "joined": 1,
    "config": {
      "name_type": 0,
      "name": "example_ssid",
      "security": 0,
      "password": "*************"
    }
  }
}\n
```

#### Get MQTT server status

Request: `AT+MQTTSERVER?\r`

Response:

```json
\r{
  "type": 0,
  "name": "MQTTSERVER?",
  "code": 0,
  "data": {
    "connected": 1,
    "config": {
      "client_id": "example_device_id",
      "address": "example.local:1234",
      "username": "example_user",
      "password": "****************",
      "use_ssl": 0
    }
  }
}\n
```

#### Get MQTT publish/subscribe topic

Request: `AT+MQTTPUBSUB?\r`

Response:

```json
\r{
  "type": 0,
  "name": "MQTTPUBSUB?",
  "code": 0,
  "data": {
    "pub_topic": "sscma/test_pub",
    "pub_qos": 0,
    "sub_topic": "sscma/test_sub",
    "sub_qos": 0
  }
}\n
```


### Execute operation

#### Load a model by model ID

Pattern: `AT+MODEL=<MODEL_ID>\r`

Request: `AT+MODEL=2\r`

Response:

```json
\r{
  "type": 0,
  "name": "MODEL",
  "code": 0,
  "data": {
    "model": {
      "id": 2,
      "type": 3,
      "address": 5242880,
      "size": 267024
    }
  }
}\n
```

Note: `"model": {..., "type": <AlgorithmType:Unsigned>,  ...}`.

####  Set a default sensor by sensor ID

Pattern: `AT+SENSOR=<SENSOR_ID,ENABLE/DISABLE>\r`

Request: `AT+SENSOR=1,1\r`

Response:

```json
\r{
  "type": 0,
  "name": "SENSOR",
  "code": 0,
  "data": {
    "sensor": {
      "id": 1,
      "type": 1,
      "state": 1
    }
  }
}\n
```

#### Sample data from current sensor

Pattern: `AT+SAMPLE=<N_TIMES>\r`

Request: `AT+SAMPLE=1\r`

Response:

```json
\r{
  "type": 0,
  "name": "SAMPLE",
  "code": 0,
  "data": {
    "sensor": {
      "id": 1,
      "type": 1,
      "state": 1
    }
  }
}\n
```

Events:

```json
\r{
  "type": 1,
  "name": "SAMPLE",
  "code": 0,
  "data": {
    "count": 8,
    "image": "<BASE64JPEG:String>"
  }
}\n
```

#### Invoke for N times

Pattern: `AT+INVOKE=<N_TIMES,DIFFERED,RESULT_ONLY>\r`

Request: `AT+INVOKE=1,0,1\r`

Response:

```json
\r{
  "type": 0,
  "name": "INVOKE",
  "code": 0,
  "data": {
    "model": {
      "id": 2,
      "type": 3,
      "address": 5242880,
      "size": 267024
    },
    "algorithm": {
      "type": 3,
      "category": 1,
      "input_from": 1,
      "config": {
        "tscore": 60,
        "tiou": 50
      }
    },
    "sensor": {
      "id": 1,
      "type": 1,
      "state": 1
    }
  }
}\n
```

Events:

```json
\r{
  "type": 1,
  "name": "INVOKE",
  "code": 0,
  "data": {
    "count": 8,
    "perf": [
      8,
      365,
      0
    ],
    "boxes": [
      [
        87,
        83,
        77,
        65,
        70,
        0
      ]
    ]
  }
}\n
```

Note:

1. `"model": {..., "type": <AlgorithmType:Unsigned>,  ...}`.
1. `"input_from": <SensorType:Unsigned>`.
1. `DIFFERED` means the event reply will only be sent if the last result is different from the previous result (compared by geometry and score).
1. `RESULT_ONLY` means the event reply will only contain the result data, otherwise the event reply will contain the image data.

#### Store info string to device flash

Pattern: `AT+INFO=<"INFO_STRING">\r`

Request: `AT+INFO="Hello World!"\r`

Response:

```json
\r{
  "type": 0,
  "name": "INFO",
  "code": 0,
  "data": {
    "crc16_maxim": 43073
  }
}\n
```

Note: Max string length is `4096 - strlen("AT+INFO=\"\"\r") - 1 - TagLength`.

#### Set action trigger (Experimental)

Pattern: `AT+ACTION=<"EXPRESSION">\r`

Request: `AT+ACTION="((max_score(target,0)>=80)&&led(1))||led(0)"\r`

Response:

```json
\r{
  "type": 0,
  "name": "ACTION",
  "code": 0,
  "data": {
    "crc16_maxim": 19753,
    "action": "((max_score(target,0)>=80)&&led(1))||led(0)"
  }
}\n
```

Events:

```json
\r{
  "type": 0,
  "name": "ACTION",
  "code": <Integer>,
  "data": {
    "crc16_maxim": <Integer>,
    "action": "<String>"
  }
}\n
```

Function map:

| Name                   | Brief                                                              |
|------------------------|--------------------------------------------------------------------|
| `count()`              | Count the number of all results                                    |
| `count(target,id)`     | Count the number of the results filter by a target id              |
| `max_score()`          | Get the max score of all results                                   |
| `max_score(target,id)` | Get the max score of the results filter by a target id             |
| `led(enable)`          | When `enable` larger than `1`, turn the status LED, otherwise else |

Note:

1. Default events reply could be emmited by the failure of condition evaluation or by function calls.
1. When evaluation fail, if it is a condition function call, identifier or operator, its value will be `0`.
1. Complex conditions are supported, e.g.:
    - `((count(target,0)>=3)&&led(1))||led(0)`
    - `((max_score(target,0)>=80)&&led(1))||led(0)`
    - `(((count(target,0)>=3)||(max_score(target,0)>=80))&&led(1))||led(0)`
    - `(count(target,0)-count(target,1))>=(count(target,3)+count(target,4)+count(target,5))`
    - `(count(target,0)>1)&&(count(target,3)<=5)||((count(target,4)+count(target,2))<10)`
1. Supported expression tokens:
    - Unsigned constant.
    - Integeral identifier.
    - Integeral and non-mutate argument function call.
    - Binary operator.
    - Compare operator.
    - Logic operator.

#### Connect to a WiFi AP

Pattern: `AT+WIFI=<"NAME",SECURITY,"PASSWORD">\r`

Request: `AT+WIFI="example_ssid",0,"example_password"\r`

Response:

A synchronous **Operation response** will be sent after the configuration is stored in flash.

```json
\r{
  "type": 0,
  "name": "WIFI",
  "code": 0,
  "data": {
    "name_type": 0,
    "name": "example_ssid",
    "security": 0,
    "password": "example_password"
  }
}\n
```

The next **Event response** will be sent after the configuration is applied.

```json
\r{
  "type": 1,
  "name": "WIFI",
  "code": 0,
  "data": {
    "joined": 1,
    "config": {
      "name_type": 0,
      "name": "example_ssid",
      "security": 0,
      "password": "*************"
    }
  }
}\n
```

**Before** we have recieved the **Event response** corresponding to our request, we may recieve other **Event responses** that related to the request, which is emmitted by the chained setup operations if we have set them (e.g. a MQTT server with a publish or subscribe topic).

```json
\r{
  "type": 1,
  "name": "WIFI@MQTTSERVER@MQTTPUBSUB",
  "code": 0,
  "data": {
    "pub_topic": "sscma/test_pub",
    "pub_qos": 0,
    "sub_topic": "sscma/test_sub",
    "sub_qos": 0
  }
}\n

\r{
  "type": 1,
  "name": "WIFI@MQTTSERVER",
  "code": 0,
  "data": {
    "connected": 1,
    "config": {
      "client_id": "example_device_id",
      "address": "example.local:1234",
      "username": "example_user",
      "password": "****************",
      "use_ssl": 0
    }
  }
}\n
```

This means that if we had set a MQTT server with a publish or subscribe topic before, when we changing the WiFi configuration, the MQTT server will be reconnected automatically.

Also the chained setup operations have the ability to inherit the caller's name tag, we can easily identify the source of the events.

Security Table (Experimental):

| Security    | Value |
|-------------|-------|
| `AUTO`      | `0`   |
| `NONE`      | `1`   |
| `WEP`       | `2`   |
| `WPA1_WPA2` | `3`   |
| `WPA2_WPA3` | `4`   |
| `WPA3`      | `5`   |

Currently not supported EAP security, the security configuration will be ignored if the security is not in the table.

Note:

1. If the WiFi signal is not in a connectable range when calling this API, the **Event response** will be marked as fail or timeout, but the configuration will still be stored in flash. Once the WiFi signal is in a connectable range, the device will try to connect to the AP automatically and run all chained setup operations if no error occured.
1. If you want to disconnect current WiFi or disable the WiFi, just call this API with a empty `NAME` string.
1. If the WiFi is successfully connected in the setup period or after called this API, when the WiFi signal is lost, the device will try to reconnect to the AP automatically with a **Event response** tagged with a name `SUPERVISOR` (e.g. `SUPERVISOR@WIFI`), the chained setup operations will follow this syntax too.
1. You don't need to explicitly unsubscribe the MQTT topics or disconnecting the MQTT server before calling this API, we will do it automatically.
1. Any **Event response** will not contain the sensitive information, e.g. password string, it will be replaced by a `*` string with the same length.
1. The default timeout of the WiFi connect operation is `1.5s * retry_times`, the default retry times is `5` and will be decremnted by `1` while trying to change the network status. The network supervisor will poll the WiFi status every `10s` by default and try to reconnect if the WiFi status is not as expected.
1. The `name_type` is determined automatically, `0` means a SSID string, `1` means a BSSID string. The name or password format, security type are verified by the driver, the network supervisor functionality is also based on the driver.

#### Connect to a MQTT server

Pattern: `AT+MQTTSERVER=<"CLIENT_ID","ADDRESS:PORT","USERNAME","PASSWORD",USE_SSL>\r`

Request: `AT+MQTTSERVER="example_device_id","example.local:1234","example_user","example_password",0\r`

Response:

A synchronous **Operation response** will be sent after the configuration is stored in flash.

```json
\r{
  "type": 0,
  "name": "MQTTSERVER",
  "code": 0,
  "data": {
    "client_id": "example_device_id",
    "address": "example.local:1234",
    "username": "example_user",
    "password": "example_password",
    "use_ssl": 0
  }
}\n
```

The next **Event response** will be sent after the configuration is applied.

```json
\r{
  "type": 1,
  "name": "MQTTSERVER",
  "code": 0,
  "data": {
    "connected": 1,
    "config": {
      "client_id": "example_device_id",
      "address": "example.local:1234",
      "username": "example_user",
      "password": "****************",
      "use_ssl": 0
    }
  }
}\n
```

**Before** we have recieved the **Event response** corresponding to our request, we may recieve other **Event responses** that related to the request, which is emmitted by the chained setup operations if we have set them (e.g. a publish or subscribe topic).

```json
\r{
  "type": 0,
  "name": "MQTTPUBSUB?",
  "code": 0,
  "data": {
    "pub_topic": "sscma/test_pub",
    "pub_qos": 0,
    "sub_topic": "sscma/test_sub",
    "sub_qos": 0
  }
}\n
```

This means that if we had set a publish or subscribe topic before, when we changing the MQTT server configuration, the publish or subscribe topic will be reconfigured automatically.

Also the chained setup operations have the ability to inherit the caller's name tag, we can easily identify the source of the events.

Note:

1. If the MQTT server is not online when calling this API, the **Event response** will be marked as fail or timeout, but the configuration will still be stored in flash. Once the MQTT server is in online, the device will try to connect to the server automatically and run all chained setup operations if no error occured.
1. If you want to disconnect current MQTT server or disable the MQTT, just call this API with a empty `ADDRESS:PORT` string.
1. If the MQTT server is successfully connected in the setup period or after called this API, when the MQTT server is offline, the device will try to reconnect to the server automatically with a **Event response** tagged with a name `SUPERVISOR` (e.g. `SUPERVISOR@MQTTSERVER`), the chained setup operations will follow this syntax too.
1. You don't need to explicitly unsubscribe the MQTT topics or disconnecting the MQTT server before calling this API, we will do it automatically.
1. Any **Event response** will not contain the sensitive information, e.g. password string, it will be replaced by a `*` string with the same length.
1. The default timeout of the MQTT connect operation is `1.5s * retry_times`, the default retry times is `5` and will be decremnted by `1` while trying to change the network status. The network supervisor will poll the MQTT status every `10s` by default and try to reconnect if the MQTT status is not as expected.
1. If no port is specified int the `ADDRESS:PORT` string, the default port `1883` will be used. The `USE_SSL` is a boolean value, `0` means disable SSL, `1` means enable SSL. The username or password format, address are verified by the driver, the network supervisor functionality is also based on the driver.
1. The `client_id` is generated automatically if `CLIENT_ID` is a empty string, the default pattern is `"sscma_%s_%s_%ld"` the first `%s` is `VENDOR_PREFIX`, the second `%s` is `VENDOR_CHIP_NAME`, the `%ld` is a unique device ID.
1. The SSL functionality is in development currently.

#### Public/Subscribe MQTT topics

Pattern: `AT+MQTTPUBSUB=<"PUB_TOPIC",PUB_QOS,"SUB_TOPIC",SUB_QOS>\r`

Request: `AT+MQTTPUBSUB="sscma/test_pub",0,"sscma/test_sub",0\r`

Response:

A synchronous **Operation response** will be sent after the configuration is stored in flash.

```json
\r{
  "type": 0,
  "name": "MQTTPUBSUB",
  "code": 0,
  "data": {
    "pub_topic": "sscma/test_pub",
    "pub_qos": 0,
    "sub_topic": "sscma/test_sub",
    "sub_qos": 0
  }
}\n
```

The next **Event response** will be sent after the configuration is applied.

```json
\r{
  "type": 0,
  "name": "MQTTPUBSUB?",
  "code": 0,
  "data": {
    "pub_topic": "sscma/test_pub",
    "pub_qos": 0,
    "sub_topic": "sscma/test_sub",
    "sub_qos": 0
  }
}\n
```

**Before** we have recieved the **Event response** corresponding to our request, we may recieve an **Unnamed response** on the topic `sscma/v0/discover` by default if the device is connected to the MQTT server and the topic is subscribed successfully, which ensures that you're able to discover the device in the network.

```json
\r{
  "pub_topic": "sscma/test_pub",
  "pub_qos": 0,
  "sub_topic": "sscma/test_sub",
  "sub_qos": 0
}\n
```

Note:

1. If the device is not connected to the MQTT server when calling this API, the **Event response** will be marked as fail or timeout, but the configuration will still be stored in flash. Once the device is connected to the MQTT server, the device will try to subscribe the topic automatically and run all chained setup operations if no error occured.
1. If you want to use default generated publish or subscribe topic, just call this API with a empty `PUB_TOPIC` or `SUB_TOPIC` string, unsubscribe or unpublic a topic is not supported currently. The publish or subscribe topic format is verified by the driver, the network supervisor functionality is also based on the driver.
1. When network status changes, the device will try to subscribe the topic automatically by the chained setup operations once the connection to the MQTT server is established.
1. The default timeout of the MQTT subscribe operation is `50ms * poll_times`, the default poll times is `30` and will be decremnted by `1` while trying to subscribe the topic. The network supervisor will not monitor the public or subscribe status, which means the device may lost connection to the MQTT server if the topic is not subscribed successfully (a hardware reset may needed).
1. The default `pub_topic` is generated in the pattern `"sscma/%s/%s_%s_%ld/tx"`, the first `%s` is `SSCMA_AT_API_MAJOR_VERSION` (`v0` currently), the second `%s` is `VENDOR_PREFIX`, the third `%s` is `VENDOR_CHIP_NAME`, the `%ld` is a unique device ID. The deault `sub_topic` is generated in the same pattern but with `rx` instead of `tx`.
1. Once the device is connected to the MQTT server, the device will try to send a 'hello' message to the discovery topic `sscma/v0/discover` by default, the `v0` is the `SSCMA_AT_API_MAJOR_VERSION`.
1. You're able to run AT commands on the subscribed topic of the device, the device will reply with the result on the publish topic and serial port (if connected).


#### Set SSL certificate (Draft)

Comming soon...


### Config operation

#### Set score threshold

Pattern: `AT+TSCORE=<SCORE_THRESHOLD>\r`

Request: `AT+TSCORE=60\r`

Response:

```json
\r{
  "type": 0,
  "name": "TSCORE",
  "code": 0,
  "data": 60
}\n
```

1. Valid range `[0, 100]`.
1. Available while invoking using a specified algorithm.
1. Response `data` is the last valid config value.

#### Set IoU threshold

Pattern: `AT+TIOU=<IOU_THRESHOLD>\r`

Request: `AT+TIOU=55\r`

Response:

```json
\r{
  "type": 0,
  "name": "TIOU",
  "code": 0,
  "data": 55
}\n
```

Note:

1. Valid range `[0, 100]`.
1. Available while invoking using a specified algorithm.
1. Response `data` is the last valid config value.

### Reserved operation

#### Set LED status

Pattern: `AT+LED=<ENABLE/DISABLE>\r`

Request: `AT+LED=1\r`

No-reply.

#### List available commands

Request: `AT+HELP?\r`

Response:

```
Command list:\n
  AT+INVOKE=<N_TIMES,RESULT_ONLY>\n
    Invoke for N times (-1 for infinity loop)\n
  <String>\n...
```

#### Reboot device

Request: `AT+RST\r`

No-reply.

#### Stop all running tasks

Request: `AT+BREAK\r`

Response:

```json
\n{
  "type": 0,
  "name": "BREAK",
  "code": 0,
  "data": 1530898
}\r
```

Note: The `"data": <Unsigned>` filed is timestamp.

#### Yield I/O task for 10ms

Request: `AT+YIELD\r`

No-reply.


## Lookup Table

### Response Type

```json
"type": <Key:Unsigned>
```

| Key | Value              |
|-----|--------------------|
| `0` | Operation response |
| `1` | Event response     |
| `2` | Logging response   |

### Response Code

```json
"code": <Key:Integer>
```

| Key | Value                   |
|-----|-------------------------|
| `0` | Success                 |
| `1` | Try again               |
| `2` | Logic error             |
| `3` | Timeout                 |
| `4` | IO error                |
| `5` | Invalid argument        |
| `6` | Out of memory           |
| `7` | Busy                    |
| `8` | Not supported           |
| `9` | Operation not permitted |

### Algorithm Type

```json
"type": <Key:Unsigned>
```

| Key | Value     |
|-----|-----------|
| `0` | Undefined |
| `1` | FOMO      |
| `2` | PFLD      |
| `3` | YOLO      |
| `4` | IMCLS     |


### Algorithm Category

```json
"category": <Key:Unsigned>
```

| Key | Value          |
|-----|----------------|
| `0` | Undefined      |
| `1` | Detection      |
| `2` | Pose           |
| `3` | Classification |

### Sensor Type

```json
"type": <Key:Unsigned>
```

| Key | Value     |
|-----|-----------|
| `0` | Undefined |
| `1` | Camera    |

### Sensor State

```json
"state": <Key:Unsigned>
```

| Key | Value      |
|-----|------------|
| `0` | Unknown    |
| `1` | Registered |
| `2` | Available  |
| `3` | Locked     |

### Data Type (Sample or Invoke)

```json
"<Key:String>": "<Data:String>"
```

Key:

- `image`
- `audio`
- `raw`

Data: `<BASE64:String>`

### Performance Type

```json
"perf": [<Value:JSONList>]
```

Value:

```json
[
    8,   // preprocess time ms
    365, // run time ms
    1    // postprocess time ms
]
```

### Box Type

```json
"boxes": [<Value:JSONList>]
```

Value:

```json
[
    87, // x
    83, // y
    77, // w
    65, // h
    70, // score
    0   // target id
]
```

### Point Type

```json
"points":  [<Value:JSONList>]
```

Value:

```json
[
    87, // x
    83, // y
    70, // score
    0   // target id
]
```

### Class Type

```json
"classes": [<Value:JSONList>]
```

Value:

```json
[
    70  // score
    0,  // target id
]
```
