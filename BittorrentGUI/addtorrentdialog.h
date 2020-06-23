#ifndef ADDTORRENTDIALOG_H
#define ADDTORRENTDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QPointer>
#include <QTreeView>

#include "ui_addTorrent.h"
#include "contenttreemodel.h"
#include "TorrentManipulation.h"
#include "Decoder.h"

namespace Ui {
class AddTorrentDialog;
}

namespace Bittorrent {

class AddTorrentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddTorrentDialog(std::string filePath, std::string& buffer,
                              QPointer<QWidget> parent = nullptr);
    ~AddTorrentDialog();

private slots:
    void on_buttonCreate_clicked();

    void on_buttonCancel_clicked();

    void on_browse_clicked();

signals:
    void sendModifiedTorrent(Torrent modifiedTorrent);

private:
    Ui::AddTorrentDialog *ui;

    QString storedTorrentPath;

    Torrent modifiedTorrent;

    QPointer<QFileDialog> selectFolderDialog;

    //file content tree
    void initContentTree();
    //QPointer<QTreeView> contentTreeView;
    QPointer<ContentTreeModel> contentTreeModel;
};

}

#endif // ADDTORRENTDIALOG_H
