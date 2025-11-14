#ifndef ENLARGEPLOT_H
#define ENLARGEPLOT_H

#include <QDialog>
#include "qcustomplot.h"

namespace Ui {
class enlargePlot;
}

class enlargePlot : public QDialog
{
    Q_OBJECT

public:
    explicit enlargePlot(QWidget *parent = nullptr);
    ~enlargePlot();

    void loadPlot(QCustomPlot *sourcePlot); // 🔹 Copy from existing plot

private slots:
    void on_pushButton_fitToScreen_enlargedPlot_clicked();

    void on_pushButton_clearPoints_clicked();

private:
    Ui::enlargePlot *ui;

    QVector<QCPItemTracer*> m_tracers;
    QVector<QCPItemText*>   m_labels;
};

#endif // ENLARGEPLOT_H
