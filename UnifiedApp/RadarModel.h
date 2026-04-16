#pragma once
#include <QObject>
#include <QVector>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>
#include <QDateTime>
#include "AntennaConfig.h"

// ─────────────────────────────────────────────────────────────────────────────
// ElementHealth — three physical parameters per antenna element
// ─────────────────────────────────────────────────────────────────────────────
struct ElementHealth {
    // Sentinel value meaning "no data received yet"
    static constexpr float NO_DATA = -999.0f;

    float power       = NO_DATA;   // dBm  — forward transmit power
    float temperature = NO_DATA;   // °C   — element temperature
    float current     = NO_DATA;   // A    — supply current draw

    // ── Thresholds ────────────────────────────────────────────────
    // Power  : HEALTHY ≥ 43 dBm | WARNING 40–43 | CRITICAL < 40
    // Temp   : HEALTHY ≤ 45 °C  | WARNING 45–50 | CRITICAL > 50
    // Current: HEALTHY ≤ 1.8 A  | WARNING 1.8–2 | CRITICAL > 2
    enum Status { NoData = 0, Healthy = 1, Warning = 2, Critical = 3 };

    static Status powerStatus(float p) {
        if (p == NO_DATA) return NoData;
        if (p >= 43.0f)   return Healthy;
        if (p >= 40.0f)   return Warning;
        return Critical;
    }
    static Status tempStatus(float t) {
        if (t == NO_DATA) return NoData;
        if (t <= 45.0f)   return Healthy;
        if (t <= 50.0f)   return Warning;
        return Critical;
    }
    static Status currentStatus(float c) {
        if (c == NO_DATA) return NoData;
        if (c <= 1.8f)    return Healthy;
        if (c <= 2.0f)    return Warning;
        return Critical;
    }

    // Overall status = worst of the three
    Status overall() const {
        auto ps = powerStatus(power);
        auto ts = tempStatus(temperature);
        auto cs = currentStatus(current);
        return static_cast<Status>(qMax(int(ps), qMax(int(ts), int(cs))));
    }

    bool hasData() const {
        return power != NO_DATA || temperature != NO_DATA || current != NO_DATA;
    }

    // Health score 0-100 used for aggregate %
    int healthScore() const {
        switch (overall()) {
            case Healthy:  return 100;
            case Warning:  return 50;
            case Critical: return 0;
            default:       return -1; // excluded from average
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// RadarModel
// ─────────────────────────────────────────────────────────────────────────────
class RadarModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString  antennaName           READ antennaName           CONSTANT)
    Q_PROPERTY(int      quadrantCount         READ quadrantCount         CONSTANT)
    Q_PROPERTY(int      elementsPerQuadrant   READ elementsPerQuadrant   CONSTANT)
    Q_PROPERTY(int      elementsPerCluster    READ elementsPerCluster    CONSTANT)
    Q_PROPERTY(int      clustersPerQuadrant   READ clustersPerQuadrant   CONSTANT)
    Q_PROPERTY(int      totalElements         READ totalElements         CONSTANT)
    Q_PROPERTY(int      udpPort               READ udpPort               CONSTANT)

    Q_PROPERTY(int      healthyCount  READ healthyCount  NOTIFY statsChanged)
    Q_PROPERTY(int      warningCount  READ warningCount  NOTIFY statsChanged)
    Q_PROPERTY(int      criticalCount READ criticalCount NOTIFY statsChanged)
    Q_PROPERTY(int      noDataCount   READ noDataCount   NOTIFY statsChanged)
    Q_PROPERTY(QString  lastUpdateTime READ lastUpdateTime NOTIFY statsChanged)
    Q_PROPERTY(QVariantList healthHistory READ healthHistory NOTIFY historyChanged)

public:
    explicit RadarModel(const AntennaConfig &cfg, QObject *parent = nullptr);

    // Config accessors
    QString antennaName()         const { return m_cfg.name; }
    int     quadrantCount()       const { return m_cfg.quadrants; }
    int     elementsPerQuadrant() const { return m_cfg.elementsPerQuadrant; }
    int     elementsPerCluster()  const { return m_cfg.elementsPerCluster; }
    int     clustersPerQuadrant() const { return m_cfg.clustersPerQuadrant(); }
    int     totalElements()       const { return m_cfg.totalElements(); }
    int     udpPort()             const { return m_cfg.udpPort; }

    // Global counts
    int     healthyCount()  const { return m_healthy; }
    int     warningCount()  const { return m_warning; }
    int     criticalCount() const { return m_critical; }
    int     noDataCount()   const { return m_noData; }
    QString lastUpdateTime() const { return m_lastUpdate; }
    QVariantList healthHistory() const { return m_history; }

    // ── LOD1: per-quadrant ─────────────────────────────────────────
    // Returns 0-100 health%  (-1 if all no-data)
    Q_INVOKABLE int          quadrantHealth(int q) const;
    // Returns [healthy, warning, critical, nodata] counts
    Q_INVOKABLE QVariantList quadrantStats(int q) const;
    // Returns {avgPower, avgTemp, avgCurrent, minPower, maxTemp, maxCurrent}
    Q_INVOKABLE QVariantMap  quadrantMetrics(int q) const;

    // ── LOD2: per-cluster ──────────────────────────────────────────
    Q_INVOKABLE int          clusterHealth(int q, int c) const;
    Q_INVOKABLE QVariantList clusterStats(int q, int c) const;
    Q_INVOKABLE QVariantMap  clusterMetrics(int q, int c) const;

    // ── LOD3 / Full: element arrays ───────────────────────────────
    // Returns status int (0=NoData,1=Healthy,2=Warning,3=Critical) per element
    Q_INVOKABLE QVariantList quadrantData(int q) const;
    Q_INVOKABLE QVariantList clusterData(int q, int c) const;

    // ── Per-element detail ─────────────────────────────────────────
    Q_INVOKABLE QVariantMap  elementDetail(int q, int localElem) const;

    // ── Analytics ─────────────────────────────────────────────────
    Q_INVOKABLE QVariantList worstElements(int n = 8) const;

    // ── Data ingestion (called by UdpReceiver) ─────────────────────
    // Update a single element
    void setElement(int quad, int localElem, float power, float temp, float current);
    // Update entire quadrant at once
    void setQuadrant(int quad,
                     const QVector<float> &powers,
                     const QVector<float> &temps,
                     const QVector<float> &currents);
    // Update all quadrants at once
    void setAllQuadrants(const QVector<QVector<float>> &powers,
                         const QVector<QVector<float>> &temps,
                         const QVector<QVector<float>> &currents);

signals:
    void statsChanged();
    void historyChanged();
    void quadrantUpdated(int q);

private slots:
    void snapshotHistory();

private:
    void rebuildStats();
    static int calcHealth(const QVector<ElementHealth> &elems, int start, int count);

    AntennaConfig               m_cfg;
    QVector<QVector<ElementHealth>> m_data; // [quad][elem]

    int     m_healthy  = 0;
    int     m_warning  = 0;
    int     m_critical = 0;
    int     m_noData   = 0;
    QString m_lastUpdate;

    QVariantList m_history; // ring buffer of health% snapshots (up to 60)
    QTimer      *m_histTimer = nullptr;
};
