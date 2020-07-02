#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <cmath>

namespace Bittorrent {

SettingsDialog::SettingsDialog(QPointer<QWidget> parent)
    : QDialog(parent), ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    //set default values
    ui->httpPort_val->setValue(80);
    ui->udpPort_val->setValue(6689);
    ui->tcpPort_val->setValue(6682);

    ui->maxDLSpeed_val->setValue(std::ceil(4194304/1024));
    ui->maxDLSpeed_val->setSuffix("KiB/s");
    ui->maxULSpeed_val->setValue(std::ceil(524288/1024));
    ui->maxULSpeed_val->setSuffix("KiB/s");

    ui->maxSeeders_val->setValue(5);
    ui->maxLeechers_val->setValue(5);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_btnOk_clicked()
{
    settings modifiedSettings
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
    ui->httpPort_val->setValue(80);
    ui->udpPort_val->setValue(6689);
    ui->tcpPort_val->setValue(6682);

    ui->maxDLSpeed_val->setValue(std::ceil(4194304/1024));
    ui->maxULSpeed_val->setValue(std::ceil(524288/1024));

    ui->maxSeeders_val->setValue(5);
    ui->maxLeechers_val->setValue(5);
}

}
