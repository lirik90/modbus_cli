#ifndef MODBUS_CLI_CLIENT_H
#define MODBUS_CLI_CLIENT_H

#include <memory>

#include <QModbusClient>
#include <QTimer>

#include "config.h"

namespace Modbus_Cli {

class Client : public QObject
{
    Q_OBJECT
public:
	Client(const QString& conn_string, int timeout, int number_of_retries);
    bool connect_device();

    void read(int address, QModbusDataUnit::RegisterType type, int start_address, int count);

    void write(int address, QModbusPdu::FunctionCode func, const QByteArray& data);
    void write(int address, QModbusDataUnit::RegisterType type, int start_address, const QVector<quint16>& values);

    void readwrite(int address, QModbusDataUnit::RegisterType type, int start_address, int count, const QVector<quint16>& values);
signals:
    void connected();
    void finished();
private slots:
    void timeout();
    void error_occurred(QModbusDevice::Error e);
    void state_changed(QModbusDevice::State state);
    void reply_finished_slot();
private:
    void process_reply(QModbusReply* reply);
    void reply_finished(QModbusReply* reply);

    Das::Modbus::Config _config;
    std::shared_ptr<QModbusClient> _dev;

    QTimer _timer;
};

} // namespace Modbus_Cli

#endif // MODBUS_CLI_CLIENT_H
