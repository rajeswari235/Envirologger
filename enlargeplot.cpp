#include "enlargeplot.h"
#include "ui_enlargeplot.h"

enlargePlot::enlargePlot(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::enlargePlot)
{
    ui->setupUi(this);

    //  Allow minimize, maximize, and close buttons in the title bar
    this->setWindowFlags(Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);

    this->setWindowTitle("Enlarged Plot");
    this->resize(900, 700);

    // Allow zoom and drag
    ui->customPlot_enlarge->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    //  Apply your neon-green theme
    QCustomPlot *plot = ui->customPlot_enlarge;
    plot->setBackground(QColor(10, 20, 10));
    plot->axisRect()->setBackground(QColor(15, 35, 15));

    QColor neonGreen(0, 255, 150);
    QColor softGreen(150, 255, 180);

    plot->xAxis->setLabelColor(neonGreen);
    plot->yAxis->setLabelColor(neonGreen);
    plot->xAxis->setTickLabelColor(softGreen);
    plot->yAxis->setTickLabelColor(softGreen);

    plot->xAxis->setBasePen(QPen(neonGreen, 1));
    plot->yAxis->setBasePen(QPen(neonGreen, 1));
    plot->xAxis->setTickPen(QPen(neonGreen, 1));
    plot->yAxis->setTickPen(QPen(neonGreen, 1));

    plot->xAxis->grid()->setPen(QPen(QColor(30, 60, 30)));
    plot->yAxis->grid()->setPen(QPen(QColor(30, 60, 30)));
    plot->xAxis->grid()->setSubGridPen(QPen(QColor(20, 40, 20)));
    plot->yAxis->grid()->setSubGridPen(QPen(QColor(20, 40, 20)));
    plot->xAxis->grid()->setSubGridVisible(true);
    plot->yAxis->grid()->setSubGridVisible(true);


    // Connect plottable click → create tracer + label
    connect(ui->customPlot_enlarge, &QCustomPlot::plottableClick,
            this, [this](QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event)
    {
        Q_UNUSED(event);
        if (!plottable) return;

        QCPGraph *g = qobject_cast<QCPGraph*>(plottable);
        if (!g) return;

        auto container = g->data();
        if (!container) return;

        // ---- GET CLICKED POINT (x,y) FROM dataIndex ----
        double x = 0, y = 0;
        int idx = 0;
        bool found = false;

        for (auto it = container->constBegin(); it != container->constEnd(); ++it, ++idx)
        {
            if (idx == dataIndex)
            {
                x = it->key;
                y = it->value;
                found = true;
                break;
            }
        }
        if (!found) return;

        // ---- CREATE NEW TRACER ----
        QCPItemTracer *tr = new QCPItemTracer(ui->customPlot_enlarge);
        tr->setGraph(g);
        tr->setGraphKey(x);
        tr->setStyle(QCPItemTracer::tsCircle);
        tr->setPen(QPen(Qt::yellow));
        tr->setBrush(QBrush(Qt::yellow));
        tr->setSize(7);
        tr->setVisible(true);
        tr->updatePosition();

        // ---- CREATE NEW (x,y) LABEL ----
        QCPItemText *lb = new QCPItemText(ui->customPlot_enlarge);

        lb->setText(QString("x: %1\ny: %2")
                    .arg(x, 0, 'f', 3)
                    .arg(y, 0, 'f', 3));

        // Strong readable label (not too transparent)
        lb->setBrush(QBrush(QColor(255, 255, 120, 120)));  // soft yellow glow
        lb->setPen(QPen(Qt::black));
        lb->setFont(QFont("Segoe UI", 9, QFont::Bold));
        lb->setPadding(QMargins(8, 6, 8, 6));

        // Place EXACTLY near selected point (never far)
        lb->setPositionAlignment(Qt::AlignLeft | Qt::AlignBottom);
        lb->position->setType(QCPItemPosition::ptPlotCoords);
        lb->position->setCoords(x, y);

        lb->setVisible(true);

        // ---- Store for clearing later ----
        m_tracers.append(tr);
        m_labels.append(lb);

        ui->customPlot_enlarge->replot();
    });

}


enlargePlot::~enlargePlot()
{
    delete ui;
}

void enlargePlot::loadPlot(QCustomPlot *sourcePlot)
{
    QCustomPlot *dest = ui->customPlot_enlarge;
    dest->clearGraphs();

    // Copy all graphs and their data
    for (int i = 0; i < sourcePlot->graphCount(); ++i)
    {
        dest->addGraph();
        dest->graph(i)->setPen(sourcePlot->graph(i)->pen());

        QVector<double> xData, yData;

        // Properly iterate through QCPDataContainer<QCPGraphData>
        auto dataContainer = sourcePlot->graph(i)->data();
        for (auto it = dataContainer->constBegin(); it != dataContainer->constEnd(); ++it)
        {
            xData.append(it->key);
            yData.append(it->value);
        }

        dest->graph(i)->setData(xData, yData);
    }

    // --- Copy axis labels, colors, and ranges ---
    dest->xAxis->setLabel(sourcePlot->xAxis->label());
    dest->yAxis->setLabel(sourcePlot->yAxis->label());
    dest->xAxis->setLabelColor(sourcePlot->xAxis->labelColor());
    dest->yAxis->setLabelColor(sourcePlot->yAxis->labelColor());
    dest->xAxis->setRange(sourcePlot->xAxis->range());
    dest->yAxis->setRange(sourcePlot->yAxis->range());

    // --- Apply same font theme (BOLD axis labels) ---
    QFont labelFont("Segoe UI", 9, QFont::Bold);
    QFont tickFont("Segoe UI", 8);
    dest->xAxis->setLabelFont(labelFont);
    dest->yAxis->setLabelFont(labelFont);
    dest->xAxis->setTickLabelFont(tickFont);
    dest->yAxis->setTickLabelFont(tickFont);

    // --- Apply your neon-green dark theme again ---
    dest->setBackground(QColor(10, 20, 10));              // outer area - deep green
    dest->axisRect()->setBackground(QColor(15, 35, 15));  // inner plotting area

    QColor neonGreen(0, 255, 150);
    QColor softGreen(150, 255, 180);

    dest->xAxis->setLabelColor(neonGreen);
    dest->yAxis->setLabelColor(neonGreen);
    dest->xAxis->setTickLabelColor(softGreen);
    dest->yAxis->setTickLabelColor(softGreen);

    dest->xAxis->setBasePen(QPen(neonGreen, 1));
    dest->yAxis->setBasePen(QPen(neonGreen, 1));
    dest->xAxis->setTickPen(QPen(neonGreen, 1));
    dest->yAxis->setTickPen(QPen(neonGreen, 1));

    dest->xAxis->grid()->setPen(QPen(QColor(30, 60, 30)));
    dest->yAxis->grid()->setPen(QPen(QColor(30, 60, 30)));
    dest->xAxis->grid()->setSubGridPen(QPen(QColor(20, 40, 20)));
    dest->yAxis->grid()->setSubGridPen(QPen(QColor(20, 40, 20)));
    dest->xAxis->grid()->setSubGridVisible(true);
    dest->yAxis->grid()->setSubGridVisible(true);

    // Enable zoom/drag again
    dest->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    dest->replot();
}



void enlargePlot::on_pushButton_fitToScreen_enlargedPlot_clicked()
{
    QCustomPlot *plot = ui->customPlot_enlarge;
    if (!plot) return;

    bool hasData = false;

    // Check if there’s any data in this enlarged plot
    for (int i = 0; i < plot->graphCount(); ++i)
    {
        if (plot->graph(i)->dataCount() > 0)
        {
            hasData = true;
            break;
        }
    }

    if (hasData)
    {
        //  Auto-fit axes to show all data
        plot->rescaleAxes(true);

        // Optional margin (so data points aren’t cut)
        plot->xAxis->scaleRange(1.05, plot->xAxis->range().center());
        plot->yAxis->scaleRange(1.05, plot->yAxis->range().center());

        qDebug() << "Enlarged plot fitted to data.";
    }
    else
    {
        //  No data: reset to initial look
        plot->xAxis->setRange(0, 100);
        plot->yAxis->setRange(-5, 5);

        qDebug() << "No data found — enlarged plot reset to default range.";
    }

    plot->replot();
}


void enlargePlot::on_pushButton_clearPoints_clicked()
{
    for (QCPItemTracer *t : m_tracers)
        ui->customPlot_enlarge->removeItem(t);

    for (QCPItemText *l : m_labels)
        ui->customPlot_enlarge->removeItem(l);

    m_tracers.clear();
    m_labels.clear();

    ui->customPlot_enlarge->replot();
}
