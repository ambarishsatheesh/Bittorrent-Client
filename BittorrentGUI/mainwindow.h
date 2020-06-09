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
#include <QHash>

#include "Client.h"
#include "tableModel.h"
#include "torrentheadercheckbox.h"

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

  void toggleColumnDisplay();
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
