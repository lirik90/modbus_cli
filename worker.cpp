
#include <QDebug>
#include <QLoggingCategory>
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
    OT_DEBUG,
    OT_REPEAT,
    OT_TIMEOUT,
	OT_NUMBER_OF_RETRIES,
    OT_RAW,
    OT_FUNC,
    OT_FUNC_HEX
};

Worker::Worker(QObject *parent) :
    QObject(parent),
    _opt({
        { { "a", "address" }, QCoreApplication::translate("main", "Device address. Default: 1"), "adr", "1"},
        { { "t", "type" }, QCoreApplication::translate("main", "Register type. Default: 4 holding_register"), "type", "4"},
        { { "r", "read" }, QCoreApplication::translate("main", "Read from device.")},
        { { "w", "write" }, QCoreApplication::translate("main", "Write to device. Values comma separated. Example: 1,2,3"), "values"},
        { { "rw", "readwrite" }, QCoreApplication::translate("main", "Write and read device."), "values"},
        { { "s", "start" }, QCoreApplication::translate("main", "Start value address. Default: 0"), "start", "0"},
        { { "c", "count" }, QCoreApplication::translate("main", "Values read count. Default: 1"), "count", "1"},
        { { "d", "debug" }, QCoreApplication::translate("main", "Verbose mode. Default: 0"), "debug", "0"},
        { "repeat", QCoreApplication::translate("main", "Repeat command. -1 is infinity. Default: 0"), "repeat", "0"},
        { "timeout", QCoreApplication::translate("main", "Modbus response timeout in milliseconds. Default: 1000"), "timeout", "1000"},
		{ "retries", QCoreApplication::translate("main", "number of retries. Default: 5"), "retries", "5"},
        { "raw", QCoreApplication::translate("main", "Write raw data (hex)"), "raw"},
        { "func", QCoreApplication::translate("main", "Function code for raw request"), "func"},
        { "func_hex", QCoreApplication::translate("main", "Function code for raw request (hex)"), "func_hex"}
    })
{
}

bool Worker::process(const QStringList &args)
{
    _parser.setApplicationDescription("Modbus Cli");
    _parser.addHelpOption();
    _parser.addOptions(_opt);

    _parser.addPositionalArgument("conn", "Connection string. Example: rtu:///dev/ttyS0?baudRate=9600 mtcp://example.com:502");

    _parser.process(args);

    if (_parser.positionalArguments().empty())
    {
		qCritical() << _parser.helpText().constData();
        return false;
    }

    _adr = option(OT_ADDRESS).toInt();
    _start = option(OT_START).toInt();
    _count = option(OT_COUNT).toInt();
    _debug = option(OT_DEBUG).toInt();
    _repeat = option(OT_REPEAT).toInt();
    int timeout = option(OT_TIMEOUT).toInt();
	int number_of_retries = option(OT_NUMBER_OF_RETRIES).toInt();

    _type = get_type(option(OT_REGISTER_TYPE));
    if (_type <= QModbusDataUnit::Invalid || _type > QModbusDataUnit::HoldingRegisters)
    {
        qCritical() << "Unknown register type: " << _type;
        return false;
    }

    if (_debug) // Or use: export QT_LOGGING_RULES="qt.modbus* = true"
        QLoggingCategory::setFilterRules(QStringLiteral("qt.modbus* = true"));

	_client.reset(new Modbus_Cli::Client{_parser.positionalArguments().front(), timeout, number_of_retries});
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
		qCritical() << _parser.helpText().constData();
        qApp->exit(1);
    }
}

bool Worker::is_set(int key)
{
    return _parser.isSet(_opt.at(key));
}

QString Worker::option(int key)
{
    return _parser.value(_opt.at(key));
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
