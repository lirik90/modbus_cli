#ifndef MODBUS_CLI_WORKER_H
#define MODBUS_CLI_WORKER_H

#include <QObject>
#include <QCommandLineParser>


#include "client.h"

namespace Modbus_Cli {

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);

    bool process(const QStringList& args);
signals:
private slots:
    void on_connected();
    void on_request_finished();
private:
    void doit();

    bool is_set(int key);
    QString option(int key);

    QModbusDataUnit::RegisterType get_type(QString text);
    QVector<quint16> get_values(const QString& text);

    int _adr, _start, _count, _repeat, _timeout;
    QModbusDataUnit::RegisterType _type;

    QCommandLineParser _parser;
    QList<QCommandLineOption> _opt;

    std::unique_ptr<Client> _client;
};

} // namespace Modbus_Cli

#endif // MODBUS_CLI_WORKER_H
