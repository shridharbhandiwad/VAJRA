#ifndef RADARANTENNAWIDGET_H
#define RADARANTENNAWIDGET_H

#include <QWidget>
#include <QQuickWidget>
#include <QVBoxLayout>
#include "AntennaConfig.h"
#include "RadarModel.h"
#include "UdpReceiver.h"
#include "SimulatorEngine.h"
#include "CommandSender.h"

/**
 * RadarAntennaWidget
 *
 * Hosts the radar antenna QML monitor UI inside a Qt Widgets container.
 *
 * Two antenna configurations are supported:
 *   Antenna A – 1024 elements (4 × 256), UDP 5005
 *   Antenna B – 2048 elements (4 × 512), UDP 5006
 *
 * Each antenna has its own RadarModel, UdpReceiver, SimulatorEngine
 * and CommandSender.  The QML view receives these objects via the
 * QQuickWidget root context.
 */
class RadarAntennaWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RadarAntennaWidget(QWidget *parent = nullptr);
    ~RadarAntennaWidget() override = default;

    /** Start/stop the built-in simulator for testing without live hardware. */
    void startSimulator();
    void stopSimulator();

    /** Returns true if the UDP receiver for the currently selected antenna
     *  is bound and listening. */
    bool isListening() const;

private:
    void setupModels();
    void setupQmlEngine();

    // ── Antenna A ─────────────────────────────────────────────────
    AntennaConfig    m_cfgA;
    RadarModel      *m_modelA      = nullptr;
    UdpReceiver     *m_recvA       = nullptr;
    SimulatorEngine *m_simA        = nullptr;
    CommandSender   *m_cmdSenderA  = nullptr;

    // ── Antenna B ─────────────────────────────────────────────────
    AntennaConfig    m_cfgB;
    RadarModel      *m_modelB      = nullptr;
    UdpReceiver     *m_recvB       = nullptr;
    SimulatorEngine *m_simB        = nullptr;
    CommandSender   *m_cmdSenderB  = nullptr;

    QQuickWidget    *m_qmlView     = nullptr;
};

#endif // RADARANTENNAWIDGET_H
