#include "MQTTWorker.h"
#include <QDebug>

MQTTWorker::MQTTWorker(const char *address, const char *clientId, const char *topic, int qos, long timeout, QObject *parent)
    : QObject(parent),
      ADDRESS(address),
      CLIENTID(clientId),
      TOPIC(topic),
      QOS_LEVEL(qos),
      TIMEOUT_MS(timeout),
      running(true)
{
    MQTTClient_create(&client, ADDRESS.toUtf8().constData(), CLIENTID.toUtf8().constData(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        qDebug() << "Failed to connect to MQTT broker, return code" << rc;
        running = false;
    }
}

MQTTWorker::~MQTTWorker()
{
    if (running)
    {
        stop();
    }
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
}

void MQTTWorker::publishMessage(const QString &message)
{
    if (!running)
        return;

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    QByteArray byteArray = message.toUtf8();
    pubmsg.payload = (void *)byteArray.constData();
    pubmsg.payloadlen = byteArray.size();
    pubmsg.qos = QOS_LEVEL;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;

    int rc = MQTTClient_publishMessage(client, TOPIC.toUtf8().constData(), &pubmsg, &token);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        qDebug() << "Failed to publish message, return code" << rc;
        return;
    }
}


void MQTTWorker::stop()
{
    running = false;
    emit finished();
}
