#ifndef CREATETORRENT_H
#define CREATETORRENT_H

#include <QDialog>
#include <QFileDialog>
#include <QPointer>

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

private:
    Ui::CreateTorrent *ui;

    QString storedTorrentPath;
    QString storedWritePath;

    //settings
    bool isStartSeeding;

    //file dialogs
    QPointer<QFileDialog> selectFileDialog;
    QPointer<QFileDialog> selectFolderDialog;
    QPointer<QFileDialog> selectCreateDialog;
};

}

#endif // CREATETORRENT_H
