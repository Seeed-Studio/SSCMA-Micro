# AT Protocol Specification v2023.09.08


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
      "data": "B63E3DA5"
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
  "data": "B63E3DA5"
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

Note: `"model": {..., "type": <AlgorithmType:Unsigned>,  ...}`.

#### Get version deatils

Request: `AT+VER?\r`

Response:

```json
\r{
  "type": 0,
  "name": "VER?",
  "code": 0,
  "data": {
    "software": "2023.09.08",
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
      "input_from": 1,
      "config": {
        "tscore": 50
      }
    },
    {
      "type": 3,
      "categroy": 1,
      "input_from": 1,
      "config": {
        "tscore": 50,
        "tiou": 40
      }
    },
    {
      "type": 2,
      "categroy": 2,
      "input_from": 1,
      "config": {}
    },
    {
      "type": 1,
      "categroy": 1,
      "input_from": 1,
      "config": {
        "tscore": 80
      }
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
      "address": "0x500000",
      "size": "0x41310"
    },
    {
      "id": 5,
      "type": 3,
      "address": "0x400000",
      "size": "0x41310"
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
    "address": "0x500000",
    "size": "0x41310"
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
    "crc16_maxim": 16828,
    "cond": "(count(target,0)>=3)||(max_score(target,0)>=80)",
    "true": "LED=1",
    "false_or_exception": "LED=0"
  }
}\n
```

Note: `crc16_maxim` is calculated on string `cond + '\t' + true + '\t' + false_or_exception`.

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
      "address": "0x500000",
      "size": "0x41310"
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
    "image": "<BASE64JPEG:String>"
  }
}\n
```

#### Invoke for N times

Pattern: `AT+INVOKE=<N_TIMES,RESULT_ONLY>\r`

Request: `AT+INVOKE=1,1\r`

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
      "address": "0x500000",
      "size": "0x41310"
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

Pattern: `AT+ACTION=<"COND","TRUE_CMD","FALSE_OR_EXCEPTION_CMD">\r`

Request: `AT+ACTION="(count(target,0)>=3)||(max_score(target,0)>=80)","LED=1","LED=0"\r`

Response:

```json
\r{
  "type": 0,
  "name": "ACTION",
  "code": 0,
  "data": {
    "crc16_maxim": 16828,
    "cond": "(count(target,0)>=3)||(max_score(target,0)>=80)",
    "true": "LED=1",
    "false_or_exception": "LED=0"
  }
}\n
```

Events:

```json
\r{
  "type": 1,
  "name": "ACTION",
  "code": 0,
  "data": {
    "true": "LED=1"
  }
}\n
```

Function map:

| Name                   | Brief                                                  |
|------------------------|--------------------------------------------------------|
| `count()`              | Count the number of all results                        |
| `count(target,id)`     | Count the number of the results filter by a target id  |
| `max_score()`          | Get the max score of all results                       |
| `max_score(target,id)` | Get the max score of the results filter by a target id |

Note:

1. The `crc16_maxim` is calculated on string `cond + '\t' + true + '\t' + false_or_exception`.
1. Only have events reply when condition evaluation is `true`.
1. When evaluation fail, if it is a condition function call, identifier or operator, its value will be `0` and with no exception reply.
1. Complex conditions are supported, e.g.:
    - `count(target,0)>=3`
    - `max_score(target,0)>=80`
    - `(count(target,0)>=3)||(max_score(target,0)>=80)`
    - `(count(target,0)-count(target,1))>=(count(target,3)+count(target,4)+count(target,5))`
    - `(count(target,0)>1)&&(count(target,3)<=5)||((count(target,4)+count(target,2))<10)`
1. Supported expression tokens:
    - Unsigned constant.
    - Integeral identifier.
    - Integeral and non-mutate argument function call.
    - Binary operator.
    - Compare operator.
    - Logic operator.

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
