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
#include <QHash>

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

    void customTorrentSelectRequested(const QPoint& pos);

    void on_actionDelete_triggered();

private:
  Ui::MainWindow *ui;
  Client* ioClient;

  void toggleColumnDisplay(bool checked);
  void loadTorrent(std::string fileName, std::string& buffer);

  QMainWindow* m_rightSideWindow;
  QDockWidget* m_dockWidget1;
  QDockWidget* m_dockWidget2;
  QTableView* torrentTable;

  QMenu* torrentTableHeaderMenu;
  QMenu* torrentTableMainMenu;
  bool isFirstTorrentHeaderMenu;

  TestModel *model;

  //table main menu
  QPointer<QAction> a_deleteTorrent;

  //table header menu
  QList<QPair<QString, QPointer<QAction>>> actionList;
  QPointer<QAction> toggleDisplay_Priority;
  QPointer<QAction> toggleDisplay_AddedOn;
  QPointer<QAction> toggleDisplay_Name;
  QPointer<QAction> toggleDisplay_Size;
  QPointer<QAction> toggleDisplay_Progress;
  QPointer<QAction> toggleDisplay_Status;
  QPointer<QAction> toggleDisplay_Seeds;
  QPointer<QAction> toggleDisplay_Peers;
  QPointer<QAction> toggleDisplay_DownloadSpeed;
  QPointer<QAction> toggleDisplay_UploadSpeed;
  QPointer<QAction> toggleDisplay_ETA;
  QPointer<QAction> toggleDisplay_Ratio;
  QPointer<QAction> toggleDisplay_Tracker;
  QPointer<QAction> toggleDisplay_Downloaded;
  QPointer<QAction> toggleDisplay_Uploaded;
  QPointer<QAction> toggleDisplay_TimeActive;



};

}


#endif // MAINWINDOW_H
