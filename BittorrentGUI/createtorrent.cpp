#include "createtorrent.h"
#include "ui_createtorrent.h"
#include "loguru.h"

#include <QStandardPaths>
#include <QMessageBox>

namespace Bittorrent {

CreateTorrent::CreateTorrent(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateTorrent),
    comment{""},
    isStartSeeding{false}, isPrivate{false}
{
    ui->setupUi(this);

    //set placeholder text in path field
    auto locationList = QStandardPaths::standardLocations(
                QStandardPaths::DocumentsLocation);

    for (auto location : locationList)
    {
        if (!location.isEmpty())
        {
            ui->pathField->setPlaceholderText(location);
        }
        else
        {
            continue;
        }
    }
}

CreateTorrent::~CreateTorrent()
{
    delete ui;
}

void CreateTorrent::on_buttonCreate_clicked()
{
    using namespace torrentManipulation;

    if (storedTorrentPath.isEmpty())
    {
        if (ui->pathField->text().isEmpty())
        {
            QMessageBox::warning(this, "Warning",
                                 "No destination path provided!",
                                 QMessageBox::Ok);
            return;
        }
        else
        {
            storedTorrentPath = ui->pathField->text();
        }
    }

    if (ui->torrentName->text().isEmpty())
    {
        QMessageBox::warning(this, "Warning",
                             "Please provide a torrent name.",
                             QMessageBox::Ok);
        return;
    }

    //get destination path
    QString writePath =
            QFileDialog::getSaveFileName(
                this, tr("Select where to save the new torrent"),
                                "",
                                tr("*.torrent"));

    if (!writePath.isEmpty())
    {
        storedWritePath = writePath;

        if (!ui->trackerUrls->toPlainText().isEmpty())
        {
            //get data from QTextEdit and store in std::vector
            QString trackerData = ui->trackerUrls->toPlainText();
            QStringList qStrList = trackerData.split("\n");

            for(auto qTracker : qStrList)
            {
                trackerList.push_back(qTracker.toStdString());
            }
        }
        if (!ui->comments->toPlainText().isEmpty())
        {
            comment = ui->comments->toPlainText().toStdString();
        }

        torrentName = ui->torrentName->text().toStdString();

        createNewTorrent(torrentName,
                    storedTorrentPath.toStdString().c_str(),
                    storedWritePath.toStdString(),
                    isPrivate, comment, trackerList);

        LOG_F(INFO, "Created torrent for \"%s\". Torrent saved to %s",
              storedTorrentPath.toStdString().c_str(),
              storedWritePath.toStdString().c_str());


        if (isStartSeeding)
        {
            LOG_F(INFO, "Started seeding torrent %s.",
                  storedWritePath.toStdString().c_str());

            //implement seeding code


            //notify mainwindow by seting result code to Accepted and
            //calling accept() signal
            emit sendfilePath(storedWritePath);
        }
    }
}

void CreateTorrent::on_selectFile_clicked()
{
    selectFileDialog = new QFileDialog(this);
    selectFileDialog->setViewMode(QFileDialog::Detail);
    selectFileDialog->setFileMode(QFileDialog::ExistingFile);

    if ( QDialog::Accepted == selectFileDialog->exec() )
    {
        QStringList filenames = selectFileDialog->selectedFiles();
        QStringList::const_iterator it = filenames.begin();
        QStringList::const_iterator eIt = filenames.end();
        while ( it != eIt )
        {
            QString fileName = *it++;
            if ( !fileName.isEmpty() )
            {
                storedTorrentPath = fileName;
                ui->pathField->setText(fileName);
            }
        }
    }
}

void CreateTorrent::on_selectFolder_clicked()
{
    selectFolderDialog = new QFileDialog(this);
    selectFolderDialog->setViewMode(QFileDialog::Detail);
    selectFolderDialog->setFileMode(QFileDialog::Directory);

    if ( QDialog::Accepted == selectFolderDialog->exec() )
    {
        QStringList directoryNames = selectFolderDialog->selectedFiles();
        QStringList::const_iterator it = directoryNames.begin();
        QStringList::const_iterator eIt = directoryNames.end();
        while ( it != eIt )
        {
            QString directoryName = *it++;
            if ( !directoryName.isEmpty() )
            {
                storedTorrentPath = directoryName;
                ui->pathField->setText(directoryName);
            }
        }
    }
}


void CreateTorrent::on_buttonCancel_clicked()
{
    //implement cancel torrent creation
    /////////////////////////
    this->close();
}

}

void Bittorrent::CreateTorrent::on_startSeedingCheckBox_stateChanged(int arg1)
{
    isStartSeeding = ui->startSeedingCheckBox->isChecked() ? true : false;
}
