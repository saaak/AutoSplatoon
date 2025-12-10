#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QPoint>

#include "commonNames.h"
#include "inputemulator.h"
#include "manualcontrol.h"
#include "serialcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class AutoSplatoon;
}
QT_END_NAMESPACE

class AutoSplatoon : public QMainWindow {
    Q_OBJECT

public:
    AutoSplatoon(QWidget* parent = nullptr);
    ~AutoSplatoon();

private slots:
    void on_serialRefreshButton_clicked();

    void on_uploadButton_clicked();

    void on_serialConnectButton_clicked();

    void handleSerialStatus(uint8_t status);

    void recieveButtonAction(quint64 action, bool temporary);

    void manControlDeletedSignal();

    void on_flashButton_clicked();

    void on_startButton_clicked();

    void on_pauseButton_clicked();

    void on_haltButton_clicked();

    void executeTask();

    void executeTaskNearestNeighbor();

    void on_manualButton_clicked();

    void on_readoutput();

    void on_thresholdBox_valueChanged(int value);

signals:
    void sendButtonAction(quint64 action, bool temporary);
    void sendNewTimerLength(int newLength);
    void sendCurrentTimer(int current, int max, bool notVotes);

private:
    InputEmulator _inpEmu;
    Ui::AutoSplatoon* ui;
    QStringList serialPorts;
    void fillSerialPorts();
    bool test;
    ManualControl* manControl1 = nullptr;
    ManualControl* manControl2 = nullptr;
    bool manControlDeleted = true;
    SerialController* serialController = nullptr;
    void createSerial();
    QSignalMapper* signalMapper;

    QImage image;
    int interval;
    int column, row;
    bool pauseFlag = false;
    //bool startFlag = false;
    bool haltFlag = true;
    int threshold = 128;
    QVector<QVector<bool>> mask;

    struct Component {
        QVector<QPoint> points;
        int minRow;
        int maxRow;
        int minCol;
        int maxCol;
        QPoint entry;
    };

    QVector<QVector<bool>> buildMask(const QImage& img, int threshold);
    QVector<Component> findComponents(const QVector<QVector<bool>>& mask);
    void travelTo(int targetRow, int targetCol, int intervalMs);
    void drawComponent(const Component& comp, const QVector<QVector<bool>>& mask, int intervalMs);
    void drawComponentDFS(const Component& comp, const QVector<QVector<bool>>& mask, int intervalMs);
    QVector<QString> planTravelToRoute(int& curRow, int& curCol, int targetRow, int targetCol);
    QVector<QString> planComponentDFS(const Component& comp, const QVector<QVector<bool>>& mask, int& curRow, int& curCol);
    QVector<QString> planFullRoute(const QVector<QVector<bool>>& mask);
    void executeRoute(const QVector<QString>& route, int intervalMs);
    void dumpRoute(const QVector<QString>& route);
    void renderMaskPreview();
};
#endif // MAINWINDOW_H
