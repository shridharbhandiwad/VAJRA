#include "SimulatorEngine.h"
#include <QHostAddress>
#include <cmath>

// ── Helpers ──────────────────────────────────────────────────────────────────
float SimulatorEngine::rnd(float lo, float hi)
{
    return lo + QRandomGenerator::global()->generateDouble() * (hi - lo);
}
float SimulatorEngine::noise(float sigma)
{
    // Box-Muller approximation
    double u1 = qMax(1e-6, QRandomGenerator::global()->generateDouble());
    double u2 = QRandomGenerator::global()->generateDouble();
    return static_cast<float>(sigma * std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2));
}

// ── Construction ─────────────────────────────────────────────────────────────
SimulatorEngine::SimulatorEngine(RadarModel *model, quint16 targetPort, QObject *parent)
    : QObject(parent), m_model(model), m_port(targetPort)
{
    int Q   = model->quadrantCount();
    int EPQ = model->elementsPerQuadrant();

    m_power.resize(Q);   m_pwrBase.resize(Q);
    m_temp.resize(Q);    m_tmpBase.resize(Q);
    m_current.resize(Q); m_curBase.resize(Q);
    for (int q = 0; q < Q; ++q) {
        m_power[q].resize(EPQ);   m_pwrBase[q].resize(EPQ);
        m_temp[q].resize(EPQ);    m_tmpBase[q].resize(EPQ);
        m_current[q].resize(EPQ); m_curBase[q].resize(EPQ);
    }

    m_socket = new QUdpSocket(this);

    m_timer = new QTimer(this);
    m_timer->setInterval(80);   // ~12 Hz
    connect(m_timer, &QTimer::timeout, this, &SimulatorEngine::tick);

    m_scenTimer = new QTimer(this);
    m_scenTimer->setInterval(6000);  // rotate scenario every 6 s
    connect(m_scenTimer, &QTimer::timeout, this, &SimulatorEngine::advanceScenario);

    buildScenario();
}

// ── Scenario builder ─────────────────────────────────────────────────────────
void SimulatorEngine::buildScenario()
{
    int Q   = m_model->quadrantCount();
    int EPQ = m_model->elementsPerQuadrant();

    struct Scenario {
        const char *name;
        // per-element type distribution (fractions, sum ≤ 1; rest = Good)
        float pwrWarn;   // fraction of elements with power warning
        float pwrFault;  // fraction with power critical
        float tmpWarn;
        float tmpFault;
        float curWarn;
        float curFault;
        float dead;
        // which quadrant is most affected (-1 = all)
        int   hotQ;
    };

    static const Scenario scenarios[] = {
        // 0: All Healthy
        {"All Nominal",      0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, -1},
        // 1: Low-power band — power dropping across Q1
        {"Power Degradation (Q1)", 0.30f, 0.10f, 0.00f, 0.00f, 0.05f, 0.00f, 0.01f, 0},
        // 2: Thermal runaway — Q3 heating up
        {"Thermal Runaway (Q3)",   0.00f, 0.00f, 0.25f, 0.15f, 0.00f, 0.05f, 0.01f, 2},
        // 3: Overcurrent event — Q2/Q4
        {"Overcurrent Event (Q2)", 0.05f, 0.02f, 0.05f, 0.00f, 0.30f, 0.12f, 0.02f, 1},
        // 4: Multi-fault — spread across all quadrants
        {"Multi-Fault Storm",      0.12f, 0.08f, 0.10f, 0.06f, 0.08f, 0.04f, 0.03f, -1},
        // 5: Recovery — faults clearing
        {"Recovery Phase",         0.05f, 0.01f, 0.03f, 0.00f, 0.03f, 0.00f, 0.00f, -1},
    };
    m_scenIdx = m_scenIdx % 6;
    const Scenario &sc = scenarios[m_scenIdx];
    m_scenarioName = sc.name;

    for (int q = 0; q < Q; ++q) {
        // If scenario targets a specific hot quadrant, non-hot quadrants look healthy
        float mult = (sc.hotQ >= 0 && q != sc.hotQ) ? 0.1f : 1.0f;

        for (int i = 0; i < EPQ; ++i) {
            float roll = rnd(0.0f, 1.0f);
            float pw = 0, tf = 0, cf = 0;
            pw  = sc.pwrWarn  * mult;
            pw += sc.pwrFault * mult;
            tf  = pw + sc.tmpWarn  * mult;
            tf += sc.tmpFault * mult;
            cf  = tf + sc.curWarn  * mult;
            cf += sc.curFault * mult;

            // Base power (healthy nominal: 45–50 dBm)
            if (roll < sc.dead * mult) {
                // Dead element
                m_pwrBase[q][i] = rnd(20.0f, 30.0f);
                m_tmpBase[q][i] = rnd(55.0f, 70.0f);
                m_curBase[q][i] = rnd(0.0f,  0.3f);
            } else if (roll < sc.pwrFault * mult) {
                m_pwrBase[q][i] = rnd(30.0f, 39.9f);  // Critical power
                m_tmpBase[q][i] = rnd(30.0f, 44.0f);
                m_curBase[q][i] = rnd(1.0f,  1.7f);
            } else if (roll < (sc.pwrWarn + sc.pwrFault) * mult) {
                m_pwrBase[q][i] = rnd(40.0f, 42.9f);  // Warning power
                m_tmpBase[q][i] = rnd(32.0f, 44.0f);
                m_curBase[q][i] = rnd(1.1f,  1.7f);
            } else if (roll < (sc.pwrWarn + sc.pwrFault + sc.tmpFault) * mult) {
                m_pwrBase[q][i] = rnd(43.5f, 49.0f);
                m_tmpBase[q][i] = rnd(51.0f, 65.0f);  // Critical temp
                m_curBase[q][i] = rnd(1.5f,  2.1f);
            } else if (roll < (sc.pwrWarn + sc.pwrFault + sc.tmpWarn + sc.tmpFault) * mult) {
                m_pwrBase[q][i] = rnd(44.0f, 49.0f);
                m_tmpBase[q][i] = rnd(45.0f, 49.9f);  // Warning temp
                m_curBase[q][i] = rnd(1.4f,  1.9f);
            } else if (roll < (sc.pwrWarn + sc.pwrFault + sc.tmpWarn + sc.tmpFault + sc.curFault) * mult) {
                m_pwrBase[q][i] = rnd(43.5f, 48.0f);
                m_tmpBase[q][i] = rnd(38.0f, 49.0f);
                m_curBase[q][i] = rnd(2.05f, 3.0f);   // Critical current
            } else if (roll < cf) {
                m_pwrBase[q][i] = rnd(43.0f, 48.0f);
                m_tmpBase[q][i] = rnd(35.0f, 45.0f);
                m_curBase[q][i] = rnd(1.8f,  1.99f);  // Warning current
            } else {
                // Healthy
                m_pwrBase[q][i] = rnd(43.5f, 49.5f);
                m_tmpBase[q][i] = rnd(28.0f, 43.0f);
                m_curBase[q][i] = rnd(1.1f,  1.75f);
            }

            m_power[q][i]   = m_pwrBase[q][i];
            m_temp[q][i]    = m_tmpBase[q][i];
            m_current[q][i] = m_curBase[q][i];
        }
    }
    emit scenarioChanged();
}

// ── Runtime tick: add measurement noise and send ─────────────────────────────
void SimulatorEngine::tick()
{
    int Q   = m_model->quadrantCount();
    int EPQ = m_model->elementsPerQuadrant();

    for (int q = 0; q < Q; ++q) {
        for (int i = 0; i < EPQ; ++i) {
            // Realistic measurement noise
            m_power[q][i]   = m_pwrBase[q][i]   + noise(0.15f);
            m_temp[q][i]    = m_tmpBase[q][i]    + noise(0.40f);
            m_current[q][i] = m_curBase[q][i]    + noise(0.015f);
            // Clamp to physical limits
            m_power[q][i]   = qBound(15.0f, m_power[q][i],   60.0f);
            m_temp[q][i]    = qBound( 5.0f, m_temp[q][i],    90.0f);
            m_current[q][i] = qBound( 0.0f, m_current[q][i],  5.0f);
        }
    }
    sendSnapshot();
}

// ── Send 0x12 all-quadrants snapshot ─────────────────────────────────────────
void SimulatorEngine::sendSnapshot()
{
    int Q   = m_model->quadrantCount();
    int EPQ = m_model->elementsPerQuadrant();
    int payloadLen = 1 + Q * EPQ * 6;

    QByteArray pkt(payloadLen, 0);
    uchar *p = reinterpret_cast<uchar*>(pkt.data());
    p[0] = 0x12;

    for (int q = 0; q < Q; ++q) {
        for (int i = 0; i < EPQ; ++i) {
            uchar *ep = p + 1 + (q * EPQ + i) * 6;
            quint16 pw = encodePower  (m_power[q][i]);
            quint16 tm = encodeTemp   (m_temp[q][i]);
            quint16 cu = encodeCurrent(m_current[q][i]);
            ep[0] = pw & 0xFF; ep[1] = (pw >> 8) & 0xFF;
            ep[2] = tm & 0xFF; ep[3] = (tm >> 8) & 0xFF;
            ep[4] = cu & 0xFF; ep[5] = (cu >> 8) & 0xFF;
        }
    }
    m_socket->writeDatagram(pkt, QHostAddress::LocalHost, m_port);
}

// ── Start / Stop ─────────────────────────────────────────────────────────────
void SimulatorEngine::start()
{
    buildScenario();
    m_timer->start();
    m_scenTimer->start();
    emit runningChanged();
}
void SimulatorEngine::stop()
{
    m_timer->stop();
    m_scenTimer->stop();
    emit runningChanged();
}

void SimulatorEngine::advanceScenario()
{
    m_scenIdx = (m_scenIdx + 1) % 6;
    buildScenario();
}
