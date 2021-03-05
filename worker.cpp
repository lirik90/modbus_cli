
#include <QDebug>
#include <QCoreApplication>

#include "worker.h"

namespace Modbus_Cli {

enum Option_Type {
    OT_ADDRESS,
    OT_REGISTER_TYPE,
    OT_READ,
    OT_WRITE,
    OT_READWRITE,
    OT_START,
    OT_COUNT,
    OT_REPEAT,
    OT_TIMEOUT,
    OT_RAW,
    OT_FUNC,
    OT_FUNC_HEX
};

Worker::Worker(QObject *parent) :
    QObject(parent),
    _opt({
        { { "a", "address" }, QCoreApplication::translate("main", "Device address."), "adr", "1"},
        { { "t", "type" }, QCoreApplication::translate("main", "Register type."), "type", "4"},
        { { "r", "read" }, QCoreApplication::translate("main", "Read from device.")},
        { { "w", "write" }, QCoreApplication::translate("main", "Write to device. Values comma separated. Example: 1,2,3"), "values"},
        { { "rw", "readwrite" }, QCoreApplication::translate("main", "Write and read device."), "rwvalues"},
        { { "s", "start" }, QCoreApplication::translate("main", "Start value address."), "start", "0"},
        { { "c", "count" }, QCoreApplication::translate("main", "Values read count."), "count", "1"},
        { "repeat", QCoreApplication::translate("main", "Repeat command. -1 is infinity"), "repeat", "0"},
        { "timeout", QCoreApplication::translate("main", "Modbus response timeout in milliseconds"), "timeout", "1000"},
        { "raw", QCoreApplication::translate("main", "Write raw data (hex)"), "raw"},
        { "func", QCoreApplication::translate("main", "Function code for raw request"), "func"},
        { "func_hex", QCoreApplication::translate("main", "Function code for raw request (hex)"), "func_hex"}
    })
{
}

bool Worker::process(const QStringList &args)
{
    parser.setApplicationDescription("Modbus Cli");
    parser.addHelpOption();
    parser.addOptions(_opt);

    parser.addPositionalArgument("conn", "Connection string. Example: rtu:///dev/ttyS0?baudRate=9600 mtcp://example.com:502");

    parser.process(args);

    if (parser.positionalArguments().empty())
    {
        qCritical() << parser.helpText();
        return false;
    }

    _adr = option(OT_ADDRESS).toInt();
    _start = option(OT_START).toInt();
    _count = option(OT_COUNT).toInt();
    _repeat = option(OT_REPEAT).toInt();
    int timeout = option(OT_TIMEOUT).toInt();

    _type = get_type(option(OT_REGISTER_TYPE));
    if (_type <= QModbusDataUnit::Invalid || _type > QModbusDataUnit::HoldingRegisters)
    {
        qCritical() << "Unknown register type: " << _type;
        return false;
    }

    _client.reset(new Modbus_Cli::Client{parser.positionalArguments().front(), timeout});
    QObject::connect(_client.get(), &Modbus_Cli::Client::connected, this, &Worker::on_connected);
    QObject::connect(_client.get(), &Modbus_Cli::Client::finished, this, &Worker::on_request_finished);

    return _client->connect_device();
}

void Worker::on_connected()
{
    doit();
}

void Worker::on_request_finished()
{
    if (_repeat == -1 || _repeat-- > 0)
    {
        if (!_client->connect_device())
            qApp->quit();
    }
    else
        qApp->quit();
}

void Worker::doit()
{
    if (is_set(OT_READ))
        _client->read(_adr, _type, _start, _count);
    else if (is_set(OT_WRITE))
        _client->write(_adr, _type, _start, get_values(option(OT_WRITE)));
    else if (is_set(OT_READWRITE))
        _client->readwrite(_adr, _type, _start, _count, get_values(option(OT_WRITE)));
    else if (is_set(OT_RAW) && (is_set(OT_FUNC) || is_set(OT_FUNC_HEX)))
    {
        QString func_str = option(is_set(OT_FUNC) ? OT_FUNC : OT_FUNC_HEX);
        int func_d = func_str.toInt(nullptr, is_set(OT_FUNC) ? 10 : 16);
        QModbusPdu::FunctionCode func_code = static_cast<QModbusPdu::FunctionCode>(func_d);

        QByteArray data = QByteArray::fromHex(option(OT_RAW).toLocal8Bit());
        _client->write(_adr, func_code, data);
    }
    else
    {
        qCritical() << parser.helpText();
        qApp->exit(1);
    }
}

bool Worker::is_set(int key)
{
    return parser.isSet(_opt.at(key));
}

QString Worker::option(int key)
{
    return parser.value(_opt.at(key));
}

QModbusDataUnit::RegisterType Worker::get_type(QString text)
{
    text = text.toLower();
    if (text == "discrete") return QModbusDataUnit::DiscreteInputs;
    else if (text == "coils") return QModbusDataUnit::Coils;
    else if (text == "input_register") return QModbusDataUnit::InputRegisters;
    else if (text == "holding_register") return QModbusDataUnit::HoldingRegisters;

    return static_cast<QModbusDataUnit::RegisterType>(text.toInt());
}

QVector<quint16> Worker::get_values(const QString &text)
{
    QVector<quint16> values;
    QStringList values_strs = text.split(',');
    for (const QString& value: values_strs)
        values.push_back(value.toUInt());
    return values;
}

} // namespace Modbus_Cli
