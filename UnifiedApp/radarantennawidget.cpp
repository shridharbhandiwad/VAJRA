#include "radarantennawidget.h"
#include <QQmlContext>
#include <QVBoxLayout>
#include <QUrl>

RadarAntennaWidget::RadarAntennaWidget(QWidget *parent)
    : QWidget(parent)
{
    setupModels();
    setupQmlEngine();
}

void RadarAntennaWidget::setupModels()
{
    // ── Antenna A: 1024 elements (4 × 256) ──────────────────────
    m_cfgA.name                = "Antenna A";
    m_cfgA.quadrants           = 4;
    m_cfgA.elementsPerQuadrant = 256;
    m_cfgA.elementsPerCluster  = 16;
    m_cfgA.udpPort             = 5005;

    // ── Antenna B: 2048 elements (4 × 512) ──────────────────────
    m_cfgB.name                = "Antenna B";
    m_cfgB.quadrants           = 4;
    m_cfgB.elementsPerQuadrant = 512;
    m_cfgB.elementsPerCluster  = 16;
    m_cfgB.udpPort             = 5006;

    m_modelA     = new RadarModel(m_cfgA, this);
    m_modelB     = new RadarModel(m_cfgB, this);

    m_recvA      = new UdpReceiver(m_modelA, this);
    m_recvB      = new UdpReceiver(m_modelB, this);

    m_simA       = new SimulatorEngine(m_modelA, m_cfgA.udpPort, this);
    m_simB       = new SimulatorEngine(m_modelB, m_cfgB.udpPort, this);

    m_cmdSenderA = new CommandSender(m_cfgA, this);
    m_cmdSenderB = new CommandSender(m_cfgB, this);

    m_recvA->startListening(m_cfgA.udpPort);
    m_recvB->startListening(m_cfgB.udpPort);
}

void RadarAntennaWidget::setupQmlEngine()
{
    m_qmlView = new QQuickWidget(this);
    m_qmlView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // Expose C++ objects to QML
    QQmlContext *ctx = m_qmlView->rootContext();
    ctx->setContextProperty("antenna0",     m_modelA);
    ctx->setContextProperty("antenna1",     m_modelB);
    ctx->setContextProperty("udpReceiver0", m_recvA);
    ctx->setContextProperty("udpReceiver1", m_recvB);
    ctx->setContextProperty("simulator0",   m_simA);
    ctx->setContextProperty("simulator1",   m_simB);
    ctx->setContextProperty("cmdSender0",   m_cmdSenderA);
    ctx->setContextProperty("cmdSender1",   m_cmdSenderB);
    ctx->setContextProperty("antennaCount", 2);

    m_qmlView->setSource(QUrl("qrc:/radar_qml/RadarAntennaView.qml"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_qmlView);
    setLayout(layout);
}

void RadarAntennaWidget::startSimulator()
{
    if (m_simA) m_simA->start();
    if (m_simB) m_simB->start();
}

void RadarAntennaWidget::stopSimulator()
{
    if (m_simA) m_simA->stop();
    if (m_simB) m_simB->stop();
}

bool RadarAntennaWidget::isListening() const
{
    return m_recvA && m_recvA->listening();
}
