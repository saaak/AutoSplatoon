#include "autosplatoon.h"
#include "ui_autosplatoon.h"
#include <QFile>
#include <QMessageBox>
#include <QRadioButton>
#include <QSerialPortInfo>
#include <QTextStream>
#include <QFileDialog>
#include <QImage>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QtDebug>
#include <QThread>
#include <QProcess>
#include <QElapsedTimer>
#include <QQueue>
#include <climits>

AutoSplatoon::AutoSplatoon(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::AutoSplatoon)
{
    ui->setupUi(this);
    fillSerialPorts();

    signalMapper = new QSignalMapper(this);

    setWindowFlags(windowFlags()&~Qt::WindowMaximizeButtonHint);

    setFixedSize(this->width(),this->height());

    setWindowTitle("AutoSplatoon");

    ui->intervalBox->setValue(70);
    ui->rowBox->setValue(0);
    ui->columnBox->setValue(0);
    threshold = 128;
    ui->thresholdBox->setValue(threshold);
    ui->thresholdBox->setEnabled(false);

    manControl2 = new ManualControl();
    manControl2->setAttribute(Qt::WA_DeleteOnClose);
    connect(manControl2, SIGNAL(buttonAction(quint64, bool)), this, SLOT(recieveButtonAction(quint64, bool)));
    connect(manControl2, SIGNAL(manControlDeletedSignal()), this, SLOT(manControlDeletedSignal()));
}

AutoSplatoon::~AutoSplatoon()
{
    delete ui;
}

void AutoSplatoon::fillSerialPorts()
{
    QList<QSerialPortInfo> serialList = QSerialPortInfo::availablePorts();
    ui->serialPortsBox->clear();
    serialPorts.clear();
    for (const QSerialPortInfo& serialPortInfo : serialList) {
        serialPorts.append(serialPortInfo.systemLocation());
    }
    ui->serialPortsBox->addItems(serialPorts);
}

void AutoSplatoon::on_serialRefreshButton_clicked()
{
    fillSerialPorts();
}

void AutoSplatoon::manControlDeletedSignal()
{
    manControlDeleted = true;
}

void AutoSplatoon::on_uploadButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open Image"),QDir::homePath(),tr("(*.bmp)"));
    QImage tmp = QImage(fileName);
    if(tmp.width() == 320 && tmp.height() == 120)
    {
        haltFlag = true;
        //startFlag = false;
        pauseFlag = false;
        image = QImage(fileName);
        mask = buildMask(image, ui->thresholdBox->value());
        renderMaskPreview();
        ui->startButton->setEnabled(true);
        ui->label->setEnabled(true);
        ui->label_2->setEnabled(true);
        ui->label_4->setEnabled(true);
        ui->intervalBox->setEnabled(true);
        ui->rowBox->setEnabled(true);
        ui->columnBox->setEnabled(true);
        ui->thresholdBox->setEnabled(true);
        ui->label_threshold->setEnabled(true);
    }
}

void AutoSplatoon::recieveButtonAction(quint64 action, bool temporary)
{
    emit sendButtonAction(action, temporary);
}

void AutoSplatoon::handleSerialStatus(uint8_t status)
{
    if (status == CONNECTING) {
        ui->serialStatusLabel->setText("连接中");
        ui->serialPortsBox->setEnabled(false);
        ui->serialRefreshButton->setEnabled(false);
        ui->uploadButton->setEnabled(false);
        //ui->forceVanillaConnection->setEnabled(false);
        ui->serialConnectButton->setText("断开连接");
    } else if (status == CONNECTED_OK) {
        ui->serialStatusLabel->setText("已连接");
        ui->uploadButton->setEnabled(true);
        ui->flashButton->setEnabled(false);
        //connect(this, SIGNAL(), serialController, SLOT(recieveButtonAction(quint64, bool)));
    } else if (status == CHOCO_SYNCED_JC_L) {
        ui->serialStatusLabel->setText("Connected as Left JoyCon!");
    } else if (status == CHOCO_SYNCED_JC_R) {
        ui->serialStatusLabel->setText("Connected as Right JoyCon!");
    } else if (status == CHOCO_SYNCED_PRO) {
        ui->serialStatusLabel->setText("已连接");
        ui->uploadButton->setEnabled(true);
        ui->flashButton->setEnabled(false);
    } else if (status == CONNECTION_FAILED) {
        ui->serialStatusLabel->setText("连接失败");
    } else {
        ui->serialStatusLabel->setText("断连");
        ui->serialPortsBox->setEnabled(true);
        ui->serialRefreshButton->setEnabled(true);
        ui->uploadButton->setEnabled(false);
        ui->flashButton->setEnabled(true);
        ui->serialConnectButton->setText("连接");
        ui->serialConnectButton->setEnabled(true);
        ui->startButton->setEnabled(false);
        ui->pauseButton->setEnabled(false);
        ui->haltButton->setEnabled(false);
        //ui->forceVanillaConnection->setEnabled(true);
    }
}

void AutoSplatoon::createSerial()
{
    if (ui->serialPortsBox->currentText().isEmpty() == false) {
        if (!ui->serialPortsBox->isEnabled()) {
            ui->serialConnectButton->setEnabled(false);
            //ui->forceVanillaConnection->setEnabled(false);
            serialController->deleteLater();
        } else {
            serialController = new SerialController(this);
            connect(serialController, SIGNAL(reportSerialStatus(uint8_t)), this, SLOT(handleSerialStatus(uint8_t)));
            serialController->openAndSync(ui->serialPortsBox->currentText());
        }
    }
}

void AutoSplatoon::on_serialConnectButton_clicked()
{
    createSerial();
}

QProcess process;
QString output;

void AutoSplatoon::on_readoutput()
{
    output.append(QString(process.readAllStandardOutput().data()));
}

void AutoSplatoon::on_flashButton_clicked()
{
    ui->serialConnectButton->setEnabled(false);
    ui->flashButton->setEnabled(false);
    ui->serialPortsBox->setEnabled(false);
    ui->serialRefreshButton->setEnabled(false);
    ui->manualButton->setEnabled(false);
    ui->serialStatusLabel->setText("烧录中");

    QElapsedTimer timer;
    timer.start();
    QString cmd = QApplication::applicationDirPath();
    cmd += "/esptool.exe";
    qDebug() << cmd;
    QStringList arg;
    arg << "--baud";arg << "230400";arg << "write_flash";arg << "0x0";arg << QApplication::applicationDirPath()+"/PRO-UART0.bin";
//    arg << "--baud";arg << "230400";arg << "write_flash";
//    arg << "0x1000";arg << QApplication::applicationDirPath()+"/bootloader.bin";
//    arg << "0x10000";arg << QApplication::applicationDirPath()+"/firmware.bin";
//    arg << "0x8000";arg << QApplication::applicationDirPath()+"/partition-table.bin";

    process.start(cmd, arg);
    connect(&process , SIGNAL(readyReadStandardOutput()) , this , SLOT(on_readoutput()));
    QEventLoop loop;
    connect(&process,SIGNAL(finished(int,QProcess::ExitStatus)),&loop,SLOT(quit()));
    loop.exec();
    process.kill();
    qDebug() << output;
    int flag = output.indexOf("100 %");
    if(flag == -1)
        ui->serialStatusLabel->setText("烧录失败");
    else
        ui->serialStatusLabel->setText("烧录成功");
    ui->serialConnectButton->setEnabled(true);
    ui->flashButton->setEnabled(true);
    ui->serialPortsBox->setEnabled(true);
    ui->serialRefreshButton->setEnabled(true);
    ui->manualButton->setEnabled(true);

    output = "";
}

void AutoSplatoon::executeTask()
{
    for(; row < image.height(); row++)
    {
        if(row % 2 == 0)
        {
            for (; column < image.width(); column++)
            {
                if(haltFlag)
                    return;
                while(pauseFlag)
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
                if(qGray(image.pixel(column, row)) < 128)
                    manControl2->sendCommand("A", interval);
                manControl2->sendCommand("Dr", interval);

                ui->rowBox->setValue(row);
                ui->columnBox->setValue(column);
            }
            column -= 1;
        }
        else
        {
            for (; column >= 0; column--)
            {
                if(haltFlag)
                    return;
                while(pauseFlag)
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
                if(qGray(image.pixel(column, row)) < 128)
                    manControl2->sendCommand("A", interval);
                manControl2->sendCommand("Dl", interval);

                ui->rowBox->setValue(row);
                ui->columnBox->setValue(column);
            }
            column += 1;
        }
        manControl2->sendCommand("Dd", interval);
    }
    //startFlag = false;
    on_haltButton_clicked();
}

void AutoSplatoon::on_startButton_clicked()
{
    ui->startButton->setEnabled(false);
    ui->pauseButton->setEnabled(true);
    ui->haltButton->setEnabled(true);
    ui->uploadButton->setEnabled(false);

    ui->label->setEnabled(false);
//    ui->label_2->setEnabled(false);
//    ui->label_4->setEnabled(false);
    ui->label_2->setText("当前行数");
    ui->label_4->setText("当前列数");
    ui->intervalBox->setEnabled(false);
    ui->rowBox->setEnabled(false);
    ui->columnBox->setEnabled(false);

    interval = ui->intervalBox->value();
    row = ui->rowBox->value();
    column = ui->columnBox->value();

    //column = 0; row = 0;
    //startFlag = true;
    haltFlag = false;

    executeTaskNearestNeighbor();
}

void AutoSplatoon::on_pauseButton_clicked()
{
    if(!pauseFlag)
    {
        ui->pauseButton->setText("继续");
        pauseFlag = true;
    }
    else
    {
        pauseFlag = false;
        ui->pauseButton->setText("暂停");
    }
}

void AutoSplatoon::on_haltButton_clicked()
{
    ui->pauseButton->setEnabled(false);
    ui->haltButton->setEnabled(false);
    ui->uploadButton->setEnabled(true);
    ui->startButton->setEnabled(false);
    ui->label->setEnabled(false);
    ui->label_2->setEnabled(false);
    ui->label_4->setEnabled(false);
    ui->label_threshold->setEnabled(false);
    ui->intervalBox->setEnabled(false);
    ui->rowBox->setEnabled(false);
    ui->columnBox->setEnabled(false);
    ui->thresholdBox->setEnabled(false);
    ui->pauseButton->setText("暂停");
    //startFlag = false;
    pauseFlag = false;
    haltFlag = true;
    column = 0; row = 0;

    QGraphicsScene *scene = new QGraphicsScene;
    scene = NULL;
    ui->graphicsView->setScene(scene);
    ui->graphicsView->show();
}

void AutoSplatoon::on_manualButton_clicked()
{
    if (manControlDeleted)
    {
        manControlDeleted = false;
        manControl1 = new ManualControl();
        manControl1->setAttribute(Qt::WA_DeleteOnClose);
        manControl1->show();
        connect(manControl1, SIGNAL(buttonAction(quint64, bool)), this, SLOT(recieveButtonAction(quint64, bool)));
        connect(manControl1, SIGNAL(manControlDeletedSignal()), this, SLOT(manControlDeletedSignal()));
    }
}
void AutoSplatoon::renderMaskPreview()
{
    if (image.isNull()) return;
    QImage preview(image.width(), image.height(), QImage::Format_RGB32);
    for (int r = 0; r < image.height(); r++) {
        for (int c = 0; c < image.width(); c++) {
            bool on = (r < mask.size() && c < mask[r].size()) ? mask[r][c] : false;
            preview.setPixel(c, r, on ? qRgb(0,0,0) : qRgb(255,255,255));
        }
    }
    QGraphicsScene *scene = new QGraphicsScene;
    scene->addPixmap(QPixmap::fromImage(preview));
    ui->graphicsView->setScene(scene);
    ui->graphicsView->show();
}

void AutoSplatoon::on_thresholdBox_valueChanged(int value)
{
    threshold = value;
    if (!image.isNull()) {
        mask = buildMask(image, threshold);
        renderMaskPreview();
    }
}

QVector<QVector<bool>> AutoSplatoon::buildMask(const QImage& img, int threshold)
{
    QVector<QVector<bool>> mask(img.height());
    for (int r = 0; r < img.height(); r++) {
        mask[r].resize(img.width());
        for (int c = 0; c < img.width(); c++) {
            mask[r][c] = qGray(img.pixel(c, r)) < threshold;
        }
    }
    return mask;
}

QVector<AutoSplatoon::Component> AutoSplatoon::findComponents(const QVector<QVector<bool>>& mask)
{
    int H = mask.size();
    int W = H ? mask[0].size() : 0;
    QVector<QVector<bool>> vis(H, QVector<bool>(W, false));
    QVector<Component> comps;
    QQueue<QPoint> q;
    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            if (!mask[r][c] || vis[r][c]) continue;
            Component comp;
            comp.minRow = r;
            comp.maxRow = r;
            comp.minCol = c;
            comp.maxCol = c;
            vis[r][c] = true;
            q.enqueue(QPoint(c, r));
            while (!q.isEmpty()) {
                QPoint p = q.dequeue();
                comp.points.append(p);
                int rr = p.y();
                int cc = p.x();
                if (rr < comp.minRow) comp.minRow = rr;
                if (rr > comp.maxRow) comp.maxRow = rr;
                if (cc < comp.minCol) comp.minCol = cc;
                if (cc > comp.maxCol) comp.maxCol = cc;
                const int dr[4] = {1,-1,0,0};
                const int dc[4] = {0,0,1,-1};
                for (int k = 0; k < 4; k++) {
                    int nr = rr + dr[k];
                    int nc = cc + dc[k];
                    if (nr >= 0 && nr < H && nc >= 0 && nc < W && mask[nr][nc] && !vis[nr][nc]) {
                        vis[nr][nc] = true;
                        q.enqueue(QPoint(nc, nr));
                    }
                }
            }
            comp.entry = QPoint(comp.minCol, comp.minRow);
            comps.append(comp);
        }
    }
    return comps;
}

void AutoSplatoon::travelTo(int targetRow, int targetCol, int intervalMs)
{
    while (!haltFlag && row < targetRow) {
        while (pauseFlag) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        manControl2->sendCommand("Dd", intervalMs);
        row += 1;
        ui->rowBox->setValue(row);
        ui->columnBox->setValue(column);
    }
    while (!haltFlag && row > targetRow) {
        while (pauseFlag) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        manControl2->sendCommand("Du", intervalMs);
        row -= 1;
        ui->rowBox->setValue(row);
        ui->columnBox->setValue(column);
    }
    while (!haltFlag && column < targetCol) {
        while (pauseFlag) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        manControl2->sendCommand("Dr", intervalMs);
        column += 1;
        ui->rowBox->setValue(row);
        ui->columnBox->setValue(column);
    }
    while (!haltFlag && column > targetCol) {
        while (pauseFlag) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        manControl2->sendCommand("Dl", intervalMs);
        column -= 1;
        ui->rowBox->setValue(row);
        ui->columnBox->setValue(column);
    }
}

void AutoSplatoon::drawComponent(const Component& comp, const QVector<QVector<bool>>& mask, int intervalMs)
{
    travelTo(comp.entry.y(), comp.entry.x(), intervalMs);
    if (haltFlag) return;
    if (mask[comp.entry.y()][comp.entry.x()]) {
        manControl2->sendCommand("A", intervalMs);
    }
    QVector<QVector<bool>> visited(mask.size(), QVector<bool>(mask.isEmpty() ? 0 : mask[0].size(), false));
    visited[comp.entry.y()][comp.entry.x()] = true;
    QVector<QPoint> stack;
    QPoint cur(comp.entry.x(), comp.entry.y());
    auto inComp = [&](int rr, int cc){
        return rr >= comp.minRow && rr <= comp.maxRow && cc >= comp.minCol && cc <= comp.maxCol;
    };
    for (;;) {
        if (haltFlag) break;
        while (pauseFlag) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        const int dr[4] = {0, 0, -1, 1};
        const int dc[4] = {1, -1, 0, 0};
        bool moved = false;
        for (int k = 0; k < 4; k++) {
            int nr = cur.y() + dr[k];
            int nc = cur.x() + dc[k];
            if (!inComp(nr, nc)) continue;
            if (!mask[nr][nc]) continue;
            if (visited[nr][nc]) continue;
            stack.push_back(cur);
            if (dc[k] == 1) {
                manControl2->sendCommand("Dr", intervalMs);
                column += 1;
            } else if (dc[k] == -1) {
                manControl2->sendCommand("Dl", intervalMs);
                column -= 1;
            } else if (dr[k] == 1) {
                manControl2->sendCommand("Dd", intervalMs);
                row += 1;
            } else if (dr[k] == -1) {
                manControl2->sendCommand("Du", intervalMs);
                row -= 1;
            }
            ui->rowBox->setValue(row);
            ui->columnBox->setValue(column);
            cur = QPoint(nc, nr);
            visited[nr][nc] = true;
            manControl2->sendCommand("A", intervalMs);
            moved = true;
            break;
        }
        if (!moved) {
            if (stack.isEmpty()) break;
            QPoint back = stack.back();
            stack.pop_back();
            int rr = back.y() - cur.y();
            int cc = back.x() - cur.x();
            if (cc == 1) {
                manControl2->sendCommand("Dr", intervalMs);
                column += 1;
            } else if (cc == -1) {
                manControl2->sendCommand("Dl", intervalMs);
                column -= 1;
            } else if (rr == 1) {
                manControl2->sendCommand("Dd", intervalMs);
                row += 1;
            } else if (rr == -1) {
                manControl2->sendCommand("Du", intervalMs);
                row -= 1;
            }
            ui->rowBox->setValue(row);
            ui->columnBox->setValue(column);
            cur = back;
        }
    }
}

void AutoSplatoon::executeTaskNearestNeighbor()
{
    QVector<QVector<bool>> localMask;
    if (!mask.isEmpty()) localMask = mask;
    else localMask = buildMask(image, ui->thresholdBox->value());
    QVector<Component> comps = findComponents(localMask);
    QVector<bool> done(comps.size(), false);
    for (;;) {
        int idx = -1;
        int bestDist = INT_MAX;
        for (int i = 0; i < comps.size(); i++) {
            if (done[i]) continue;
            const Component& comp = comps[i];
            int dist = INT_MAX;
            for (const QPoint& p : comp.points) {
                int d = qAbs(p.y() - row) + qAbs(p.x() - column);
                if (d < dist) dist = d;
            }
            if (dist < bestDist) {
                bestDist = dist;
                idx = i;
            }
        }
        if (idx == -1) break;
        drawComponentDFS(comps[idx], localMask, interval);
        done[idx] = true;
        if (haltFlag) break;
    }
    on_haltButton_clicked();
}

void AutoSplatoon::drawComponentDFS(const Component& comp, const QVector<QVector<bool>>& mask, int intervalMs)
{
    drawComponent(comp, mask, intervalMs);
}
