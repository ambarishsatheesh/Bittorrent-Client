#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QWidget>
#include <QDialog>
#include <QFileDialog>
#include <QPointer>

#include "workingTorrents.h"


namespace Ui {
class SettingsDialog;
}

namespace Bittorrent {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(WorkingTorrents::settings* defaultSettings,
                            QPointer<QWidget> parent = nullptr);
    ~SettingsDialog();

private slots:
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_restoreDefaults_clicked();

private:
    Ui::SettingsDialog *ui;

    WorkingTorrents::settings* defaultSettings;

signals:
    void sendModifiedSettings(WorkingTorrents::settings modifiedSettings);


};

}

#endif // SETTINGSDIALOG_H
