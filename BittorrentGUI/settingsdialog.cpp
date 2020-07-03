#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <cmath>

namespace Bittorrent {

SettingsDialog::SettingsDialog(WorkingTorrents::settings* defaultSettings,
                               QPointer<QWidget> parent)
    : QDialog(parent), ui(new Ui::SettingsDialog),
      defaultSettings{defaultSettings}
{
    ui->setupUi(this);

    //set default values
    ui->httpPort_val->setMaximum(std::numeric_limits<int>::max());
    ui->udpPort_val->setMaximum(std::numeric_limits<int>::max());
    ui->tcpPort_val->setMaximum(std::numeric_limits<int>::max());
    ui->maxDLSpeed_val->setMaximum(std::numeric_limits<int>::max());
    ui->maxULSpeed_val->setMaximum(std::numeric_limits<int>::max());
    ui->maxSeeders_val->setMaximum(10);
    ui->maxLeechers_val->setMaximum(10);

    ui->httpPort_val->setValue(defaultSettings->httpPort);
    ui->udpPort_val->setValue(defaultSettings->udpPort);
    ui->tcpPort_val->setValue(defaultSettings->tcpPort);

    ui->maxDLSpeed_val->setValue(std::ceil(defaultSettings->maxDLSpeed/1024));
    ui->maxDLSpeed_val->setSuffix("KiB/s");
    ui->maxULSpeed_val->setValue(std::ceil(defaultSettings->maxULSpeed/1024));
    ui->maxULSpeed_val->setSuffix("KiB/s");

    ui->maxSeeders_val->setValue(defaultSettings->maxSeeders);
    ui->maxLeechers_val->setValue(defaultSettings->maxLeechers);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_btnOk_clicked()
{
    WorkingTorrents::settings modifiedSettings
    {
        ui->httpPort_val->value(),
        ui->udpPort_val->value(),
        ui->tcpPort_val->value(),
        ui->maxDLSpeed_val->value(),
        ui->maxULSpeed_val->value(),
        ui->maxSeeders_val->value(),
        ui->maxLeechers_val->value(),
    };

    emit sendModifiedSettings(modifiedSettings);

    this->close();
}

void SettingsDialog::on_btnCancel_clicked()
{
    this->close();
}

void SettingsDialog::on_restoreDefaults_clicked()
{
    ui->httpPort_val->setValue(defaultSettings->httpPort);
    ui->udpPort_val->setValue(defaultSettings->udpPort);
    ui->tcpPort_val->setValue(defaultSettings->tcpPort);

    ui->maxDLSpeed_val->setValue(std::ceil(defaultSettings->maxDLSpeed/1024));
    ui->maxULSpeed_val->setValue(std::ceil(defaultSettings->maxULSpeed/1024));

    ui->maxSeeders_val->setValue(defaultSettings->maxSeeders);
    ui->maxLeechers_val->setValue(defaultSettings->maxLeechers);
}

}
