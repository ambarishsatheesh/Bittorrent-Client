#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractTableModel>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QTableView>
#include <QMenu>
#include <QPointer>
#include <QAction>
#include <QWidgetAction>
#include <QCheckBox>
#include <QFileDialog>

#include "Client.h"
#include "tableModel.h"
#include "torrentheadercheckbox.h"
#include "createtorrent.h"

namespace Ui {
class MainWindow;
}


namespace Bittorrent {

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(Client* client, QWidget *parent = 0);
  ~MainWindow();

private slots:
    void on_actionAdd_Torrent_triggered();

    void on_actionExit_Client_triggered();

    void customHeaderMenuRequested(const QPoint& pos);

    void customTorrentSelectRequested(const QPoint& pos);

    void on_actionDelete_triggered();

    void on_actionTorrent_Creator_triggered();

    void loadCreatedTorrent(QString filePath);

private:
    Ui::MainWindow* ui;
    Client* ioClient;
    QPointer<CreateTorrent> createTorDialog;

    void toggleColumnDisplay();
    void loadTorrent(std::string fileName, std::string& buffer);

    QPointer<QMainWindow> m_rightSideWindow;
    QPointer<QDockWidget> m_dockWidget1;
    QPointer<QDockWidget> m_dockWidget2;
    QPointer<QTableView> torrentTable;

    QPointer<QMenu> torrentTableHeaderMenu;
    QPointer<QMenu> torrentTableMainMenu;
    bool isFirstTorrentHeaderMenu;

    QPointer<TestModel> model;

    //table main menu
    QPointer<QAction> a_deleteTorrent;

    //file dialog
    QPointer<QFileDialog> addTorrentDialog;

    //table header menu
    QList<QPair<QString, QPointer<TorrentHeaderCheckbox>>> actionList;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_Priority;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_AddedOn;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_Name;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_Size;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_Progress;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_Status;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_Seeds;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_Peers;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_DownloadSpeed;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_UploadSpeed;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_ETA;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_Ratio;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_Tracker;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_Downloaded;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_Uploaded;
    QPointer<TorrentHeaderCheckbox> toggleDisplay_TimeActive;



};

}


#endif // MAINWINDOW_H
