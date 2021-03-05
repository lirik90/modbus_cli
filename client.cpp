

#include <QThread>
#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>
#include <QDebug>
#include <QMetaEnum>

#include "client.h"

namespace Modbus_Cli {

Client::Client(const QString &conn_string, int timeout) :
    _config(conn_string, timeout)
{
    if (_config._tcp._address.isEmpty())
        _dev = std::make_shared<QModbusRtuSerialMaster>();
    else
        _dev = std::make_shared<QModbusTcpClient>();

    connect(_dev.get(), &QModbusClient::errorOccurred, this, &Client::error_occurred);
    connect(_dev.get(), &QModbusClient::stateChanged, this, &Client::state_changed);

    connect(&_timer, &QTimer::timeout, this, &Client::timeout);
    _timer.setSingleShot(true);
    _timer.setInterval(10000);
}

bool Client::connect_device()
{
    if (_dev->state() != QModbusClient::UnconnectedState)
    {
        if (_dev->state() == QModbusClient::ConnectedState)
            emit connected();
        return true;
    }

    {
        auto dbg = qInfo().noquote() << "Connecting to modbus device:";
        if (_config._tcp._address.isEmpty())
        {
            dbg << _config._rtu._name
                << "Speed:" << _config._rtu._baud_rate
                << QMetaEnum::fromType<QSerialPort::DataBits>().valueToKey(_config._rtu._data_bits)
                << QMetaEnum::fromType<QSerialPort::Parity>().valueToKey(_config._rtu._parity)
                << QMetaEnum::fromType<QSerialPort::StopBits>().valueToKey(_config._rtu._stop_bits)
                << QMetaEnum::fromType<QSerialPort::FlowControl>().valueToKey(_config._rtu._flow_control);
        }
        else
            dbg.nospace() << _config._tcp._address << ':' << _config._tcp._port;
    }

    Das::Modbus::Config::set(_config, _dev.get());
    _timer.start();
    return _dev->connectDevice();
}

void Client::read(int address, QModbusDataUnit::RegisterType type, int start_address, int count)
{
    QModbusDataUnit data(type, start_address, count);
    QModbusReply* reply = _dev->sendReadRequest(data, address);
    process_reply(reply);
}

void Client::write(int address, QModbusPdu::FunctionCode func, const QByteArray &data)
{
    QModbusRequest request{func, data};
    QModbusReply* reply = _dev->sendRawRequest(request, address);
    process_reply(reply);
}

void Client::write(int address, QModbusDataUnit::RegisterType type, int start_address, const QVector<quint16> &values)
{
    qDebug() << "Write:" << values;
    QModbusDataUnit data(type, start_address, values);
    QModbusReply* reply = _dev->sendWriteRequest(data, address);
    process_reply(reply);
}

void Client::readwrite(int address, QModbusDataUnit::RegisterType type, int start_address, int count, const QVector<quint16> &values)
{
    qDebug() << "Write:" << values;
    QModbusDataUnit read_data(type, start_address, count);
    QModbusDataUnit write_data(type, start_address, values);
    QModbusReply* reply = _dev->sendReadWriteRequest(read_data, write_data, address);
    process_reply(reply);
}

void Client::timeout()
{
    qCritical() << "Connection timeout";
    emit finished();
}

void Client::error_occurred(QModbusDevice::Error e)
{
    qCritical().noquote() << "Occurred:" << e << _dev->errorString();
    if (e == QModbusDevice::ConnectionError)
        _dev->disconnectDevice();
}

void Client::state_changed(QModbusDevice::State state)
{
    if (state == QModbusDevice::ConnectedState)
    {
        _timer.stop();
        emit connected();
    }
}

void Client::reply_finished_slot()
{
    reply_finished(qobject_cast<QModbusReply*>(sender()));
}

void Client::process_reply(QModbusReply *reply)
{
    if (reply)
    {
        if (reply->isFinished())
            reply_finished(reply);
        else
            connect(reply, &QModbusReply::finished, this, &Client::reply_finished_slot);
    }
    else
        qCritical().noquote() << tr("Reply error: ") + _dev->errorString();
}

void Client::reply_finished(QModbusReply *reply)
{
    if (reply->error() != QModbusDevice::NoError)
    {
        qCritical().noquote() << "Reply error:" << reply->error() << reply->errorString()
                              << (reply->error() == QModbusDevice::ProtocolError ?
                                      tr("Mobus exception: 0x%1").arg(reply->rawResult().exceptionCode(), -1, 16) :
                                      tr("code: 0x%1").arg(reply->error(), -1, 16));
    }
    else
    {
        const QModbusDataUnit unit = reply->result();
        for (uint i = 0; i < unit.valueCount(); ++i)
            qInfo() << (unit.startAddress() + i) << "=" << unit.value(i);

        QModbusResponse response = reply->rawResult();
        qDebug() << "Raw response:" << response.data().toHex().toUpper();
    }

    reply->deleteLater();

    emit finished();
}

} // namespace Modbus_Cli
