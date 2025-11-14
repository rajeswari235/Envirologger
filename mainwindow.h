#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <serialporthandler.h>
#include <QMessageBox>
#include <QFile>
#include <QDateTime>
#include <QTimer>
#include <windows.h>
#include <psapi.h>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QApplication>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QLabel>
#include <QScreen>
#include <QInputDialog>

#include <enlargeplot.h>
#include "xlsxdocument.h"   // QXlsx header

#include <complex>
#include <vector>
#include <cmath>



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Forward declaration of serialPortHandler
class serialPortHandler;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void refreshPorts();

    //For Saving log Data
    void resetLogFile();
    static void writeToNotes(const QString &data);
    void initializeLogFile();
    void closeLogFile();

    quint8 calculateChecksum(const QByteArray &data);
    QString hexBytes(QByteArray &cmd);

    //Extra features
    void printMemoryUsage();

    void elapseStart();
    void elapseEnd(bool goFurther = false, const QString &label = "");

    QDialog* createPleaseWaitDialog(const QString &text, int timeSeconds = 0);

    inline void pauseFor(int milliseconds) {
        QEventLoop loop;
        QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);  // After delay, quit the event loop
        loop.exec();  // Start the event loop and wait for it to quit
        QApplication::processEvents();  // Keep UI healthy
    }

    void initializeAllPlots();

    void makePacket32UI(QList<QByteArray> &rawPacket32List);
    void makePacket4100AdxlTempList(QList<QByteArray> &rawPacket4100AdxlList,QList<QByteArray> &rawPacketTemperatureList);
    void makePacket4100InclList(QList<QByteArray> &rawPacket4100InclList);

    void saveAllSensorDataToExcel(const QVector<double> &adxlIndex,
                                              const QVector<double> &xAdxl,
                                              const QVector<double> &yAdxl,
                                              const QVector<double> &zAdxl,
                                              const QVector<double> &tempIndex,
                                              const QVector<double> &temperature,
                                              const QVector<double> &inclIndex,
                                              const QVector<double> &inclX,
                                              const QVector<double> &inclY);

   void initializeSensorVectors();

   // FFT helpers as class member functions
   int nextPowerOfTwo(int v);

   void fftRecursive(std::vector<std::complex<double>> &a);

   void plotFFT(const QVector<double> &signal,
                QCustomPlot *plot,
                double sampleInterval = 0.1);

   void setupFFTPlot(QCustomPlot *plot, const QString &xLabel);


private slots:
        void onPortSelected(const QString &portName);

        void portStatus(const QString&);

        void showGuiData(const QByteArray &byteArrayData);

        //response time handling

        void handleTimeout();

        void onDataReceived();

        void on_pushButton_calibrateScreen_clicked();

        void on_pushButton_getEventData_clicked();

        void on_pushButton_startLog_clicked();

        void on_pushButton_getLogEvents_clicked();

        void on_pushButton_stopPlot_clicked();

        void on_pushButton_enlargePlot_clicked();

        void on_pushButton_fitToScreen_clicked();

        void on_pushButton_saveLogPlots_clicked();

        void on_pushButton_clearLogPlots_clicked();

        void on_pushButton_clearPlots_clicked();

        void on_pushButton_fitToScreen_fft_clicked();

        void on_pushButton_clearPoints_fft_clicked();

signals:
    void sendMsgId(quint8 id);

private:
    Ui::MainWindow *ui;
    serialPortHandler *serialObj;

    //Log handling
    static QFile logFile;
    static QTextStream logStream;

    //Response Time waiting timer
     QTimer *responseTimer = nullptr; // Timer to track response timeout

    //Extras
     QElapsedTimer elapsedTimer;

     QDialog *dlg = nullptr;

     QDialog *dlgPlot = nullptr;

     // --- ADXL ---
     QVector<double> finalAdxlIndex;
     QVector<double> finalXAdxl;
     QVector<double> finalYAdxl;
     QVector<double> finalZAdxl;

     // --- Temperature ---
     QVector<double> finalTempIndex;
     QVector<double> finalTemperature;

     // --- Inclinometer ---
     QVector<double> finalInclIndex;
     QVector<double> finalInclX;
     QVector<double> finalInclY;


};
#endif // MAINWINDOW_H
