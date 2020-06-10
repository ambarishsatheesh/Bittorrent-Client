#ifndef CREATETORRENT_H
#define CREATETORRENT_H

#include <QDialog>
#include <QFileDialog>
#include <QPointer>

#include "TorrentManipulation.h"

namespace Ui {
class CreateTorrent;
}

namespace Bittorrent {

class CreateTorrent : public QDialog
{
    Q_OBJECT

public:
    explicit CreateTorrent(QWidget *parent = nullptr);
    ~CreateTorrent();

private slots:
    void on_buttonCreate_clicked();

    void on_buttonCancel_clicked();

    void on_selectFile_clicked();

    void on_selectFolder_clicked();

    void on_startSeedingCheckBox_stateChanged(int arg1);

signals:
    void sendfilePath(QString);

private:
    Ui::CreateTorrent *ui;

    QString storedTorrentPath;
    QString storedWritePath;

    //creationData
    std::vector<std::string> trackerList;
    std::string comment;
    std::string torrentName;

    //torrent
    std::shared_ptr<Torrent> createdTorrent;

    //settings
    bool isStartSeeding;
    bool isPrivate;

    //file dialogs
    QPointer<QFileDialog> selectFileDialog;
    QPointer<QFileDialog> selectFolderDialog;
    QPointer<QFileDialog> selectCreateDialog;
};

}

#endif // CREATETORRENT_H
