#include "RadarModel.h"
#include <algorithm>
#include <numeric>

RadarModel::RadarModel(const AntennaConfig &cfg, QObject *parent)
    : QObject(parent), m_cfg(cfg)
{
    m_data.resize(cfg.quadrants);
    for (auto &q : m_data)
        q.resize(cfg.elementsPerQuadrant);  // default-constructed → all NO_DATA

    m_histTimer = new QTimer(this);
    m_histTimer->setInterval(2000);
    connect(m_histTimer, &QTimer::timeout, this, &RadarModel::snapshotHistory);
    m_histTimer->start();

    m_lastUpdate = "No data";
}

// ── Aggregate health helper ───────────────────────────────────────────────────
int RadarModel::calcHealth(const QVector<ElementHealth> &elems, int start, int count)
{
    int total = 0, scored = 0;
    for (int i = start; i < start + count && i < elems.size(); ++i) {
        int s = elems[i].healthScore();
        if (s >= 0) { total += s; scored++; }
    }
    return (scored > 0) ? (total / scored) : -1;
}

// ── Global stats rebuild ─────────────────────────────────────────────────────
void RadarModel::rebuildStats()
{
    m_healthy = m_warning = m_critical = m_noData = 0;
    for (auto &quad : m_data) {
        for (auto &e : quad) {
            switch (e.overall()) {
                case ElementHealth::Healthy:  ++m_healthy;  break;
                case ElementHealth::Warning:  ++m_warning;  break;
                case ElementHealth::Critical: ++m_critical; break;
                default:                      ++m_noData;   break;
            }
        }
    }
    m_lastUpdate = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    emit statsChanged();
}

// ── LOD1 ─────────────────────────────────────────────────────────────────────
int RadarModel::quadrantHealth(int q) const
{
    if (q < 0 || q >= m_data.size()) return -1;
    return calcHealth(m_data[q], 0, m_data[q].size());
}

QVariantList RadarModel::quadrantStats(int q) const
{
    if (q < 0 || q >= m_data.size()) return {0,0,0,0};
    int h=0, w=0, c=0, n=0;
    for (auto &e : m_data[q]) {
        switch (e.overall()) {
            case ElementHealth::Healthy:  ++h; break;
            case ElementHealth::Warning:  ++w; break;
            case ElementHealth::Critical: ++c; break;
            default:                      ++n; break;
        }
    }
    return {h, w, c, n};
}

QVariantMap RadarModel::quadrantMetrics(int q) const
{
    QVariantMap m;
    if (q < 0 || q >= m_data.size()) return m;
    double sp=0, st=0, sc=0;
    float  minP=999, maxT=-999, maxC=-999;
    int    np=0, nt=0, nc=0;
    for (auto &e : m_data[q]) {
        if (e.power       != ElementHealth::NO_DATA_VAL) { sp+=e.power;       minP=qMin(minP,e.power);       ++np; }
        if (e.temperature != ElementHealth::NO_DATA_VAL) { st+=e.temperature; maxT=qMax(maxT,e.temperature); ++nt; }
        if (e.current     != ElementHealth::NO_DATA_VAL) { sc+=e.current;     maxC=qMax(maxC,e.current);     ++nc; }
    }
    m["avgPower"]   = np ? sp/np : -999.0;
    m["avgTemp"]    = nt ? st/nt : -999.0;
    m["avgCurrent"] = nc ? sc/nc : -999.0;
    m["minPower"]   = np ? minP  : -999.0;
    m["maxTemp"]    = nt ? maxT  : -999.0;
    m["maxCurrent"] = nc ? maxC  : -999.0;
    return m;
}

// ── LOD2 ─────────────────────────────────────────────────────────────────────
int RadarModel::clusterHealth(int q, int c) const
{
    if (q < 0 || q >= m_data.size()) return -1;
    int epc = m_cfg.elementsPerCluster;
    return calcHealth(m_data[q], c * epc, epc);
}

QVariantList RadarModel::clusterStats(int q, int c) const
{
    if (q < 0 || q >= m_data.size()) return {0,0,0,0};
    int epc   = m_cfg.elementsPerCluster;
    int start = c * epc;
    int h=0, w=0, cc=0, n=0;
    for (int i = start; i < start + epc && i < m_data[q].size(); ++i) {
        switch (m_data[q][i].overall()) {
            case ElementHealth::Healthy:  ++h;  break;
            case ElementHealth::Warning:  ++w;  break;
            case ElementHealth::Critical: ++cc; break;
            default:                      ++n;  break;
        }
    }
    return {h, w, cc, n};
}

QVariantMap RadarModel::clusterMetrics(int q, int c) const
{
    QVariantMap m;
    if (q < 0 || q >= m_data.size()) return m;
    int epc   = m_cfg.elementsPerCluster;
    int start = c * epc;
    double sp=0, st=0, sc=0;
    int np=0, nt=0, nc=0;
    for (int i = start; i < start+epc && i < m_data[q].size(); ++i) {
        auto &e = m_data[q][i];
        if (e.power       != ElementHealth::NO_DATA_VAL) { sp+=e.power;       ++np; }
        if (e.temperature != ElementHealth::NO_DATA_VAL) { st+=e.temperature; ++nt; }
        if (e.current     != ElementHealth::NO_DATA_VAL) { sc+=e.current;     ++nc; }
    }
    m["avgPower"]   = np ? sp/np : -999.0;
    m["avgTemp"]    = nt ? st/nt : -999.0;
    m["avgCurrent"] = nc ? sc/nc : -999.0;
    return m;
}

// ── LOD3 / Full ───────────────────────────────────────────────────────────────
QVariantList RadarModel::quadrantData(int q) const
{
    if (q < 0 || q >= m_data.size()) return {};
    QVariantList out;
    out.reserve(m_data[q].size());
    for (auto &e : m_data[q])
        out.append(static_cast<int>(e.overall()));
    return out;
}

QVariantList RadarModel::clusterData(int q, int c) const
{
    if (q < 0 || q >= m_data.size()) return {};
    int epc = m_cfg.elementsPerCluster;
    int start = c * epc;
    QVariantList out;
    for (int i = start; i < start+epc && i < m_data[q].size(); ++i)
        out.append(static_cast<int>(m_data[q][i].overall()));
    return out;
}

QVariantMap RadarModel::elementDetail(int q, int localElem) const
{
    QVariantMap m;
    if (q < 0 || q >= m_data.size()) return m;
    if (localElem < 0 || localElem >= m_data[q].size()) return m;
    auto &e = m_data[q][localElem];
    m["power"]       = e.power;
    m["temperature"] = e.temperature;
    m["current"]     = e.current;
    m["status"]      = static_cast<int>(e.overall());
    m["powerStatus"] = static_cast<int>(ElementHealth::powerStatus(e.power));
    m["tempStatus"]  = static_cast<int>(ElementHealth::tempStatus(e.temperature));
    m["currStatus"]  = static_cast<int>(ElementHealth::currentStatus(e.current));
    return m;
}

// ── Analytics ────────────────────────────────────────────────────────────────
QVariantList RadarModel::worstElements(int n) const
{
    struct Candidate { int gid, q, local; int status; float power, temp, current; };
    QVector<Candidate> cands;

    for (int q = 0; q < m_data.size(); ++q) {
        for (int i = 0; i < m_data[q].size(); ++i) {
            auto &e = m_data[q][i];
            int s = e.overall();
            if (s >= ElementHealth::Warning)
                cands.append({q * m_cfg.elementsPerQuadrant + i, q, i,
                              s, e.power, e.temperature, e.current});
        }
    }
    std::sort(cands.begin(), cands.end(), [](const Candidate &a, const Candidate &b){
        if (a.status != b.status) return a.status > b.status;
        // Among same status, sort by power (lowest first)
        float ap = a.power == ElementHealth::NO_DATA_VAL ? 999.0f : a.power;
        float bp = b.power == ElementHealth::NO_DATA_VAL ? 999.0f : b.power;
        return ap < bp;
    });

    QVariantList out;
    for (int i = 0; i < qMin(n, cands.size()); ++i) {
        QVariantMap m;
        m["gid"]     = cands[i].gid;
        m["quad"]    = cands[i].q;
        m["local"]   = cands[i].local;
        m["status"]  = cands[i].status;
        m["power"]   = cands[i].power;
        m["temp"]    = cands[i].temp;
        m["current"] = cands[i].current;
        out.append(m);
    }
    return out;
}

// ── Ingestion ────────────────────────────────────────────────────────────────
void RadarModel::setElement(int q, int le, float power, float temp, float current)
{
    if (q < 0 || q >= m_data.size()) return;
    if (le < 0 || le >= m_data[q].size()) return;
    m_data[q][le].power       = power;
    m_data[q][le].temperature = temp;
    m_data[q][le].current     = current;
    rebuildStats();
    emit quadrantUpdated(q);
}

void RadarModel::setQuadrant(int q,
                             const QVector<float> &powers,
                             const QVector<float> &temps,
                             const QVector<float> &currents)
{
    if (q < 0 || q >= m_data.size()) return;
    int n = m_data[q].size();
    for (int i = 0; i < n; ++i) {
        m_data[q][i].power       = (i < powers.size())   ? powers[i]   : ElementHealth::NO_DATA_VAL;
        m_data[q][i].temperature = (i < temps.size())    ? temps[i]    : ElementHealth::NO_DATA_VAL;
        m_data[q][i].current     = (i < currents.size()) ? currents[i] : ElementHealth::NO_DATA_VAL;
    }
    rebuildStats();
    emit quadrantUpdated(q);
}

void RadarModel::setAllQuadrants(const QVector<QVector<float>> &powers,
                                 const QVector<QVector<float>> &temps,
                                 const QVector<QVector<float>> &currents)
{
    for (int q = 0; q < m_data.size(); ++q) {
        int n = m_data[q].size();
        for (int i = 0; i < n; ++i) {
            m_data[q][i].power       = (q < powers.size()   && i < powers[q].size())   ? powers[q][i]   : ElementHealth::NO_DATA_VAL;
            m_data[q][i].temperature = (q < temps.size()    && i < temps[q].size())    ? temps[q][i]    : ElementHealth::NO_DATA_VAL;
            m_data[q][i].current     = (q < currents.size() && i < currents[q].size()) ? currents[q][i] : ElementHealth::NO_DATA_VAL;
        }
    }
    rebuildStats();
    for (int q = 0; q < m_data.size(); ++q)
        emit quadrantUpdated(q);
}

// ── History snapshot ─────────────────────────────────────────────────────────
void RadarModel::snapshotHistory()
{
    int total = m_healthy + m_warning + m_critical + m_noData;
    int pct   = (total > 0 && (total - m_noData) > 0)
                ? (m_healthy * 100) / (total - m_noData)
                : -1;
    m_history.append(pct);
    if (m_history.size() > 60)
        m_history.removeFirst();
    emit historyChanged();
}
