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

#include "Client.h"
#include "tableModel.h"

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

    void customMainMenuRequested(const QPoint& pos);

    void on_actionDelete_triggered();

private:
  Ui::MainWindow *ui;
  Client* ioClient;

  void toggleColumnDisplay(bool checked);
  void loadTorrent(std::string fileName, std::string& buffer);
  void deleteTorrent();


  QMainWindow* m_rightSideWindow;
  QDockWidget* m_dockWidget1;
  QDockWidget* m_dockWidget2;
  QTableView* torrentTable;

  QMenu* torrentTableHeaderMenu;
  QMenu* torrentTableMainMenu;
  int numTorrentTableHeaderMenu;

  TestModel *model;

  //table main menu
  QPointer<QAction> a_deleteTorrent;

  //table header menu
  QPointer<QAction> toggleDisplayPriority;
  QPointer<QAction> toggleDisplayAddedOn;
  QPointer<QAction> toggleDisplayName;
  QPointer<QAction> toggleDisplaySize;
  QPointer<QAction> toggleDisplayProgress;
  QPointer<QAction> toggleDisplayStatus;
  QPointer<QAction> toggleDisplaySeeds;
  QPointer<QAction> toggleDisplayPeers;
  QPointer<QAction> toggleDisplayDSpeed;
  QPointer<QAction> toggleDisplayUSpeed;
  QPointer<QAction> toggleDisplayETA;
  QPointer<QAction> toggleDisplayRatio;
  QPointer<QAction> toggleDisplayTracker;
  QPointer<QAction> toggleDisplayTimeActive;
  QPointer<QAction> toggleDisplayDownloaded;
  QPointer<QAction> toggleDisplayUploaded;


};

}


#endif // MAINWINDOW_H
