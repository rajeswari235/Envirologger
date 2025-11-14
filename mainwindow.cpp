#include "mainwindow.h"
#include "ui_mainwindow.h"

QFile MainWindow::logFile;
QTextStream MainWindow::logStream;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    serialObj =   new serialPortHandler(this);

    connect(ui->pushButton_clear,&QPushButton::clicked,ui->textEdit_rawBytes,&QTextEdit::clear);

    ui->comboBox_ports->addItems(serialObj->availablePorts());

    connect(ui->pushButton_portsRefresh,&QPushButton::clicked,this,&MainWindow::refreshPorts);

    connect(ui->comboBox_ports,SIGNAL(activated(const QString &)),this,SLOT(onPortSelected(const QString &)));

    connect(this,&MainWindow::sendMsgId,serialObj,&serialPortHandler::recvMsgId);


    //writeToNotes from serial class
    connect(serialObj,&serialPortHandler::executeWriteToNotes,this,&MainWindow::writeToNotes);

    //debugging signals
    connect(serialObj,&serialPortHandler::portOpening,this,&MainWindow::portStatus);

    //gui display signal
    connect(serialObj,&serialPortHandler::guiDisplay,this,&MainWindow::showGuiData);

    //reset previous notes #Notes things : Logging file
    resetLogFile();
    writeToNotes(+"    ******    "+QCoreApplication::applicationName() +
                 "     Application Started");
    //#################################################

    //Response Timer *********************************************##############
    responseTimer = new QTimer(this);
    responseTimer->setSingleShot(true); // Ensure it fires only once per use

    // Connect the timer's timeout signal to a slot that handles the timeout
    connect(responseTimer, &QTimer::timeout, this, &MainWindow::handleTimeout);

    connect(serialObj, &serialPortHandler::dataReceived, this, &MainWindow::onDataReceived);
    //************************************************************##############

    writeToNotes("Pointer Size: "+QString::number(sizeof(void *))+" If it is 8 : 64 bit else 4 means 32 bit");

    setWindowTitle("Envirologger");

    showMaximized();

    ui->tabWidget->setCurrentWidget(ui->tab_logger);

    initializeAllPlots();

    // Setting Table Get Log Events
    ui->tableWidget_getLogEvents->setColumnCount(3);
    ui->tableWidget_getLogEvents->setHorizontalHeaderLabels({"Event ID", "Start Time and Date", "End Time and Date"});

    ui->tableWidget_getLogEvents->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->tableWidget_getLogEvents->setAlternatingRowColors(true);

    auto header = ui->tableWidget_getLogEvents->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);

}

MainWindow::~MainWindow()
{
    writeToNotes(+"    ******    "+QCoreApplication::applicationName() +
                 "     Application Closed");
    delete ui;
    delete serialObj;
    delete responseTimer;
    closeLogFile();
}

void MainWindow::initializeLogFile() {
    if (!logFile.isOpen()) {
        logFile.setFileName("debug_notes.txt");
        if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
            qCritical() << "Failed to open log file.";
        } else {
            logStream.setDevice(&logFile);
        }
    }
}

void MainWindow::resetLogFile() {
    // Close the log file if it is open
    if (logFile.isOpen()) {
        logStream.flush();
        logFile.close();
    }

    // Check if the file exists and delete it
    QFile::remove("debug_notes.txt");

    // Reinitialize the log file
    initializeLogFile();
}


void MainWindow::writeToNotes(const QString &data) {
    if (!logFile.isOpen()) {
        qCritical() << "Log file is not open.";
        return;
    }

    // Add a timestamp for each entry
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    logStream << "[" << timestamp << "] " << data << Qt::endl;
    logStream.flush(); // Ensure immediate write to disk
}

void MainWindow::closeLogFile() {
    if (logFile.isOpen()) {
        logStream.flush();
        logFile.close();
    }
}

quint8 MainWindow::calculateChecksum(const QByteArray &data)
{
    quint8 checkSum = 0;
    for(quint8 byte : data)
    {
        checkSum ^= byte;
    }

    return checkSum;
}

void MainWindow::refreshPorts()
{
    QString currentPort = ui->comboBox_ports->currentText();

    qDebug()<<"Refreshing ports...";
    ui->comboBox_ports->clear();
    QStringList availablePorts;
    ui->comboBox_ports->addItems(serialObj->availablePorts());

    ui->comboBox_ports->setCurrentText(currentPort);
}

void MainWindow::onPortSelected(const QString &portName)
{
    serialObj->setPORTNAME(portName);
}

void MainWindow::handleTimeout()
{
    QMessageBox::warning(this, "Timeout", "Hardware Not Responding!");
}

void MainWindow::onDataReceived()
{
    // Stop the timer since data has been received
    qDebug()<<"Hello stop it";
    if (responseTimer->isActive()) {
        responseTimer->stop();
    }
}


//The below function is intended for providing space between hex bytes
QString MainWindow::hexBytes(QByteArray &cmd)
{
    //**************************Visuals*******************
    QString hexOutput = cmd.toHex().toUpper();
    QString formattedHexOutput;

    for (int i = 0; i < hexOutput.size(); i += 2) {
        if (i > 0) {
            formattedHexOutput += " ";
        }
        formattedHexOutput += hexOutput.mid(i, 2);
    }
    return formattedHexOutput;
    //**************************Visuals*******************
}

void MainWindow::printMemoryUsage()
{
    PROCESS_MEMORY_COUNTERS_EX memInfo;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&memInfo, sizeof(memInfo))) {
        SIZE_T workingSet = memInfo.WorkingSetSize;
        SIZE_T privateUsage = memInfo.PrivateUsage;

        qDebug() << "Working Set (RAM):"
                 << workingSet / 1024 << "KB ("
                 << QString::number(workingSet / (1024.0 * 1024.0), 'f', 2) << "MB)";

        qDebug() << "Private Bytes:"
                 << privateUsage / 1024 << "KB ("
                 << QString::number(privateUsage / (1024.0 * 1024.0), 'f', 2) << "MB)";
    } else {
        qDebug() << "Failed to get memory info!";
    }
}

void MainWindow::elapseStart()
{
    elapsedTimer.start();
}

void MainWindow::elapseEnd(bool goFurther, const QString &label)
{
    qint64 ns = elapsedTimer.nsecsElapsed();
    double ms = ns / 1000000.0;

    if (label.isEmpty()) {
        qDebug() << "Time taken from elapseStart() to elapseEnd():"
                 << ns << "ns (" << ms << "ms)";
    } else {
        qDebug() << "Elapsed [" << label << "]:"
                 << ns << "ns (" << ms << "ms)";
    }

    if (!goFurther)
        elapsedTimer.restart();
}

QDialog* MainWindow::createPleaseWaitDialog(const QString &text, int timeSeconds)
{
    // --- Create dialog ---
    QDialog *dlg = new QDialog(this);
    dlg->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModal(true);

    // --- Styling ---
    dlg->setStyleSheet(R"(
        QDialog {
            background-color: #f8f8f8;
            border: 2px solid #0078D7;
            border-radius: 8px;
        }
        QLabel {
            font-size: 16px;
            padding: 10px;
        }
    )");

    // --- Layout and main label ---
    QVBoxLayout *layout = new QVBoxLayout(dlg);
    QLabel *mainLabel = new QLabel(text, dlg);
    layout->addWidget(mainLabel);

    QLabel *timerLabel = nullptr;

    // --- Optional countdown ---
    if (timeSeconds > 0)
    {
        timerLabel = new QLabel(QString("Remaining: %1s").arg(timeSeconds), dlg);
        timerLabel->setAlignment(Qt::AlignCenter);
        timerLabel->setStyleSheet("color: #0078D7; font-weight: bold;");
        layout->addWidget(timerLabel);

        QTimer *countdown = new QTimer(dlg);
        countdown->setInterval(1000);

        int *remaining = new int(timeSeconds);

        QObject::connect(countdown, &QTimer::timeout, dlg, [countdown, remaining, timerLabel]() {
            (*remaining)--;
            if (*remaining <= 0)
            {
                countdown->stop();
                delete remaining;
            }
            else
            {
                timerLabel->setText(QString("Remaining: %1s").arg(*remaining));
            }
        });

        countdown->start();
    }

    dlg->setLayout(layout);
    dlg->adjustSize();
    dlg->setFixedSize(dlg->sizeHint());
    dlg->show();

    QApplication::processEvents(); // ensures dialog appears immediately

    return dlg;
}


void MainWindow::initializeAllPlots()
{
    // Common setup lambda
    auto setupPlot = [](QCustomPlot *plot, const QString &xLabel, const QString &yLabel)
    {
        plot->clearGraphs();
        plot->xAxis->setLabel(xLabel);
        plot->yAxis->setLabel(yLabel);
        plot->legend->setVisible(false);

        // Enable user interactions
        plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

        // Set axis fonts
        QFont labelFont("Segoe UI", 9, QFont::Bold);
        QFont tickFont("Segoe UI", 8);
        plot->xAxis->setLabelFont(labelFont);
        plot->yAxis->setLabelFont(labelFont);
        plot->xAxis->setTickLabelFont(tickFont);
        plot->yAxis->setTickLabelFont(tickFont);

        //  Dark green background
        plot->setBackground(QColor(10, 20, 10));              // outer area - deep green
        plot->axisRect()->setBackground(QColor(15, 35, 15));  // inner plotting area

        //  Axis text + pen colors (neon green for clarity)
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

        // Subtle greenish grid lines
        plot->xAxis->grid()->setPen(QPen(QColor(30, 60, 30)));
        plot->yAxis->grid()->setPen(QPen(QColor(30, 60, 30)));
        plot->xAxis->grid()->setSubGridPen(QPen(QColor(20, 40, 20)));
        plot->yAxis->grid()->setSubGridPen(QPen(QColor(20, 40, 20)));
        plot->xAxis->grid()->setSubGridVisible(true);
        plot->yAxis->grid()->setSubGridVisible(true);

        plot->replot();
    };

    //  Graph color themes — tuned for neon-green background
    QColor adxlColors[] = {
        QColor(255, 60, 60),    // ADXL X - Bright Red
        QColor(255, 180, 0),    // ADXL Y - Deep Amber
        QColor(120, 180, 255)   // ADXL Z - Soft Sky Blue
    };

    QColor inclinometerColors[] = {
        QColor(255, 100, 255),  // Inclinometer X - Vivid Magenta
        QColor(0, 255, 220)     // Inclinometer Y - Aqua Cyan
    };

    QColor tempColor(255, 255, 100); // Temperature - Bright Yellow


    //  ADXL X, Y, Z — voltages vs samples (1 unit = 100 ms)
    setupPlot(ui->customPlot_adxl_x, "ADXL X Time (1 = 100 ms)", "Voltage (V)");
    setupPlot(ui->customPlot_adxl_y, "ADXL Y Time (1 = 100 ms)", "Voltage (V)");
    setupPlot(ui->customPlot_adxl_z, "ADXL Z Time (1 = 100 ms)", "Voltage (V)");

    ui->customPlot_adxl_x->addGraph(); ui->customPlot_adxl_x->graph(0)->setPen(QPen(adxlColors[0], 1));
    ui->customPlot_adxl_y->addGraph(); ui->customPlot_adxl_y->graph(0)->setPen(QPen(adxlColors[1], 1));
    ui->customPlot_adxl_z->addGraph(); ui->customPlot_adxl_z->graph(0)->setPen(QPen(adxlColors[2], 1));

    //  Inclinometer X, Y — degrees vs time (1 unit = 1 ms)
    setupPlot(ui->customPlot_inclinometer_x, "Inclinometer Time X (ms)", "Degrees (°)");
    setupPlot(ui->customPlot_inclinometer_y, "Inclinometer Time Y (ms)", "Degrees (°)");

    ui->customPlot_inclinometer_x->addGraph(); ui->customPlot_inclinometer_x->graph(0)->setPen(QPen(inclinometerColors[0], 1));
    ui->customPlot_inclinometer_y->addGraph(); ui->customPlot_inclinometer_y->graph(0)->setPen(QPen(inclinometerColors[1], 1));

    //  Temperature — Celsius vs samples
    setupPlot(ui->customPlot_temperature, "Samples", "Temperature (°C)");
    ui->customPlot_temperature->addGraph(); ui->customPlot_temperature->graph(0)->setPen(QPen(tempColor, 1.1));

    // Axis ranges start clean
    for (auto plot : {ui->customPlot_adxl_x, ui->customPlot_adxl_y, ui->customPlot_adxl_z,
                      ui->customPlot_inclinometer_x, ui->customPlot_inclinometer_y,
                      ui->customPlot_temperature})
    {
        plot->xAxis->setRange(0, 100);
        plot->yAxis->setRange(-5, 5);
        plot->replot();
    }
}

void MainWindow::makePacket32UI(QList<QByteArray> &rawPacket32List)
{
    if(rawPacket32List.size() == 1)
    {
        QByteArray Item1 = rawPacket32List[0];
        qDebug()<<Item1.toHex(' ').toUpper();

        // Extracting eventId
        quint8 highByte = static_cast<quint8>(Item1[2]);
        quint8 lowByte  = static_cast<quint8>(Item1[3]);

        quint16 eventId = (highByte << 8) | lowByte;

        ui->lineEdit_eventId->setText(QString::number(eventId));

        ui->lineEdit_eventId->setStyleSheet("background-color:yellow");

        QTimer::singleShot(500,[this](){
            ui->lineEdit_eventId->setStyleSheet("");
        });

        //Extracting Start Time and End Time

        // Bytes extraction
        QByteArray startTimeBytes = Item1.mid(20,6);
        QByteArray endTimeBytes = Item1.mid(26,6);

        // Bytes to Decimals conversion
        QVector<quint8> startTimeDecimals;
        for(auto eachByte : startTimeBytes)
        {
            startTimeDecimals.append(static_cast<quint8>(eachByte));
        }
        qDebug()<<startTimeDecimals<<" :startTimeDecimals";

        QVector<quint8> endTimeDecimals;
        for(auto eachByte : endTimeBytes)
        {
            endTimeDecimals.append(static_cast<quint8>(eachByte));
        }
        qDebug()<<endTimeDecimals<<" :endTimeDecimals";

        // Decimals to string conversion
        QStringList startTimeStringList;
        for(auto parts : startTimeDecimals)
        {
            startTimeStringList.append(QString::number(parts));
        }

        QString uiStartTime = startTimeStringList.join(":");
        qDebug()<<uiStartTime<<" :uiStartTime";

        QStringList endTimeStringList;
        for(auto parts : endTimeDecimals)
        {
            endTimeStringList.append(QString::number(parts));
        }

        QString uiEndTime = endTimeStringList.join(":");
        qDebug()<<uiEndTime<<" :uiEndTime";

        // Formatting the strings
        // Split the string by ':'
        QStringList startParts = uiStartTime.split(":");
        QStringList endParts   = uiEndTime.split(":");

        if (startParts.size() == 6 && endParts.size() == 6)
        {
            QString formattedStart = QString("%1:%2:%3_%4/%5/%6")
                    .arg(startParts[0]).arg(startParts[1]).arg(startParts[2])
                    .arg(startParts[3]).arg(startParts[4]).arg(startParts[5]);

            QString formattedEnd = QString("%1:%2:%3_%4/%5/%6")
                    .arg(endParts[0]).arg(endParts[1]).arg(endParts[2])
                    .arg(endParts[3]).arg(endParts[4]).arg(endParts[5]);

            qDebug() << formattedStart << " :formattedStart";
            qDebug() << formattedEnd   << " :formattedEnd";

            ui->lineEdit_startTime->setText(formattedStart);
            ui->lineEdit_endTime->setText(formattedEnd);

            // Ui blinking
            ui->lineEdit_startTime->setStyleSheet("background-color:yellow");

            QTimer::singleShot(500,[this](){
                ui->lineEdit_startTime->setStyleSheet("");
            });

            ui->lineEdit_endTime->setStyleSheet("background-color:yellow");

            QTimer::singleShot(500,[this](){
                ui->lineEdit_endTime->setStyleSheet("");
            });

        }
        else
        {
            qDebug() << "Invalid time format!";
        }
    }
    else
    {
       QMessageBox::warning(this,"Error","packet32List size is more than 1");
    }
}

void MainWindow::makePacket4100AdxlTempList(QList<QByteArray> &rawPacket4100AdxlList,
                                            QList<QByteArray> &rawPacketTemperatureList)
{
    QVector<double> sampleIndex;
    QVector<double> xAdxl, yAdxl, zAdxl;
    int globalSample = 0;

    // --- ADXL Data Processing ---
    for (int p = 0; p < rawPacket4100AdxlList.size(); ++p)
    {
        QByteArray packet = rawPacket4100AdxlList[p];

        if (packet.size() < 20)
        {
            qDebug() << "Skipping too short ADXL packet:" << packet.size();
            continue;
        }

        QByteArray trimmed = packet.mid(3);
        if (trimmed.size() > 3) trimmed.chop(3); // remove footer
        if (trimmed.size() > 2) trimmed.chop(2); // remove temperature bytes

        int usableSize = trimmed.size();
        if (usableSize < 6)
        {
            qDebug() << "Packet too short after trimming:" << usableSize;
            continue;
        }

        for (int i = 0; i + 5 < usableSize; i += 6)
        {
            qint16 xRaw = (static_cast<quint8>(trimmed[i])     << 8) | static_cast<quint8>(trimmed[i + 1]);
            qint16 yRaw = (static_cast<quint8>(trimmed[i + 2]) << 8) | static_cast<quint8>(trimmed[i + 3]);
            qint16 zRaw = (static_cast<quint8>(trimmed[i + 4]) << 8) | static_cast<quint8>(trimmed[i + 5]);

            // Keep last 12 bits only first 4 bits eliminate in a 16 bit integer
            xRaw &= 0x0FFF;
            yRaw &= 0x0FFF;
            zRaw &= 0x0FFF;

            sampleIndex.append(globalSample++);
            xAdxl.append((xRaw * 3.3 * 2) / 4096.0);
            yAdxl.append((yRaw * 3.3 * 2) / 4096.0);
            zAdxl.append((zRaw * 3.3 * 2) / 4096.0);
        }


        qDebug() << "Processed ADXL packet" << p << ", extracted" << usableSize / 6 << "samples";
    }

    qDebug() << "Total ADXL samples:" << sampleIndex.size();

    // --- Temperature Data Processing ---
    QVector<double> tempIndex;
    QVector<double> temperatureValues;

    for (int i = 0; i < rawPacketTemperatureList.size(); ++i)
    {
        QByteArray tempBytes = rawPacketTemperatureList[i];
        if (tempBytes.size() < 2)
        {
            qDebug() << "Skipping short temperature packet:" << tempBytes.size();
            continue;
        }

        quint16 tempRaw = (static_cast<quint8>(tempBytes[0]) << 8) | static_cast<quint8>(tempBytes[1]);
        tempIndex.append(i);

        // Keep first 14 bits only last 2 bits eliminate in a 16 bit integer
        tempRaw &= ~0x0003;

        temperatureValues.append(-46.85 + (175.72 * tempRaw) / 65536.0);
    }

    qDebug() << "Total temperature samples:" << temperatureValues.size();

    // --- Plotting Helper ---
    auto plotGraph = [](QCustomPlot *plot, const QVector<double> &x, const QVector<double> &y)
    {
        if (plot->graphCount() > 0)
        {
            plot->graph(0)->setData(x, y);
            plot->rescaleAxes();
            plot->replot();
        }
    };

    // --- Plot ADXL ---
    plotGraph(ui->customPlot_adxl_x, sampleIndex, xAdxl);
    plotGraph(ui->customPlot_adxl_y, sampleIndex, yAdxl);
    plotGraph(ui->customPlot_adxl_z, sampleIndex, zAdxl);

    // --- Plot Temperature ---
    plotGraph(ui->customPlot_temperature, tempIndex, temperatureValues);

    // --- Passing local values to global values

    this->finalAdxlIndex = sampleIndex;
    this->finalXAdxl = xAdxl;
    this->finalYAdxl = yAdxl;
    this->finalZAdxl = zAdxl;

    this->finalTempIndex = tempIndex;
    this->finalTemperature = temperatureValues;

}

void MainWindow::makePacket4100InclList(QList<QByteArray> &rawPacket4100InclList)
{
    QVector<double> sampleIndex;
    QVector<double> inclX, inclY;
    int globalSample = 0;

    for (int p = 0; p < rawPacket4100InclList.size(); ++p)
    {
        QByteArray packet = rawPacket4100InclList[p];

        if (packet.size() < 20)
        {
            qDebug() << "Skipping too short Inclinometer packet:" << packet.size();
            continue;
        }

        // Remove header (3 bytes)
        QByteArray trimmed = packet.mid(3);

        // Remove footer (3 bytes)
        if (trimmed.size() > 3)
            trimmed.chop(3);

        // Remove last 2 dummy bytes before footer
        if (trimmed.size() > 2)
            trimmed.chop(2);

        int usableSize = trimmed.size();
        if (usableSize < 4)
        {
            qDebug() << "Packet too short after trimming:" << usableSize;
            continue;
        }

        // Process each 4-byte sample (Xg, Yg)
        for (int i = 0; i + 3 < usableSize; i += 4)
        {
            qint16 xRaw = (static_cast<quint8>(trimmed[i + 1])     << 8) | static_cast<quint8>(trimmed[i]);
            qint16 yRaw = (static_cast<quint8>(trimmed[i + 3]) << 8) | static_cast<quint8>(trimmed[i + 2]);

            // Convert to g-values
            double xg = (xRaw * 0.031) / 1000.0;
            double yg = (yRaw * 0.031) / 1000.0;

            // Clamp to [-1, 1]
            xg = std::max(-1.0, std::min(1.0, xg));
            yg = std::max(-1.0, std::min(1.0, yg));

            // Convert to degrees
            double xDeg = std::asin(xg) * (180.0 / M_PI);
            double yDeg = std::asin(yg) * (180.0 / M_PI);

            sampleIndex.append(globalSample++);
            inclX.append(xDeg);
            inclY.append(yDeg);
        }

        qDebug() << "Processed Incl packet" << p << ", extracted" << usableSize / 4 << "samples";
    }

    qDebug() << "Total Incl samples:" << sampleIndex.size();

    // --- Plotting ---
    auto plotGraph = [](QCustomPlot *plot, const QVector<double> &x, const QVector<double> &y)
    {
        if (plot->graphCount() > 0)
        {
            plot->graph(0)->setData(x, y);
            plot->rescaleAxes();
            plot->replot();
        }
    };

    plotGraph(ui->customPlot_inclinometer_x, sampleIndex, inclX);
    plotGraph(ui->customPlot_inclinometer_y, sampleIndex, inclY);

    // --- Passing local values to global values
    this->finalInclIndex = sampleIndex;
    this->finalInclX = inclX;
    this->finalInclY = inclY;

}

void MainWindow::saveAllSensorDataToExcel(const QVector<double> &adxlIndex,
                                          const QVector<double> &xAdxl,
                                          const QVector<double> &yAdxl,
                                          const QVector<double> &zAdxl,
                                          const QVector<double> &tempIndex,
                                          const QVector<double> &temperature,
                                          const QVector<double> &inclIndex,
                                          const QVector<double> &inclX,
                                          const QVector<double> &inclY)
{
    QXlsx::Document xlsx;

    // ---------- HEADER FORMAT ----------
    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    headerFormat.setBorderStyle(QXlsx::Format::BorderThin);

    // ---------- DATA FORMAT ----------
    QXlsx::Format dataFormat;
    dataFormat.setBorderStyle(QXlsx::Format::BorderThin);

    // ---------------- HEADERS ----------------
    xlsx.write("A1", "Sample Index", headerFormat);
    xlsx.write("B1", "ADXL X (V)",   headerFormat);
    xlsx.write("C1", "ADXL Y (V)",   headerFormat);
    xlsx.write("D1", "ADXL Z (V)",   headerFormat);

    xlsx.write("F1", "Temp Index",   headerFormat);
    xlsx.write("G1", "Temperature (°C)", headerFormat);

    xlsx.write("I1", "Incl Index",   headerFormat);
    xlsx.write("J1", "Incl X (deg)", headerFormat);
    xlsx.write("K1", "Incl Y (deg)", headerFormat);

    // ---------- COLUMN WIDTHS ----------
    xlsx.setColumnWidth(1, 1, 12);   // Index
    xlsx.setColumnWidth(2, 4, 15);   // ADXL X,Y,Z
    xlsx.setColumnWidth(6, 7, 15);   // Temperature
    xlsx.setColumnWidth(9, 11, 15);  // Inclinometer

    int row = 2;

    // ------------ ADXL Values --------------
    for (int i = 0; i < xAdxl.size(); i++)
    {
        xlsx.write(row, 1, adxlIndex[i], dataFormat);
        xlsx.write(row, 2, xAdxl[i],     dataFormat);
        xlsx.write(row, 3, yAdxl[i],     dataFormat);
        xlsx.write(row, 4, zAdxl[i],     dataFormat);
        row++;
    }

    // ------------ Temperature Values --------------
    int tRow = 2;
    for (int i = 0; i < temperature.size(); i++)
    {
        xlsx.write(tRow, 6, tempIndex[i],   dataFormat);
        xlsx.write(tRow, 7, temperature[i], dataFormat);
        tRow++;
    }

    // ------------ Inclinometer Values --------------
    int iRow = 2;
    for (int i = 0; i < inclX.size(); i++)
    {
        xlsx.write(iRow, 9,  inclIndex[i], dataFormat);
        xlsx.write(iRow, 10, inclX[i],     dataFormat);
        xlsx.write(iRow, 11, inclY[i],     dataFormat);
        iRow++;
    }

    // ---------------- SIMPLE FILE DIALOG ----------------
    QString defaultName = QString("SensorData_%1.xlsx")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    QString fullPath = QFileDialog::getSaveFileName(
                this,
                "Save Sensor Data",
                desktopPath + "/" + defaultName,
                "Excel Files (*.xlsx)"
    );

    if (fullPath.isEmpty()) {
        QMessageBox::information(this, "Save Cancelled",
                                 "User cancelled the file save operation.");
        return;
    }

    if (!fullPath.endsWith(".xlsx", Qt::CaseInsensitive))
        fullPath += ".xlsx";

    // ---------------- SAVE ----------------
    if (xlsx.saveAs(fullPath)) {
        QMessageBox::information(this, "Success",
                                 "Sensor data saved successfully at:\n" + fullPath);
    } else {
        QMessageBox::critical(this, "Save Failed",
                              "Failed to save Excel file.\nCheck permissions or try another location.");
    }
}


void MainWindow::initializeSensorVectors()
{
    // --- ADXL ---
    finalAdxlIndex.clear();
    finalXAdxl.clear();
    finalYAdxl.clear();
    finalZAdxl.clear();

    // --- Temperature ---
    finalTempIndex.clear();
    finalTemperature.clear();

    // --- Inclinometer ---
    finalInclIndex.clear();
    finalInclX.clear();
    finalInclY.clear();
}



void MainWindow::portStatus(const QString &data)
{
    if(data.startsWith("Serial object is not initialized/port not selected"))
    {
        if(dlgPlot)
        {
            dlgPlot->close();
            dlgPlot = nullptr;
        }

        QMessageBox::critical(this,"Port Error","Please Select Port Using Above Dropdown");
    }

    if(data.startsWith("Serial port ") && data.endsWith(" opened successfully at baud rate 921600"))
    {
        QMessageBox::information(this,"Success",data);
    }

    if(data.startsWith("Failed to open port"))
    {
        if(dlgPlot)
        {
            dlgPlot->close();
            dlgPlot = nullptr;
        }
        QMessageBox::critical(this,"Error",data);
    }

    ui->textEdit_rawBytes->append(data);
}

void MainWindow::showGuiData(const QByteArray &byteArrayData)
{
    QByteArray data = byteArrayData;

    // Get Event Data Command mdgId 0x01
    if(data.startsWith(QByteArray::fromHex("AA BB")) && data.endsWith(QByteArray::fromHex("AA BB CC DD FF")))
    {
        int i = 0;

        QList<QByteArray> packet32List;
        QList<QByteArray> packet4100AdxlList;
        QList<QByteArray> packet4100InclList;
        QList<QByteArray> packetTemperatureList;

        int invalidHeaderCount = 0;

        while (i < data.size())
        {
            //  Make sure we have at least 3 bytes for a header
            if (i + 3 > data.size())
                break;

            QByteArray header = data.mid(i, 3);

            // --- Case 1: Packet32 ---
            if (header.startsWith(QByteArray::fromHex("AA BB")))
            {
                if (i + 32 <= data.size())
                {
                    QByteArray packet32 = data.mid(i, 32);
                    packet32List.append(packet32);
                    i += 32;
                    continue;
                }
                else break; // incomplete packet at end
            }

            // --- Case 2: Packet4100_ADXL ---
            else if (header == QByteArray::fromHex("CC DD FF"))
            {
                if (i + 4100 <= data.size())
                {
                    QByteArray packet4100 = data.mid(i, 4100);
                    if (packet4100.endsWith(QByteArray::fromHex("FF EE FF")))
                    {
                        if(packet4100.contains(QByteArray::fromHex("FF FF FF FF FF FF")))
                        {
                            // Special condition FF's checking
                            QByteArray specialPacket = packet4100;

                            qDebug()<<"Consecutive FF's detected at packet: "+QString::number(packet4100AdxlList.size());
                            writeToNotes("Consecutive FF's detected at packet: " + QString::number(packet4100AdxlList.size()));


                            int fIndex = specialPacket.indexOf(QByteArray::fromHex("FF FF FF FF FF FF"));
                            qDebug()<<fIndex<<" :fIndex";

                            qDebug()<< "Removing ff bytes count: " << (specialPacket.size() - fIndex) - 5;
                            writeToNotes("Removing ff bytes count: " + QString::number((specialPacket.size() - fIndex) - 5));

                            specialPacket.remove(fIndex,(specialPacket.size() - fIndex) - 5);


                            packet4100AdxlList.append(specialPacket);
                            qDebug()<<specialPacket.toHex(' ').toUpper()<<" :specialPacket";

                            // writeToNotes Log
                            writeToNotes("fIndex (start of FFs): " + QString::number(fIndex));
                            writeToNotes("specialPacket: " + specialPacket.toHex(' ').toUpper());
                        }
                        else
                        {
                            // Normal condition
                            packet4100AdxlList.append(packet4100);
                        }

                        // Extract last 2 bytes before footer as temperature
                        QByteArray tempBytes = packet4100.mid(4100 - 5, 2);
                        packetTemperatureList.append(tempBytes);
                    }
                    else
                    {
                        invalidHeaderCount++;
                    }
                    i += 4100;
                    continue;
                }
                else break;
            }

            // --- Case 3: Packet4100_INCLINOMETER ---
            else if (header == QByteArray::fromHex("EE FF FF"))
            {
                if (i + 4100 <= data.size())
                {
                    QByteArray packet4100 = data.mid(i, 4100);
                    if (packet4100.endsWith(QByteArray::fromHex("FF CC DD")))
                    {
                        packet4100InclList.append(packet4100);
                    }
                    else
                    {
                        invalidHeaderCount++;
                    }
                    i += 4100;
                    continue;
                }
                else break;
            }

            // --- Case 4: Unknown header ---
            else
            {
                // Unknown header found — treat as invalid frame
                int next = qMin(i + 4100, data.size());  // move by full packet size
                QByteArray maybeFooter = data.mid(next - 3, 3);

                qDebug() << "Unknown header" << header.toHex()
                         << "possible footer" << maybeFooter.toHex();

                writeToNotes("Unknown header: " + header.toHex(' ').toUpper() +
                              " | possible footer: " + maybeFooter.toHex(' ').toUpper());

                invalidHeaderCount++;
                i = next;  // skip full 4100 bytes
            }
        }

        // Summary logs
        qDebug() << " Packet32 count:" << packet32List.size();
        qDebug() << " Packet4100 ADXL count:" << packet4100AdxlList.size();
        qDebug() << " Packet4100 Inclinometer count:" << packet4100InclList.size();
        qDebug() << " Temperature samples:" << packetTemperatureList.size();
        qDebug() << " Invalid headers:" << invalidHeaderCount;

        if(packetTemperatureList.size() != 147)
        {
            qDebug()<<"Lesser Adxl/Temperature Packets Detected With Size : "<<packetTemperatureList.size();
            writeToNotes("Lesser Adxl/Temperature Packets Detected With Size : "+QString::number(packetTemperatureList.size()));
            QMessageBox::warning(this,"Packet Missing","Some packets are missing 147/"+QString::number(packetTemperatureList.size()));
        }

        // writeToNotes log
        writeToNotes("Packet32 count: " + QString::number(packet32List.size()));
        writeToNotes("Packet4100 ADXL count: " + QString::number(packet4100AdxlList.size()));
        writeToNotes("Packet4100 Inclinometer count: " + QString::number(packet4100InclList.size()));
        writeToNotes("Temperature samples: " + QString::number(packetTemperatureList.size()));
        writeToNotes("Invalid headers: " + QString::number(invalidHeaderCount));


        //Making Packets
        makePacket32UI(packet32List);
        makePacket4100AdxlTempList(packet4100AdxlList,packetTemperatureList);
        makePacket4100InclList(packet4100InclList);

        if(dlgPlot)
        {
            dlgPlot->close();
            dlgPlot = nullptr;
        }

        QDialog *excelSavingDialog = createPleaseWaitDialog("⌛ Please Wait, Data Saving ...");

        saveAllSensorDataToExcel(
            finalAdxlIndex, finalXAdxl, finalYAdxl, finalZAdxl,
            finalTempIndex, finalTemperature,
            finalInclIndex, finalInclX, finalInclY
        );

        if(excelSavingDialog)
        {
            excelSavingDialog->close();
            excelSavingDialog = nullptr;
        }

    }
    // Get Event Data Command Nack Condition mdgId 0x01
    else if(data.startsWith(QByteArray::fromHex("53 54 45 FF")))
    {
        QMessageBox::warning(this,"Error","Invalid Event Id");
        writeToNotes(" ### Invalid Event Id ###");
    }

    // Start Log Initial Command msgId 0x02
    else if(data.startsWith(QByteArray::fromHex("54 53 41 43 4B")))
    {
        dlg = createPleaseWaitDialog("⌛ Please Wait Data Logging ...",10);
        QTimer::singleShot(12000,[this](){
            if(dlg)
            {
                dlg->close();
                dlg = nullptr;
                QMessageBox::critical(this,"Failed","Failed To Log Data !");
            }
        });
    }

    // Start Log End Initial Command msgId 0x02
    else if(data.startsWith(QByteArray::fromHex("54 53 50")))
    {
        if(dlg)
        {
            dlg->close();
            dlg = nullptr;
            QMessageBox::information(this,"Success","Successfully data logged !");
        }
    }

    // Get Log Events Command msgId 0x03
    else if (data.startsWith(QByteArray::fromHex("AA BB")))
    {
        QByteArray logData = data;
        int packetSize = 32;
        int totalPackets = logData.size() / packetSize;

        // --- Clear table first ---
        ui->tableWidget_getLogEvents->clear();
        ui->tableWidget_getLogEvents->setRowCount(0);
        ui->tableWidget_getLogEvents->setColumnCount(3);
        ui->tableWidget_getLogEvents->setHorizontalHeaderLabels(QStringList() << "Event ID" << "Start Time and Date" << "End Time and Date");

        int validCount = 0;

        for (int i = 0; i < totalPackets; ++i)
        {
            QByteArray packet = logData.mid(i * packetSize, packetSize);

            // Stop processing if this packet is all FFs (padding)
            if (packet.count(char(0xFF)) == packetSize)
                break;

            // --- Check header ---
            if (!packet.startsWith(QByteArray::fromHex("AA BB")))
                continue;

            // --- Extract Event ID ---
            quint8 msb = static_cast<quint8>(packet[2]);
            quint8 lsb = static_cast<quint8>(packet[3]);
            quint16 eventId = (msb << 8) | lsb;

            // --- Extract Start Time (6 bytes: hh mm ss dd mm yy) ---
            QByteArray startTimeBytes = packet.mid(20, 6);
            QStringList startParts;
            for (auto b : startTimeBytes)
                startParts.append(QString::number(static_cast<quint8>(b)).rightJustified(2, '0'));

            QString formattedStart = QString("%1:%2:%3 %4/%5/%6")
                .arg(startParts[0]).arg(startParts[1]).arg(startParts[2])
                .arg(startParts[3]).arg(startParts[4]).arg(startParts[5]);

            // --- Extract End Time (6 bytes: hh mm ss dd mm yy) ---
            QByteArray endTimeBytes = packet.mid(26, 6);
            QStringList endParts;
            for (auto b : endTimeBytes)
                endParts.append(QString::number(static_cast<quint8>(b)).rightJustified(2, '0'));

            QString formattedEnd = QString("%1:%2:%3 %4/%5/%6")
                .arg(endParts[0]).arg(endParts[1]).arg(endParts[2])
                .arg(endParts[3]).arg(endParts[4]).arg(endParts[5]);

            // --- Insert into Table ---
            int row = ui->tableWidget_getLogEvents->rowCount();
            ui->tableWidget_getLogEvents->insertRow(row);
            ui->tableWidget_getLogEvents->setItem(row, 0, new QTableWidgetItem(QString::number(eventId)));
            ui->tableWidget_getLogEvents->setItem(row, 1, new QTableWidgetItem(formattedStart));
            ui->tableWidget_getLogEvents->setItem(row, 2, new QTableWidgetItem(formattedEnd));

            validCount++;
        }

        // --- Count trailing FFs ---
        int ffCount = 0;
        for (int i = validCount * packetSize; i < logData.size(); ++i)
        {
            if (static_cast<quint8>(logData[i]) == 0xFF)
                ffCount++;
        }

        qDebug() << "Total Packets Parsed:" << validCount;
        qDebug() << "Trailing FF bytes count:" << ffCount;

        writeToNotes("Total Packets Parsed:"+QString::number(validCount));
        writeToNotes("Trailing FF bytes count:"+QString::number(ffCount));
    }

    // Stop Plot Command msgId 0x04
    else if(data == QByteArray::fromHex("53 54 46"))
    {
        QDialog *excelSavingDialog = createPleaseWaitDialog("⌛ Please Wait, Data Saving ...");

        saveAllSensorDataToExcel(
            finalAdxlIndex, finalXAdxl, finalYAdxl, finalZAdxl,
            finalTempIndex, finalTemperature,
            finalInclIndex, finalInclX, finalInclY
        );

        if(excelSavingDialog)
        {
            excelSavingDialog->close();
            excelSavingDialog = nullptr;
        }

        QMessageBox::information(this,"Success","Plot stopped/saved successfully !");

    }

}



void MainWindow::on_pushButton_calibrateScreen_clicked()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QSize res = screen->size();

    QSettings settings("settings.ini", QSettings::IniFormat);

    QMessageBox::StandardButton choice = QMessageBox::question(
        this, "Calibrate Screen",
        "Do you want to enter custom screen details (width, height, diagonal) or reset to system default?",
        QMessageBox::Yes | QMessageBox::No);

    if (choice == QMessageBox::No) {
        // Reset to default
        settings.remove("Display/calibratedDPI");
        settings.remove("Display/width");
        settings.remove("Display/height");
        settings.remove("Display/diagonal");
        QMessageBox::information(this, "Calibration Removed",
                                 "Screen DPI reset to system default.\nRestart app to apply.");
        return;
    }

    // Custom input
    bool ok = false;
    int width = QInputDialog::getInt(this, "Screen Width",
                                     "Enter screen width (pixels):",
                                     res.width(), 100, 10000, 1, &ok);
    if (!ok) return;

    int height = QInputDialog::getInt(this, "Screen Height",
                                      "Enter screen height (pixels):",
                                      res.height(), 100, 10000, 1, &ok);
    if (!ok) return;

    double diagonalInches = QInputDialog::getDouble(
        this, "Screen Diagonal",
        "Enter screen diagonal size (in inches):",
        settings.value("Display/diagonal", 14.0).toDouble(), 3.0, 100.0, 1, &ok);
    if (!ok) return;

    // Calculate DPI
    double ppi = std::sqrt(width * width + height * height) / diagonalInches;

    // Save all values
    settings.setValue("Display/width", width);
    settings.setValue("Display/height", height);
    settings.setValue("Display/diagonal", diagonalInches);
    settings.setValue("Display/calibratedDPI", static_cast<int>(ppi));

    QMessageBox::information(this, "Calibration Done",
                             QString("Resolution: %1 x %2\nDiagonal: %3 in\nDPI set to %4.\nRestart app to apply.")
                             .arg(width).arg(height).arg(diagonalInches).arg(ppi, 0, 'f', 2));
}


void MainWindow::on_pushButton_getEventData_clicked()
{
    bool ok;
    QString text = ui->lineEdit_enterEventId->text().trimmed();
    int eventId = text.toInt(&ok);

    if(!ok || eventId < 0 || eventId > 65535)
    {
        QMessageBox::warning(this, "Error", "Please enter a valid Event ID (0–65535)");
        return;
    }

    initializeAllPlots();

    initializeSensorVectors();

    // Start the timeout timer
    responseTimer->start(2000); // 2 Sec timer

    QByteArray command;

    command.append(0x53); //1
    command.append(0x54); //2
    command.append(0x45); //3

    // Split eventId into 2 bytes (big-endian)
    command.append(static_cast<quint8>((eventId >> 8) & 0xFF)); // High byte 4
    command.append(static_cast<quint8>(eventId & 0xFF)); // Low byte 5

    command.append(0xFF); //6
    command.append(0xFF); //7


    qDebug() << "Get Event Data cmd sent : " + hexBytes(command);
    writeToNotes("Get Event Data cmd sent : " + hexBytes(command));


    emit sendMsgId(0x01);

    dlgPlot = createPleaseWaitDialog("⌛ Please Wait Loading Plot !!!");

    // Automatic dlgPlot close after 10 Sec watchdog timer
    QTimer::singleShot(10000,[this](){
        if(dlgPlot)
        {
            dlgPlot->close();
            dlgPlot = nullptr;
            QMessageBox::warning(this,"Error","Data Interrupted");
        }

    });

    serialObj->writeData(command);
}

void MainWindow::on_pushButton_startLog_clicked()
{
    // Start the timeout timer
    responseTimer->start(2000); // 2 Sec timer

    QByteArray command;

    command.append(0x53); //1
    command.append(0x54); //2
    command.append(0x42); //3


    qDebug() << "Start Log cmd sent : " + hexBytes(command);
    writeToNotes("Start Log cmd sent : " + hexBytes(command));


    emit sendMsgId(0x02);
    serialObj->writeData(command);
}

void MainWindow::on_pushButton_getLogEvents_clicked()
{
    // Start the timeout timer
    responseTimer->start(2000); // 2 Sec timer

    QByteArray command;

    command.append(0x53); //1
    command.append(0x54); //2
    command.append(0x43); //3


    qDebug() << "Get Log Events cmd sent : " + hexBytes(command);
    writeToNotes("Get Log Events cmd sent : " + hexBytes(command));


    emit sendMsgId(0x03);
    serialObj->writeData(command);
}

void MainWindow::on_pushButton_stopPlot_clicked()
{
    // Start the timeout timer
    responseTimer->start(2000); // 2 Sec timer

    QByteArray command;

    command.append(0x53); //1
    command.append(0x54); //2
    command.append(0x46); //3


    qDebug() << "Stop Plot cmd sent : " + hexBytes(command);
    writeToNotes("Stop Plot Events cmd sent : " + hexBytes(command));


    emit sendMsgId(0x04);
    serialObj->writeData(command);
}

void MainWindow::on_pushButton_enlargePlot_clicked()
{
    QString selected = ui->comboBox_enlargePlot->currentText();
    QCustomPlot *plot = nullptr;

    if (selected == "ADXL_X")          plot = ui->customPlot_adxl_x;
    else if (selected == "ADXL_Y")     plot = ui->customPlot_adxl_y;
    else if (selected == "ADXL_Z")     plot = ui->customPlot_adxl_z;
    else if (selected == "Temperature") plot = ui->customPlot_temperature;
    else if (selected == "Inclinometer_X") plot = ui->customPlot_inclinometer_x;
    else if (selected == "Inclinometer_Y") plot = ui->customPlot_inclinometer_y;

    if (!plot)
    {
        QMessageBox::warning(this, "Warning", "Please select a valid plot to enlarge!");
        return;
    }

    // Create enlargePlot dialog and load the selected plot into it
    enlargePlot *dlg = new enlargePlot(this);
    dlg->loadPlot(plot);
    dlg->setModal(false);
    dlg->show();
}


void MainWindow::on_pushButton_fitToScreen_clicked()
{
    // Collect all plots
    QList<QCustomPlot*> allPlots = {
        ui->customPlot_adxl_x,
        ui->customPlot_adxl_y,
        ui->customPlot_adxl_z,
        ui->customPlot_inclinometer_x,
        ui->customPlot_inclinometer_y,
        ui->customPlot_temperature
    };

    // Iterate through each and fit accordingly
    for (QCustomPlot *plot : allPlots)
    {
        if (!plot) continue;

        bool hasData = false;
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
            //  Auto-fit to existing data
            plot->rescaleAxes(true);

            // Small padding for aesthetics
            plot->xAxis->scaleRange(1.05, plot->xAxis->range().center());
            plot->yAxis->scaleRange(1.05, plot->yAxis->range().center());
        }
        else
        {
            //  No data — reset to initial default view
            plot->xAxis->setRange(0, 100);
            plot->yAxis->setRange(-5, 5);
        }

        plot->replot();
    }

    qDebug() << "All plots adjusted — data-fitted if available, otherwise reset to default view.";
    writeToNotes("All plots adjusted — data-fitted if available, otherwise reset to default view.");
}


void MainWindow::on_pushButton_saveLogPlots_clicked()
{
    // Prepare default filename (EventsLogData_yyyyMMdd_HHmmss.xlsx)
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString defaultFileName = QString("EventsLogData_%1.xlsx").arg(timestamp);

    // Get desktop path as default location
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString defaultFullPath = desktopPath + "/" + defaultFileName;

    // Ask user where to save (with default pre-filled)
    QString selectedFile = QFileDialog::getSaveFileName(
        this,
        tr("Save Log Data"),
        defaultFullPath,
        tr("Excel Files (*.xlsx)")
    );

    if (selectedFile.isEmpty())
        return; // user cancelled

    // Ensure file ends with .xlsx
    if (!selectedFile.endsWith(".xlsx", Qt::CaseInsensitive))
        selectedFile += ".xlsx";

    // Create an Excel document
    QXlsx::Document xlsx;

    // Header format (just bold)
    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);

    // Write headers
    xlsx.write(1, 1, "Event ID", headerFormat);
    xlsx.write(1, 2, "Start Time and Date", headerFormat);
    xlsx.write(1, 3, "End Time and Date", headerFormat);

    // Write table data
    int rowCount = ui->tableWidget_getLogEvents->rowCount();
    int colCount = ui->tableWidget_getLogEvents->columnCount();

    for (int r = 0; r < rowCount; ++r)
    {
        for (int c = 0; c < colCount; ++c)
        {
            QTableWidgetItem *item = ui->tableWidget_getLogEvents->item(r, c);
            if (!item) continue;

            if (c == 0) // Event ID column should be integer
            {
                bool ok;
                int eventId = item->text().toInt(&ok);
                if (ok)
                    xlsx.write(r + 2, c + 1, eventId);
                else
                    xlsx.write(r + 2, c + 1, item->text());
            }
            else
            {
                xlsx.write(r + 2, c + 1, item->text());
            }
        }
    }

    // Auto fit columns
    xlsx.currentWorksheet()->setColumnWidth(2, 3, 20);

    // Save file
    if (xlsx.saveAs(selectedFile))
    {
        QMessageBox::information(this, "Success",
                                 " Log data saved successfully at:\n" + selectedFile);
    }
    else
    {
        QMessageBox::critical(this, "Error",
                              " Failed to save log data.\nPlease check permissions or path.");
    }
}



void MainWindow::on_pushButton_clearLogPlots_clicked()
{
    ui->tableWidget_getLogEvents->setRowCount(0);
    writeToNotes("table data cleared.");
}

void MainWindow::on_pushButton_clearPlots_clicked()
{
    // Clear all plot graphs
     QList<QCustomPlot*> allPlots = {
         ui->customPlot_adxl_x,
         ui->customPlot_adxl_y,
         ui->customPlot_adxl_z,
         ui->customPlot_inclinometer_x,
         ui->customPlot_inclinometer_y,
         ui->customPlot_temperature
     };

     for (QCustomPlot *plot : allPlots)
     {
         if (plot) {
             for (int i = 0; i < plot->graphCount(); ++i)
                 plot->graph(i)->data()->clear();
             plot->replot();
         }
     }

     writeToNotes("All log plots are cleared.");
}
