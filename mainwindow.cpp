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

    initializeAllPlots();

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

QDialog* MainWindow::createPleaseWaitDialog(const QString &text)
{
    QDialog *dlg = new QDialog(this);  // Create a QDialog with MainWindow as parent
    dlg->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    dlg->setAttribute(Qt::WA_DeleteOnClose);  // Auto-delete when closed
    dlg->setModal(true);  // Prevent interaction with the rest of the UI

    dlg->setStyleSheet(R"(
        QDialog { background-color: #f8f8f8; border: 2px solid #0078D7; border-radius: 8px; }
        QLabel { font-size: 16px; padding: 20px; }
    )");

    QVBoxLayout *layout = new QVBoxLayout(dlg);  // Layout for vertical arrangement
    layout->addWidget(new QLabel(text));         // Message shown in the dialog

    dlg->setLayout(layout);         // Apply layout
    dlg->adjustSize();              // Resize dialog based on content
    dlg->setFixedSize(dlg->sizeHint());  // Fix size to avoid resizing by user

    dlg->show();                    // Show the dialog
    QApplication::processEvents(); // Force the event loop to process so it appears immediately

    return dlg;  // Return the pointer so you can manually close/delete it later

//    Using of this function
//    QDialog *dlg = createPleaseWaitDialog("⏳ Please Wait ...");
    //    dlg->close();
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

    //  Graph color themes — bright on dark
    QColor adxlColors[] = {
        QColor(0, 255, 255),   // Cyan
        QColor(0, 180, 255),   // Electric Blue
        QColor(0, 255, 100)    // Neon Green
    };
    QColor inclinometerColors[] = {
        QColor(255, 80, 180),  // Magenta
        QColor(255, 200, 80)   // Amber
    };
    QColor tempColor(255, 70, 70); // Bright red for temperature

    //  ADXL X, Y, Z — voltages vs samples (1 unit = 100 ms)
    setupPlot(ui->customPlot_adxl_x, "ADXL X Time (1 = 100 ms)", "Voltage (V)");
    setupPlot(ui->customPlot_adxl_y, "ADXL Y Time (1 = 100 ms)", "Voltage (V)");
    setupPlot(ui->customPlot_adxl_z, "ADXL Z Time (1 = 100 ms)", "Voltage (V)");

    ui->customPlot_adxl_x->addGraph(); ui->customPlot_adxl_x->graph(0)->setPen(QPen(adxlColors[0], 2));
    ui->customPlot_adxl_y->addGraph(); ui->customPlot_adxl_y->graph(0)->setPen(QPen(adxlColors[1], 2));
    ui->customPlot_adxl_z->addGraph(); ui->customPlot_adxl_z->graph(0)->setPen(QPen(adxlColors[2], 2));

    //  Inclinometer X, Y — degrees vs time (1 unit = 1 ms)
    setupPlot(ui->customPlot_inclinometer_x, "Inclinometer Time X (ms)", "Degrees (°)");
    setupPlot(ui->customPlot_inclinometer_y, "Inclinometer Time Y (ms)", "Degrees (°)");

    ui->customPlot_inclinometer_x->addGraph(); ui->customPlot_inclinometer_x->graph(0)->setPen(QPen(inclinometerColors[0], 2));
    ui->customPlot_inclinometer_y->addGraph(); ui->customPlot_inclinometer_y->graph(0)->setPen(QPen(inclinometerColors[1], 2));

    //  Temperature — Celsius vs samples
    setupPlot(ui->customPlot_temparature, "Samples", "Temperature (°C)");
    ui->customPlot_temparature->addGraph(); ui->customPlot_temparature->graph(0)->setPen(QPen(tempColor, 2));

    // Axis ranges start clean
    for (auto plot : {ui->customPlot_adxl_x, ui->customPlot_adxl_y, ui->customPlot_adxl_z,
                      ui->customPlot_inclinometer_x, ui->customPlot_inclinometer_y,
                      ui->customPlot_temparature})
    {
        plot->xAxis->setRange(0, 100);
        plot->yAxis->setRange(-5, 5);
        plot->replot();
    }
}




void MainWindow::portStatus(const QString &data)
{
    if(data.startsWith("Serial object is not initialized/port not selected"))
    {
        QMessageBox::critical(this,"Port Error","Please Select Port Using Above Dropdown");
    }

    if(data.startsWith("Serial port ") && data.endsWith(" opened successfully at baud rate 921600"))
    {
        QMessageBox::information(this,"Success",data);
    }

    if(data.startsWith("Failed to open port"))
    {
        QMessageBox::critical(this,"Error",data);
    }

    ui->textEdit_rawBytes->append(data);
}

void MainWindow::showGuiData(const QByteArray &byteArrayData)
{
    QByteArray data = byteArrayData;

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
                            qDebug()<<fIndex;

                            int footerIndex = specialPacket.indexOf(QByteArray::fromHex("FF EE FF"));
                            qDebug()<<footerIndex;

                            specialPacket.remove(fIndex,footerIndex - 2);
                            packet4100AdxlList.append(specialPacket);
                            qDebug()<<specialPacket.toHex(' ').toUpper()<<" :specialPacket";

                            // writeToNotes Log
                            writeToNotes("fIndex (start of FFs): " + QString::number(fIndex));
                            writeToNotes("footerIndex (footer start): " + QString::number(footerIndex));
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

        qDebug()<<" Temparature samples list at 146: "<<packetTemperatureList.at(146).toHex(' ').toUpper();

        // writeToNotes log
        writeToNotes("Packet32 count: " + QString::number(packet32List.size()));
        writeToNotes("Packet4100 ADXL count: " + QString::number(packet4100AdxlList.size()));
        writeToNotes("Packet4100 Inclinometer count: " + QString::number(packet4100InclList.size()));
        writeToNotes("Temperature samples: " + QString::number(packetTemperatureList.size()));
        writeToNotes("Invalid headers: " + QString::number(invalidHeaderCount));

        writeToNotes("Temperature sample[146]: " + packetTemperatureList.at(146).toHex(' ').toUpper());
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
    serialObj->writeData(command);
}
