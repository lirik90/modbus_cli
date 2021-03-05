//#include <iostream>

#include <QCoreApplication>

#include "worker.h"

void logger(QtMsgType type, const QMessageLogContext &ctx, const QString &text)
{
    qt_message_output(type, ctx, text);
//    std::cout << qFormatLogMessage(type, ctx, text).toStdString() << std::endl;
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(logger);
    QCoreApplication a(argc, argv);

    Modbus_Cli::Worker w;
    w.process(a.arguments());
    return a.exec();
}
