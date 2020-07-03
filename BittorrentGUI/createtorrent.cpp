#include "createtorrent.h"
#include "ui_createtorrent.h"
#include "loguru.h"

#include <QStandardPaths>
#include <QMessageBox>

namespace Bittorrent {

CreateTorrent::CreateTorrent(QPointer<QWidget> parent) :
    QDialog(parent),
    ui(new Ui::CreateTorrent),
    comment{""},
    isAddToClient{false}, isPrivate{false}
{
    ui->setupUi(this);

    //set progress bar range
    ui->progressBar->setRange(0, 5);

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

    //handle typed path (and deleted selected path)
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

        if (!QFile::exists(storedTorrentPath) &&
                !QDir(storedTorrentPath).exists())
        {
            QMessageBox::warning(this, "Warning",
                                 "Invalid path provided!",
                                 QMessageBox::Ok);
            return;
        }
    }

    //handle empty torrent name
    if (ui->torrentName->text().isEmpty())
    {
        QMessageBox::warning(this, "Warning",
                             "Please provide a torrent name.",
                             QMessageBox::Ok);
        return;
    }
    //handle empty trackers box
    if (ui->trackerUrls->toPlainText().isEmpty())
    {
        QMessageBox::warning(this, "Warning",
                             "Please provide at least one tracker.",
                             QMessageBox::Ok);
        return;
    }

    //get destination path
    QString writePath =
            QFileDialog::getSaveFileName(
                this, tr("Select where to save the new torrent"),
                                "",
                                tr("*.torrent"));

    //actual torrent file creation
    if (!writePath.isEmpty())
    {
        storedWritePath = writePath;

        //handle empty tracker url field
        if (!ui->trackerUrls->toPlainText().isEmpty())
        {
            //get data from QTextEdit and store in std::vector
            QString trackerData = ui->trackerUrls->toPlainText();
            QStringList qStrList = trackerData.split("\n");

            for(auto qTracker : qStrList)
            {
                trackerList.push_back(qTracker.toStdString());
            }

            //set current progress level
            ui->progressBar->setValue(2);
        }
        //handle empty comments field
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

        //set current progress level
        ui->progressBar->setValue(5);

        if (isAddToClient || ui->addToClient_check->isChecked())
        {
            LOG_F(INFO, "Started seeding torrent %s.",
                  storedWritePath.toStdString().c_str());

            //notify mainwindow and load torrent to client
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

void CreateTorrent::on_addToClient_check_stateChanged(int arg1)
{
    isAddToClient = ui->addToClient_check->isChecked() ? true : false;
}

}


