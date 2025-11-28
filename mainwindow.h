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
#include "kiss_fft.h"
#include <QString>




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
   void computeAndPlotFFT(const QVector<double>& signal,
                          double Fs,
                          QCustomPlot *plot);

   void on_pushButton_clearPoints_fft_clicked();
    void on_pushButton_fitToScreen_fft_clicked();
    void removeDC(QVector<double> &x);

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


       // void on_tabWidget_currentChanged(int index);

        void on_pushButton_logTime_clicked();

        void on_pushButton_setthreshold_clicked();

        void on_pushButton_setTime_clicked();

        void on_pushButton_ADXLfrequency_clicked();

        void on_pushButton_inclinometerFrequency_clicked();

        void on_pushButton_remainingLogs_clicked();

        void on_pushButton_on_clicked();

        void on_pushButton_off_clicked();

        void blinkWidget(QWidget *w);

        void on_pushButton_currentParameters_clicked();

        //fft functions

        void applyHanning(QVector<double> &signal);
        void performFFT(const QVector<double> &input,
                        QVector<double> &magnitude,
                        QVector<double> &freqAxis,
                        double sampleRate);
//        QVector<double>generateSineWave(double frequency,
//                                                     double amplitude,
//                                                     double durationSeconds,
//                                                     double sampleRate);
//        QVector<double>generateSineWave(double freq1,
//                                                     double amp1,
//                                                     double freq2,
//                                                     double amp2,
//                                                     double durationSeconds,
//                                                     double sampleRate);
       void setupFFTPlot(QCustomPlot *plot, const QString &xLabel);



       void on_spinBox_samplingfrequency_valueChanged(int arg1);

       void on_spinBox_Inclinometer_valueChanged(int arg1);

     //  void testPureSineFFT(int N, double Fs, double f, QCustomPlot *plot);


       void on_pushButton_erase_clicked();

signals:
    void sendMsgId(quint8 id);

private:
    Ui::MainWindow *ui;
    serialPortHandler *serialObj;
    QTimer timer;
    QCustomPlot *fftPlot;
    QList<QCPItemTracer*> fftTracers;
    QList<QCPItemText*>   fftLabels;


    //Log handling
    static QFile logFile;
    static QTextStream logStream;
    void setupPlot(QCustomPlot *plot, const QString &xLabel, const QString &yLabel,bool noClearGraph=0);


    //Response Time waiting timer
     QTimer *responseTimer = nullptr; // Timer to track response timeout

    //Extras
     QElapsedTimer elapsedTimer;

     QDialog *dlg = nullptr;

     QDialog *dlgPlot = nullptr;
      QDialog *eraseDlg=nullptr;

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

     QList<QByteArray> packet32List;
     QList<QByteArray> packet4100AdxlList;
     QList<QByteArray> packet4100InclList;
     QList<QByteArray> packetTemperatureList;

     quint16 adxlFreq;
     quint16 inclinometerFreq;

};
#endif // MAINWINDOW_H
