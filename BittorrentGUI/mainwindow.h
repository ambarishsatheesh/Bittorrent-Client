#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractTableModel>
#include <QTableView>
#include <QMenu>
#include <QPointer>
#include <QAction>
#include "tableModel.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

private slots:
    void on_actionAdd_Torrent_triggered();

    void on_actionExit_Client_triggered();

private:
  Ui::MainWindow *ui;
  void customHeaderMenuRequested(const QPoint& pos);
  void toggleColumnDisplay(bool checked);
  void loadTorrent(std::string fileName, std::string buffer);


  QMainWindow* m_rightSideWindow;
  QDockWidget* m_dockWidget1;
  QDockWidget* m_dockWidget2;
  QTableView* torrentTable;
  QMenu* torrentTableHeaderMenu;
  int numTorrentTableHeaderMenu;

  TestModel *model;

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


#endif // MAINWINDOW_H
