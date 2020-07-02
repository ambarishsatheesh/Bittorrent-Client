#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QWidget>
#include <QDialog>
#include <QFileDialog>
#include <QPointer>


namespace Ui {
class SettingsDialog;
}

namespace Bittorrent {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QPointer<QWidget> parent = nullptr);
    ~SettingsDialog();

private slots:
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_restoreDefaults_clicked();

public:
    struct settings
    {
        int httpPort;
        int udpPort;
        int tcpPort;

        int maxDLSpeed;
        int maxULSpeed;

        int maxSeeders;
        int maxLeechers;

        settings(int httpPort, int udpPort, int tcpPort,
                           int maxDLSpeed, int maxULSpeed,
                           int maxSeeders, int maxLeechers)
            : httpPort{httpPort}, udpPort{udpPort}, tcpPort{tcpPort},
              maxDLSpeed{maxDLSpeed}, maxULSpeed{maxULSpeed},
              maxSeeders{maxSeeders}, maxLeechers{maxLeechers}
        {}
    };

private:
    Ui::SettingsDialog *ui;

    QString storedTorrentPath;

signals:
    void sendModifiedSettings(settings modifiedSettings);


};

}

#endif // SETTINGSDIALOG_H
