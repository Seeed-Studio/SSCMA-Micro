#include "ma_server_at.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <stack>

#include "callback/algorithm.hpp"
#include "callback/common.hpp"
#include "callback/config.hpp"
#include "callback/info.hpp"
#include "callback/invoke.hpp"
#include "callback/model.hpp"
#include "callback/mqtt.hpp"
#include "callback/resource.hpp"
#include "callback/sample.hpp"
#include "callback/sensor.hpp"
#include "callback/wifi.hpp"
#include "callback/trigger.hpp"


namespace ma {

using namespace ma::server::callback;

// static constexpr char MA_TAG[] = "ma::server::ATServer";

ATService::ATService(const std::string& name, const std::string& desc, const std::string& args, ATServiceCallback cb) : name(name), desc(desc), args(args), cb(cb) {
    argc = args.empty() ? 0 : std::count(args.begin(), args.end(), ',') + 1;
}

ma_err_t ATServer::addService(ATService& service) {
    for (auto it = m_services.begin(); it != m_services.end(); ++it) {
        if (it->name == service.name) {
            return MA_EEXIST;
        }
    }
    m_services.push_front(service);

    return MA_OK;
}

ATServer::ATServer(Encoder* encoder) : m_encoder(*encoder) {
    m_thread = new Thread("ATServer", ATServer::threadEntryStub, this, MA_SEVER_AT_EXECUTOR_TASK_PRIO, MA_SEVER_AT_EXECUTOR_STACK_SIZE);
    MA_ASSERT(m_thread);
}

ATServer::ATServer(Encoder& encoder) : m_encoder(encoder) {
    m_thread = new Thread("ATServer", ATServer::threadEntryStub, this, MA_SEVER_AT_EXECUTOR_TASK_PRIO, MA_SEVER_AT_EXECUTOR_STACK_SIZE);
    MA_ASSERT(m_thread);
}

void ATServer::threadEntryStub(void* arg) {
    MA_LOGD(MA_TAG, "Starting Thread Entry Stub");
    ATServer* server = static_cast<ATServer*>(arg);
    server->threadEntry();
}

void ATServer::threadEntry() {
    // Startup
    {
        initDefaultSensor(m_encoder);
        initDefaultModel(m_encoder);
        initDefaultAlgorithm(m_encoder);
        initDefaultTrigger(m_encoder);
    }

    char* buf = new char[MA_SEVER_AT_CMD_MAX_LENGTH + 1];
    std::memset(buf, 0, MA_SEVER_AT_CMD_MAX_LENGTH + 1);
    while (true) {
        for (auto& transport : static_resource->device->getTransports()) {
            if (transport && *transport) {
                int len = transport->receiveIf(buf, MA_SEVER_AT_CMD_MAX_LENGTH, 0x0D);
                if (len > 1) {
                    execute(buf, transport);
                    std::memset(buf, 0, MA_SEVER_AT_CMD_MAX_LENGTH + 1);
                }
                len = transport->receiveIf(buf, MA_SEVER_AT_CMD_MAX_LENGTH, 0x0A);
                if (len > 1) {
                    execute(buf, transport);
                    std::memset(buf, 0, MA_SEVER_AT_CMD_MAX_LENGTH + 1);
                }
            }
        }
        Thread::sleep(Tick::fromMicroseconds(20));
    }
    delete[] buf;
}

ma_err_t ATServer::init() {
    ma_err_t err = MA_OK;

    this->addService("ID?", "Get device ID", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([cmd = std::move(args[0]), &transport, &encoder](const std::atomic<bool>&) { get_device_id(cmd, transport, encoder); });
        return MA_OK;
    });

    this->addService("NAME?", "Get device name", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([cmd = std::move(args[0]), &transport, &encoder](const std::atomic<bool>&) { get_device_name(cmd, transport, encoder); });
        return MA_OK;
    });

    this->addService("STAT?", "Get device status", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([cmd = std::move(args[0]), &transport, &encoder](const std::atomic<bool>&) { get_device_status(cmd, transport, encoder); });
        return MA_OK;
    });

    this->addService("VER?", "Get device version", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([cmd = std::move(args[0]), &transport, &encoder](const std::atomic<bool>&) { get_version(cmd, transport, encoder, MA_AT_API_VERSION); });
        return MA_OK;
    });

    this->addService("RST", "Reset device", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([cmd = std::move(args[0]), &transport, &encoder](const std::atomic<bool>&) { static_resource->device->~Device(); });
        return MA_OK;
    });

    this->addService("BREAK", "Stop all running tasks", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        if (static_resource->is_ready.load()) [[likely]] {
            static_resource->executor->submit([cmd = std::move(args[0]), &transport, &encoder](const std::atomic<bool>&) {
                static_resource->current_task_id.fetch_add(1);
                break_task(cmd, transport, encoder);
            });
        }
        return MA_OK;
    });

    this->addService("YIELD", "Yield running tasks for a period time", "TIME_S", [](std::vector<std::string> args, Transport& Transport, Encoder& Encoder) {
        static_resource->executor->submit([cmd = std::move(args[0]), period = std::atoi(args[1].c_str()), &Transport, &Encoder](const std::atomic<bool>& stop_token) mutable {
            while (stop_token.load(std::memory_order_seq_cst) && period > 0) {
                Thread::yield();
                period -= 1;
            }
        });
        return MA_OK;
    });

    this->addService("LED", "Set LED status", "ENABLE/DISABLE", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([cmd = std::move(args[0]), sta = std::atoi(args[1].c_str()), &transport, &encoder](const std::atomic<bool>&) { task_status(cmd, sta, transport, encoder); });

        return MA_OK;
    });

    this->addService("MODELS?", "Get available models", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getAvailableModels(args, transport, encoder); });
        return MA_OK;
    });

    this->addService("MODEL?", "Get current model", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getModelInfo(args, transport, encoder); });
        return MA_OK;
    });

    this->addService("MODEL", "Set current model", "MODEL_ID", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) {
            static_resource->current_task_id += 1;
            configureModel(args, transport, encoder);
        });
        return MA_OK;
    });

    addService("INFO?", "Get stored info", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { readInfo(args, transport, encoder); });
        return MA_OK;
    });

    addService("INFO", "Set info", "VALUE", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { storeInfo(args, transport, encoder); });
        return MA_OK;
    });

    addService("SENSORS?", "Get available sensors", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getAvailableSensors(args, transport, encoder); });
        return MA_OK;
    });

    addService("SENSOR?", "Get current sensor status", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getSensorStatus(args, transport, encoder); });
        return MA_OK;
    });

    addService("SENSOR", "Configure current sensor", "SENSOR_ID,ENABLE,OPT_ID", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) {
            static_resource->current_task_id += 1;
            configureSensor(args, transport, encoder);
        });
        return MA_OK;
    });

    addService("SAMPLE", "Sample sensor data", "N_TIMES", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) {
            static_resource->current_task_id += 1;
            Sample::create(args, transport, encoder, static_resource->current_task_id)->run();
        });
        return MA_OK;
    });

    addService("INVOKE", "Invoke model", "N_TIMES,RESULTS_ONLY", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) {
            static_resource->current_task_id += 1;
            Invoke::create(args, transport, encoder, static_resource->current_task_id)->run();
        });
        return MA_OK;
    });

    addService("ALGO", "Set algorithm type", "ALGO_ID", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { configureAlgorithm(args, transport, encoder); });
        return MA_OK;
    });

    addService("ALGO?", "Get algorithm type", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getAlgorithmInfo(args, transport, encoder); });
        return MA_OK;
    });

    addService("TIOU?", "Get IOU threshold", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getNMSThreshold(args, transport, encoder); });
        return MA_OK;
    });

    addService("TSCORE?", "Get score threshold", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getScoreThreshold(args, transport, encoder); });
        return MA_OK;
    });

    addService("TIOU", "Set IOU threshold", "VALUE", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { setNMSThreshold(args, transport, encoder); });
        return MA_OK;
    });

    addService("TSCORE", "Set score threshold", "VALUE", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { setScoreThreshold(args, transport, encoder); });
        return MA_OK;
    });

    addService("WIFI", "Configure Wi-Fi", "NAME,SECURITY,PASSWORD", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { configureWifi(args, transport, encoder); });
        return MA_OK;
    });

#if !MA_HAS_NATTIVE_WIFI_SUPPORT
    addService("WIFISTA", "Set Wi-Fi status", "STATUS", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { setWifiSta(args, transport, encoder); });
        return MA_OK;
    });

    addService("WIFIIN4", "Set Wi-Fi info", "IP,NETMASK,GATEWAY", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { setWifiIn4Info(args, transport, encoder); });
        return MA_OK;
    });

    addService("WIFIIN6", "Set Wi-Fi info", "IP", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { setWifiIn6Info(args, transport, encoder); });
        return MA_OK;
    });

    addService("WIFIVER", "Set Wi-Fi version", "VERSION", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { setWifiVer(args, transport, encoder); });
        return MA_OK;
    });

    addService("WIFIVER?", "Get Wi-Fi version", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getWifiVer(args, transport, encoder); });
        return MA_OK;
    });
#endif

    addService("WIFI?", "Get Wi-Fi status", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getWifiInfo(args, transport, encoder); });
        return MA_OK;
    });

    addService("MQTTSERVER", "Configure MQTT server", "CLIENT_ID,BROKER,PORT,CLIENT_ID,USERNAME,USE_SSL", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { configureMqttServer(args, transport, encoder); });
        return MA_OK;
    });

#if !MA_HAS_NATTIVE_MQTT_SUPPORT
    addService("MQTTSERVERSTA", "Set MQTT server status", "STATUS", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { setMqttSta(args, transport, encoder); });
        return MA_OK;
    });
#endif

    addService("MQTTSERVERSTA?", "Get MQTT server status", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getMqttSta(args, transport, encoder); });
        return MA_OK;
    });

    addService("MQTTPUBSUB?", "Get MQTT server status", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getMqttPubsub(args, transport, encoder); });
        return MA_OK;
    });

    addService("MQTTSERVER?", "Get MQTT server status", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getMqttConfig(args, transport, encoder); });
        return MA_OK;
    });

    addService("MQTTCA", "Set MQTT CA", "CA", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { configureMqttCA(args, transport, encoder); });
        return MA_OK;
    });

    addService("MQTTCA?", "Get MQTT CA", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) { getMqttCA(args, transport, encoder); });
        return MA_OK;
    });

    addService("TRIGGER", "Set trigger rule", "RULE", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) {
            configureTrigger(args, transport, encoder);
        });
        return MA_OK;
    });

    addService("TRIGGER?", "Get trigger rule", "", [](std::vector<std::string> args, Transport& transport, Encoder& encoder) {
        static_resource->executor->submit([args = std::move(args), &transport, &encoder](const std::atomic<bool>&) {
            getTrigger(args, transport, encoder);
        });
        return MA_OK;
    });

    return MA_OK;
}

#ifdef MA_SERVER_RUN_DEVICE_BACKGROUND_TASK
extern void __ma_device_background_task();
static void __ma_device_task_entry(void*) {
    while (1) {  
        static_resource->executor->submit([&](const std::atomic<bool>&) {
            __ma_device_background_task();
        });
        Thread::sleep(Tick::fromMilliseconds(MA_SERVER_RUN_DEVICE_BACKGROUND_TASK_INTERVAL_MS));
    }
}
#endif

ma_err_t ATServer::start() {
    static_resource->executor->submit([](const std::atomic<bool>&) { static_resource->is_ready.store(true); });

#ifdef MA_SERVER_RUN_DEVICE_BACKGROUND_TASK
    static Thread* device_task = new Thread(
        "DeviceTask",
        __ma_device_task_entry,
        this,
        MA_SERVER_RUN_DEVICE_BACKGROUND_TASK_PRIORITY,
        MA_SERVER_RUN_DEVICE_BACKGROUND_TASK_STACK_SIZE);
    if (device_task) {
        device_task->start();
    }
#endif

    int sta = m_thread->start(this) ? 0 : -1;
    MA_LOGD(MA_TAG, "ATServer started: %d", sta);

    return MA_OK;
}

ma_err_t ATServer::stop() {
    return MA_OK;
}

ma_err_t ATServer::addService(const std::string& name, const std::string& desc, const std::string& args, ATServiceCallback cb) {
    ATService service(name, desc, args, cb);
    return addService(service);
}

ma_err_t ATServer::execute(std::string line, Transport& transport) {
    ma_err_t ret = MA_OK;
    std::string name;
    std::string args;

    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return !std::isprint(c); }), line.end());

    // find first '=' in AT command (<name>=<args>)
    size_t pos = line.find_first_of("=");
    if (pos != std::string::npos) {
        name = line.substr(0, pos);
        args = line.substr(pos + 1, line.size());
    } else {
        name = line;
    }

    std::transform(name.begin(), name.end(), name.begin(), ::toupper);

    // check if name is valid (starts with "AT+")
    if (name.rfind("AT+", 0) != 0) {
        m_encoder.begin(MA_MSG_TYPE_EVT, MA_EINVAL, "AT", "Uknown command: " + name);
        m_encoder.end();
        transport.send(reinterpret_cast<const char*>(m_encoder.data()), m_encoder.size());
        return MA_EINVAL;
    }

    name = name.substr(3, name.size());  // remove "AT+" command prefix

    // find command MA_TAG (everything after last '@'), then remove the MA_TAG to get the AT command
    // name
    size_t cmd_body_pos    = name.rfind('@');
    cmd_body_pos           = cmd_body_pos != std::string::npos ? cmd_body_pos + 1 : 0;
    std::string target_cmd = name.substr(cmd_body_pos, name.size());

    // find command in cmd list
    auto it = std::find_if(m_services.begin(), m_services.end(), [&](const ATService& c) { return c.name.compare(target_cmd) == 0; });

    if (it == m_services.end()) [[unlikely]] {
        m_encoder.begin(MA_MSG_TYPE_EVT, MA_EINVAL, "AT", "Uknown command: " + name);
        m_encoder.end();
        transport.send(reinterpret_cast<const char*>(m_encoder.data()), m_encoder.size());
        return MA_EINVAL;
    }

    // split args by ','
    std::vector<std::string> argv;
    argv.push_back(name);
    std::stack<char> stk;
    size_t index = 0;
    size_t size  = args.size();

    while (index < size && argv.size() < it->argc + 1u) {  // while not reach end of args and not enough args
        char c = args.at(index);
        if (c == '\'' || c == '"') [[unlikely]] {  // if current char is a quote
            stk.push(c);                           // push the quote to stack
            std::string arg;
            while (++index < size && stk.size()) {  // while not reach end of args and stack is not empty
                c = args.at(index);                 // get next char
                if (c == stk.top())                 // if the char matches the top of stack, pop the stack
                    stk.pop();
                else if (c != '\\') [[likely]]  // if the char is not a backslash, append it to
                    arg += c;
                else if (++index < size) [[likely]]  // if the char is a backslash, get next char, append it to arg
                    arg += args.at(index);
            }
            argv.push_back(std::move(arg));
        } else if (c == '-' || std::isdigit(c)) {  // if current char is a digit or a minus sign
            size_t prev = index;
            while (++index < size)  // while not reach end of args
                if (!std::isdigit(args.at(index)))
                    break;                                    // if current char is not a digit, break
            argv.push_back(args.substr(prev, index - prev));  // append the number string to argv
        } else
            ++index;  // if current char is not a quote or a digit, skip it
    }
    argv.shrink_to_fit();

    if (argv.size() < it->argc + 1) [[unlikely]] {
        m_encoder.begin(MA_MSG_TYPE_EVT, MA_EINVAL, "AT", "Command " + name + " got wrong arguments");
        m_encoder.end();
        transport.send(reinterpret_cast<const char*>(m_encoder.data()), m_encoder.size());
        return MA_EINVAL;
    }

    ret = it->cb(std::move(argv), transport, m_encoder);

    return ret;
}

ma_err_t ATServer::execute(std::string line, Transport* transport) {
    return execute(line, *transport);
}

}  // namespace ma