#ifndef TRMGRIDVIEW_H
#define TRMGRIDVIEW_H

#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>

/**
 * TrmCell – A single TRM element cell in the grid.
 *
 * Displays:
 *   - TRM number (ID)
 *   - Background colour based on health/status
 *   - Tooltip with full TRM data
 *
 * Matches the visual style of the Radar Antenna Health Monitor screenshot:
 * small numbered coloured cells arranged in a dense grid.
 */
class TrmCell : public QWidget
{
    Q_OBJECT
public:
    struct TrmData {
        int id = 0;
        double health = 100.0;
        QString color = "#00FF00";
        QString status = "HEALTHY";
        double voltage = 0.0;
        double current = 0.0;
        bool on = true;
        QString trip = "Normal";
        QString bit = "Normal";
        int quadrant = -1;
        int channel = -1;
    };

    explicit TrmCell(int trmId, QWidget* parent = nullptr);

    void setData(const TrmData& data);
    const TrmData& data() const { return m_data; }
    int trmId() const { return m_id; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void clicked(int trmId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    int m_id;
    TrmData m_data;
    bool m_hovered = false;
};

/**
 * TrmLegend – Colour legend bar shown below the TRM grid.
 */
class TrmLegend : public QWidget
{
    Q_OBJECT
public:
    explicit TrmLegend(QWidget* parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
};

/**
 * TrmStatusBar – Horizontal counters row: HEALTHY / WARNING / CRITICAL / UNKNOWN.
 */
class TrmStatusBar : public QWidget
{
    Q_OBJECT
public:
    explicit TrmStatusBar(QWidget* parent = nullptr);

    void update(int healthy, int warning, int critical, int unknown, int total);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_healthy = 0, m_warning = 0, m_critical = 0, m_unknown = 0, m_total = 0;
};

/**
 * TrmGridView – Full antenna subsystem TRM grid panel.
 *
 * Replicates the "RADAR ANTENNA HEALTH MONITOR" layout from the screenshot:
 *   - Title bar with subsystem info
 *   - Status counters
 *   - Dense grid of numbered TRM cells colour-coded by health
 *   - Legend
 *   - Detail panel for selected TRM
 *
 * Usage:
 *   TrmGridView* grid = new TrmGridView(this);
 *   grid->setTrmCount(271, 16);  // 271 TRMs, 16 columns
 *   grid->updateTrmData(jsonArray);  // from trmDataReceived signal
 */
class TrmGridView : public QWidget
{
    Q_OBJECT
public:
    explicit TrmGridView(QWidget* parent = nullptr);
    ~TrmGridView() override;

    /** Configure grid dimensions. Must be called before first updateTrmData(). */
    void setTrmCount(int count, int columns = 16);

    /** Update all TRM cells from a JSON array (from MessageServer::trmDataReceived). */
    void updateTrmData(const QJsonArray& trmArray);

    /** Update a single TRM cell directly. */
    void updateTrm(int id, const TrmCell::TrmData& data);

    /** Reset all cells to unknown/grey state. */
    void reset();

    int trmCount() const { return m_trmCount; }
    int columns() const  { return m_columns; }

signals:
    void trmSelected(int trmId, const TrmCell::TrmData& data);

private slots:
    void onCellClicked(int trmId);

private:
    void buildGrid();
    void updateStatusCounts();

    int m_trmCount = 0;
    int m_columns = 16;

    QVector<TrmCell*> m_cells;   // index = TRM id
    QGridLayout* m_gridLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    TrmStatusBar* m_statusBar = nullptr;
    TrmLegend* m_legend = nullptr;
    QLabel* m_detailLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
};

#endif // TRMGRIDVIEW_H
