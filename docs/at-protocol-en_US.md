# AT Protocol Specification v2023.05.15


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
      "data": "7e2d02cf"
    }\n
    ```

### Command Types

- Read-only operation: `AT+<String>?\r`
- Execute operation: `AT+<String>!\r` or `AT+<String>=<Any>,<Any>...\r`
- Config operation: `AT+T<String>=<Any>\r`
- Reserved operation: `AT+<String>\r` or `AT+<String>=<Any>\r`

Note: The type `<Any>` means a number in string format or a quoted string (include escape characters).

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


## Interaction Examples

### Read-only operation

#### Get device ID

Request: `AT+ID?\r`

Response:

```json
\r{
  "type": 0,
  "name": "ID?",
  "code": 0,
  "data": "7e2d02cf"
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

#### Get version details

Request: `AT+VER?\r`

Response:

```json
\r{
  "type": 0,
  "name": "VER?",
  "code": 0,
  "data": {
    "at_api": "v0",
    "software": "2023.10.10",
    "hardware": "1"
  }
}\n
```

Note:

1. All version info is stored in strings.
1. The `hardware` version is chip revision on the ESP32 port.

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
      "state": 1,
      "opt_id": 0,
      "opt_detail": "240x240 Auto",
      "opts": {
        "2": "640x480 Auto",
        "1": "480x480 Auto",
        "0": "240x240 Auto"
      }
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
    "state": 1,
    "opt_id": 0,
    "opt_detail": "240x240 Auto"
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

#### Query all keys of KV objects from device flash

Request: `AT+KVQUERY?\r`

Response:

```json
\r{
  "type": 0,
  "name": "KVQUERY?",
  "code": 0,
  "data": [
    "__ver_num__",
    "edgelab#type_name#F0F569FD",
    "edgelab#type_name#30EEDD69",
    "edgelab#type_name#60DC70C1",
    "edgelab#type_name#1FEE9C53",
    "edgelab#type_name#D7A1D0BC",
    "sscma#version",
    "sscma#boot_count",
    "sscma#info"
    "sscma#conf#model_id",
    "sscma#conf#sensor_id",
  ]
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


#### Get trigger rules (Experimental)

Request: `AT+TRIGGER?\r`

Response:

```json
\r{
  "type": 0,
  "name": "TRIGGER?",
  "code": 0,
  "data": {
    "trigger_rules": "0,1,50,1,0,1|1,1,50,2,0,1"
  }
}\n
```

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
    "status": 2,
    "in4_info": {
      "ip": "192.168.0.100",
      "netmask": "255.255.255.0",
      "gateway": "192.168.0.1"
    },
    "in6_info": {
      "ip": ":::::::"
    },
    "config": {
      "name_type": 0,
      "name": "example_ssid",
      "security": 0,
      "password": "*************"
    }
  }
}\n
```

Status Table:

| Status | Value                                                             |
|--------|-------------------------------------------------------------------|
| `0`    | the WiFi is not initialized or joined                             |
| `1`    | the WiFi is joined, but the latest configuration is not applied |
| `2`    | the WiFi is joined, and the latest configuration is applied     |

#### Get Wi-Fi module version (no native network only)

Request: `AT+WIFIVER?\r`

Response:

```json
\r{
  "type": 0,
  "name": "WIFIVER",
  "code": 0,
  "data": {
    "ver": "v0.0.1"
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
    "status": 2,
    "config": {
      "client_id": "xiao_s3_7e2d02cf",
      "address": "example.local",
      "port": 1883,
      "username": "example_user",
      "password": "****************",
      "use_ssl": 0
    }
  }
}\n
```

Status Table:

| Status | Value                                                                       |
|--------|-----------------------------------------------------------------------------|
| `0`    | the MQTT server is not initialized or connected                             |
| `1`    | the MQTT server is connected, but the latest configuration is not applied |
| `2`    | the MQTT server is connected, and the latest configuration is applied     |

#### Get MQTT publish/subscribe topic

Request: `AT+MQTTPUBSUB?\r`

Response:

```json
\r{
  "type": 0,
  "name": "MQTTPUBSUB?",
  "code": 0,
  "data": {
    "config": {
      "pub_topic": "sscma/v0/xiao_s3_7e2d02cf/tx",
      "pub_qos": 0,
      "sub_topic": "sscma/v0/xiao_s3_7e2d02cf/rx",
      "sub_qos": 0
    }
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

Pattern: `AT+SENSOR=<SENSOR_ID,ENABLE/DISABLE,OPT_ID>\r`

Request: `AT+SENSOR=1,1,0\r`

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
      "state": 1,
      "opt_id": 0,
      "opt_detail": "240x240 Auto"
    }
  }
}\n
```

Note:
- The type of `OPT_ID` is `uint16_t` by default.
- You can set the rotate angle by `OPT_ID` for some sensors, the rotate angle is a multiple of 90 degrees, the default value is `0`.
- The rotate angle options table is here:
    | Rotate Angle | Bits            | Value |
    |--------------|-----------------|-------|
    | `0`          | `None`          | `0`   |
    | `90`         | `OPT_ID[12：12]` | `1`   |
    | `180`        | `OPT_ID[13：13]` | `1`   |
    | `270`        | `OPT_ID[14：14]` | `1`   |

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

#### Create or alter a KV object on device flash

Pattern: `AT+KVALTER=<"KEY","VALUE">\r`

Request: `AT+KVALTER="key_0","This is test value"\r`

Response:

```json
\r{
  "type": 0,
  "name": "KVALTER",
  "code": 0,
  "data": {
    "key": "key_0",
    "crc16_maxim": 478
  }
}\n
```

#### Query the value of a KV object from device flash

Pattern: `AT+KVQUERY=<"KEY">\r`

Request: `AT+KVQUERY="key_0"\r`

Response:

```json
\r{
  "type": 0,
  "name": "KVQUERY",
  "code": 0,
  "data": {
    "key": "key_0",
    "value": "This is test value",
    "crc16_maxim": 478
  }
}\n
```

#### Drop a KV object from device flash

Pattern: `AT+KVDROP=<"KEY">\r`

Request: `AT+KVDROP="key_0"\r`

Response:

```json
\r{
  "type": 0,
  "name": "KVDROP",
  "code": 0,
  "data": {
    "key": "key_0"
  }
}\n
```

#### Set trigger (Experimental)

Pattern: `AT+TRIGGER=<"TRIGGER_RULES">\r`

Request: `AT+TRIGGER="0,1,50,1,0,1|1,1,50,2,0,1"\r`

Response:

```json
\r{
  "type": 0,
  "name": "TRIGGER",
  "code": 0,
  "data": {
    "trigger_rules": "0,1,50,1,0,1|1,1,50,2,0,1"
  }
}\n
```

Note:

1. The trigger rules consist of single or multiple trigger rule(s), each rule is separated by `|`.
2. Each rule consists of 6 fields, separated by `,`, the fields are:
    - `CLASS_ID`: Integer that not negative, specified class for the evaluation of the condition. 
    - `CONDITION`: Integer, the condition type, `0` means `>`, `1` means `<`, `2` means `>=`, `3` means `<=`, `4` means `==`, `5` means `!=`, otherwise returns error.
    - `SCORE_THRESHOLD`: Integer between `[0, 100]`, the score threshold.
    - `GPIO_PIN`: Integer in a specific set, different boards may have different available GPIO pins, if the pin is not available, returns error. (e.g. There's 6 GPIO pins `{1,2,3,21,41,42}` available on XIAO ESP32-S3 board)
    - `INITIAL_LEVEL`: Integer, the initial level of the GPIO pin, `0` means low, `1` means high, otherwise returns error.
    - `TRIGGER_LEVEL`: Integer, the trigger level of the GPIO pin, `0` means low, `1` means high, otherwise returns error.
4. If any part of the trigger rules is invalid(missing, invalid value, etc.), the trigger rules will not be stored in the flash, and an error will be returned.
5. The trigger rules will be stored in the flash, the trigger will be checked after each invoke operation.
6. While invoking, the trigger could be dynamically updated by the `AT+TRIGGER` command.
7. The device side will not check if whether the GPIO pins is unique, or the initial level and trigger level is contrary for each rule, the configurator should check the rules by themself to have expected behavior.
8. If you want to disable the trigger, just call this API with a empty string.
9. The number of the rules is not limited, and for the class which does not actually exist, the rule would never be triggered.
10. The trigger rules in response will always by the configuration you set, if your configuration is not valid, the last valid configuration will not be changed.


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

| Name                   | Brief                                                                                   |
|------------------------|-----------------------------------------------------------------------------------------|
| `count()`              | Count the number of all results                                                         |
| `count(target,id)`     | Count the number of the results filter by a target id                                   |
| `max_score()`          | Get the max score of all results                                                        |
| `max_score(target,id)` | Get the max score of the results filter by a target id                                  |
| `led(enable)`          | When `enable` larger than `1`, turn on the status LED, otherwise off, always return `1` |
| `save_jpeg()`          | Save current frame to external TF card, returns `1` when success, otherwise `0`         |
| `set_gpio(pin,level)`  | Set the GPIO pin level, returns `1` when success, otherwise `0`                         |

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
1. Save JPEG specific:
    - External TF card may required to be formatted as FAT32 (cluster size at 8192) or exFat.
    - The default **save path** is `<Device(Product) Name> Export`, if the directory is not exist, it will be created automatically.
    - Inside the **save path**, each boot with this action triggered may create a new **save foler** named by a incremented number, the lastest folder name is stored in a hidden file named `.sscma` in the save path, user should not modify this file, otherwise error may occur.
    - Inside the **save folder**, each file is named in numeric format with the `.jpeg` extension, the number is the timestamp (the up time of the device in milliseconds) when the file is triggered to save, so it only guarantees the saved files will not be overwritten in at most 49 days (`max(uint32_t) / 1000 / 60 / 60 / 24` = 49.7 days).
    - Unexpected power off or reboot may cause the file system to be corrupted, data loss may occur.
    - If the external TF card failed to initialize or a file is failed to save, the function will return `0` and once its fails a **event response** will be emmitted.
    - The **event response** has a **tag** as same as the **command body** which used to **start invoke**, e.g.`INVOKE@ACTION` or `<Tag>@INVOKE@ACTION`, the `data` field contains a brief description of the error.

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

1. If the WiFi signal is not in a connectable range when calling this API, the configuration will still be stored in flash. Once the WiFi signal is in a connectable range, the device will try to connect to the AP automatically and run all chained setup operations if no error occured.
1. If you want to disconnect current WiFi or disable the WiFi, just call this API with a empty `NAME` string.
1. If the WiFi is successfully connected in the setup period or after called this API, when the WiFi signal is lost, the device will try to reconnect to the AP automatically.
1. You don't need to explicitly unsubscribe the MQTT topics or disconnecting the MQTT server before calling this API, we will do it automatically.
1. Any **Event response** will not contain the sensitive information, e.g. password string, it will be replaced by a `*` string with the same length.
1. The default timeout of the WiFi connect operation is `0.05s * retry_times`, the default retry times is `300`. The supervisor will poll the WiFi status every `5s` by default and try to reconnect if the WiFi status is not as expected.
1. The `name_type` is determined automatically, `0` means a SSID string, `1` means a BSSID string. The name or password format, security type are verified by the driver (e.g. the password length should >= 8 when the security level is above none), the network supervisor functionality is also based on the driver.


#### Set Wi-Fi status (no native network only)

Request: `AT+WIFISTA=2\r`

Response:

```json
\r{
  "type": 0,
  "name": "WIFISTA",
  "code": 0,
  "data": {
    "status": 2
  }
}\n
```

#### Set Wi-Fi module version (no native network only)

Request: `AT+WIFIVER="v0.0.1"\r`

Response:

```json
\r{
  "type": 0,
  "name": "WIFIVER",
  "code": 0,
  "data": {
    "ver": "v0.0.1"
  }
}\n
```


#### Set Wi-Fi IPv4 info (no native network only)

Request: `AT+WIFIIN4="192.168.1.123","255.255.255.0","192.168.1.1"\r`

Response:

```json
\r{
  "type": 0,
  "name": "WIFIIN4",
  "code": 0,
  "data": {
    "in4_info": {
      "ip": "192.168.1.123",
      "netmask": "255.255.255.0",
      "gateway": "192.168.250.1"
    }
  }
}\n
```


#### Set WiFi IPv6 info (no native network only)

Request: `AT+WIFIIN6="2001:bb6:434c:4d00:6045:6de9:33d0:f385"\r`

Response:

```json
\r{
  "type": 0,
  "name": "WIFIIN6",
  "code": 0,
  "data": {
    "in6_info": {
      "ip": "2001:bb6:434c:4d00:6045:6de9:33d0:f385"
    }
  }
}\n
```


#### Connect to a MQTT server

Pattern: `AT+MQTTSERVER=<"CLIENT_ID","ADDRESS",PORT,"USERNAME","PASSWORD",USE_SSL>\r`

Request: `AT+MQTTSERVER="example_device_id","example.local",1883,"example_user","example_password",0\r`

Response:

A synchronous **Operation response** will be sent after the configuration is stored in flash.

```json
\r{
  "type": 0,
  "name": "MQTTSERVER",
  "code": 0,
  "data": {
    "client_id": "example_device_id",
    "address": "example.local",
    "port": 1883,
    "username": "example_user",
    "password": "example_password",
    "use_ssl": 0
  }
}\n
```

Note:

1. If the MQTT server is not online when calling this API, the configuration will still be stored in flash. Once the MQTT server is in online, the device will try to connect to the server automatically and run all chained setup operations if no error occured.
1. If you want to disconnect current MQTT server or disable the MQTT, just call this API with a empty `ADDRESS` string.
1. If the MQTT server is successfully connected in the setup period or after called this API, when the MQTT server is offline, the device will try to reconnect to the server automatically.
1. You don't need to explicitly unsubscribe the MQTT topics or disconnecting the MQTT server before calling this API, we will do it automatically.
1. Any **Event response** will not contain the sensitive information, e.g. password string, it will be replaced by a `*` string with the same length.
1. The default timeout of the MQTT connect operation is `0.05s * retry_times`, the default retry times is `300`. The network supervisor will poll the MQTT status every `5s` by default and try to reconnect if the MQTT status is not as expected.
1. If no port is specified in the `PORT` argument, the default port `1883` (or `8883` for SSL) will be used. The `USE_SSL` is a boolean value, `0` means disable SSL, `1` means enable SSL. The username or password format, address are verified by the driver, the network supervisor functionality is also based on the driver.
1. The `client_id` is generated automatically if `CLIENT_ID` is a empty string, the default pattern is `"%s_%s_%ld"` the first `%s` is `PRODUCT_NAME_PREFIX`, the second `%s` is `PRODUCT_NAME_SUFFIX`, the `%ld` is a unique device ID.
1. The SSL functionality is in development currently, for testing purpose, you can try [EMQX](https://www.emqx.io/) as a MQTT server with SSL enabled.


#### Set MQTT server status (no native network only)

Request: `AT+MQTTSERVERSTA=2\r`

Response:

```json
/r{
  "type": 0,
  "name": "MQTTSERVERSTA",
  "code": 0,
  "data": {
    "status": 2
  }
}/n
```


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

