#include "addtorrentdialog.h"
#include "loguru.h"

#include <QStandardPaths>
#include <QMessageBox>

namespace Bittorrent {

using namespace torrentManipulation;

AddTorrentDialog::AddTorrentDialog(std::string filePath, std::string& buffer,
                                   QPointer<QWidget> parent)
    : QDialog(parent), ui(new Ui::AddTorrentDialog), storedTorrentPath{""},
      modifiedTorrent{modifiedTorrent}
{
    ui->setupUi(this);

    //set placeholder text in path field
    auto locationList = QStandardPaths::standardLocations(
                QStandardPaths::DocumentsLocation);

    for (auto location : locationList)
    {
        if (!location.isEmpty())
        {
            ui->pathField->setText(location);
        }
        else
        {
            continue;
        }
    }

    initContentTree();

    valueDictionary decodedTorrent =
            boost::get<valueDictionary>(Decoder::decode(buffer));
    *modifiedTorrent = toTorrentObj(filePath.c_str(), decodedTorrent);

    //set label values
    ui->sizeVal->setText(
                QString::fromStdString(
                    humanReadableBytes(modifiedTorrent->piecesData.totalSize)));
    ui->commentVal->setText(
                QString::fromStdString(modifiedTorrent->generalData.comment));
    ui->hashVal->setText(
                QString::fromStdString(
                    modifiedTorrent->hashesData.hexStringInfoHash));

}

AddTorrentDialog::~AddTorrentDialog()
{
    delete ui;
}

void AddTorrentDialog::initContentTree()
{
    contentTreeView = new QTreeView(this);
    contentTreeModel = new ContentTreeModel(*modifiedTorrent, ui->contentTree);
    contentTreeView->setModel(contentTreeModel);
    contentTreeView->setColumnWidth(0, 500);
}

void AddTorrentDialog::on_buttonCreate_clicked()
{
    storedTorrentPath = ui->pathField->text();

    if (!QDir(storedTorrentPath).exists())
    {
        QMessageBox::warning(this, "Warning",
                             "Invalid path provided!",
                             QMessageBox::Ok);
        return;
    }

    //actual torrent file creation
    if (!storedTorrentPath.isEmpty())
    {
        //set download directory using input value
        modifiedTorrent->generalData.downloadDirectory =
                storedTorrentPath.toStdString();

        //send back to MainWindow
        emit sendModifiedTorrent(*modifiedTorrent);
    }
}

void AddTorrentDialog::on_browse_clicked()
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

void AddTorrentDialog::on_buttonCancel_clicked()
{
    //implement cancel torrent creation
    /////////////////////////
    this->close();
}

}

