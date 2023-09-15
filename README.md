# SSCMA-Micro CPP SDK

![SSCMA](docs/images/sscma.png)

This is a cross-platform machine learning inference framework designed for embedded devices, serving for [SenseCraft Model Assistant by Seeed Studio](https://github.com/Seeed-Studio/SSCMA) and providing functionalities such as digital signal processing, model inference, AT command interaction, and more.

## Structure

```
sscma-micro
├── core
│   ├── algorithm
│   ├── data
│   ├── engine
│   ├── synchronize
│   └── utils
├── docs
│   └── images
├── porting
│   └── espressif
│       └── boards
│           ├── espressif_esp32s3_eye
│           └── seeed_xiao_esp32s3
├── sscma
│   ├── callback
│   │   └── internal
│   ├── interpreter
│   └── repl
└── third_party
    ├── FlashDB
    └── JPEGENC
```

## License

This project is released under the [MIT license](LICENSES).
