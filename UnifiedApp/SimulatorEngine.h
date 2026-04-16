#pragma once
#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QVector>
#include <QRandomGenerator>
#include "RadarModel.h"

/*
 * SimulatorEngine  — generates realistic antenna element data
 *
 * Nominal element values (healthy):
 *   Power  : 45–50 dBm   (threshold 43 dBm)
 *   Temp   : 30–42 °C    (threshold 50 °C)
 *   Current: 1.2–1.7 A   (threshold 2.0 A)
 *
 * Six rotating scenarios demonstrate all fault modes.
 */
class SimulatorEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool    running  READ running  NOTIFY runningChanged)
    Q_PROPERTY(QString scenario READ scenario NOTIFY scenarioChanged)

public:
    explicit SimulatorEngine(RadarModel *model, quint16 targetPort, QObject *parent = nullptr);

    bool    running()  const { return m_timer && m_timer->isActive(); }
    QString scenario() const { return m_scenarioName; }

public slots:
    void start();
    void stop();

signals:
    void runningChanged();
    void scenarioChanged();

private slots:
    void tick();
    void advanceScenario();

private:
    // Encoding helpers matching UdpReceiver decode
    static quint16 encodePower  (float dBm) { return quint16(qMax(0.0f, (dBm + 10.0f) * 100.0f)); }
    static quint16 encodeTemp   (float C)   { return quint16(qMax(0.0f, C * 10.0f)); }
    static quint16 encodeCurrent(float A)   { return quint16(qMax(0.0f, A * 1000.0f)); }

    float  rnd(float lo, float hi);     // uniform random float
    float  noise(float sigma);          // gaussian-like noise

    void buildScenario();               // populate m_power/m_temp/m_current
    void sendSnapshot();

    // Element type per simulation
    enum ElemType { Good, PowerWarn, PowerFault, ThermalWarn, ThermalFault,
                    CurrentWarn, CurrentFault, Dead };

    RadarModel  *m_model      = nullptr;
    quint16      m_port;
    QTimer      *m_timer      = nullptr;
    QTimer      *m_scenTimer  = nullptr;
    QUdpSocket  *m_socket     = nullptr;

    int     m_scenIdx   = 0;
    QString m_scenarioName;

    // Simulated live values (updated each tick with noise)
    QVector<QVector<float>> m_power;    // [quad][elem] dBm
    QVector<QVector<float>> m_temp;     // °C
    QVector<QVector<float>> m_current;  // A

    // Base values (set per scenario, noise added each tick)
    QVector<QVector<float>> m_pwrBase;
    QVector<QVector<float>> m_tmpBase;
    QVector<QVector<float>> m_curBase;
};
