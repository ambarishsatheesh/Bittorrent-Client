#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractTableModel>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QDockWidget>
#include <QHeaderView>
#include <QTableView>
#include <QListView>
#include <QMenu>
#include <QPointer>
#include <QAction>
#include <QWidgetAction>
#include <QCheckBox>
#include <QFileDialog>
#include <QSortFilterProxyModel>
#include <QToolBar>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFont>
#include <QPushButton>
#include <QStackedWidget>
#include <QDataWidgetMapper>
#include <QTreeView>
#include <QTextBrowser>

#include "ui_generalInfoTab.h"
#include "Client.h"
#include "tableModel.h"
#include "torrentheadercheckbox.h"
#include "createtorrent.h"
#include "torrentsortfilterproxymodel.h"
#include "torrentinfolist.h"
#include "trackertablemodel.h"
#include "generalinfomodel.h"
#include "contenttreemodel.h"
#include "addtorrentdialog.h"
#include "settingsdialog.h"
#include "TorrentManipulation.h"

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

    void toggleColumnDisplay();

    void loadTorrent(std::string fileName, std::string& buffer);

    void textFilterChanged();

    void trackerListItemSelected(const QModelIndex& index);

    void showAllTorrents();

    void duplicateTorrentSlot(QString torrentName);

    void torrentSelected(const QItemSelection &selected,
                         const QItemSelection &deselected);

    void on_actionResume_triggered();

    void on_actionPause_triggered();

    void handleNewTorrent(Torrent modifiedTorrent);

    void on_actionOptions_triggered();

    void on_settingsChange(WorkingTorrents::settings modifiedSettings);

private:
    Ui::MainWindow* ui;
    Ui::generalInfo* generalInfoTab;
    Client* ioClient;

    //Windows
    void initWindows();
    QPointer<QMainWindow> m_rightSideWindow;
    QPointer<QDockWidget> m_dockWidget1;
    QPointer<QDockWidget> m_dockWidget2;
    QPointer<QDockWidget> m_dockWidget3;
    QPointer<QDockWidget> m_dockWidget4;
    QPointer<QDockWidget> m_dockWidget5;
    QPointer<QSplitter> splitter1;
    QPointer<QTabWidget> leftWidget;
    QPointer<QWidget> transfersTab;
    QPointer<QWidget> logTab;

    //torrents table
    void initTorrentTable();
    QPointer<QTableView> torrentTable;
    QPointer<TorrentTableModel> torrentModel;

    //general info
    void initGeneralInfo();
    QPointer<generalInfoModel> generalDataModel;
    QPointer<QDataWidgetMapper> generalInfoMapper;

    //trackers tables
    void initTrackersTable();
    QPointer<QTableView> initTrackerTable;
    QPointer<TrackerTableModel> initTrackerModel;
    QPointer<QStackedWidget> trackerTableStack;
    struct trackerTableData
    {
        QPointer<QTableView> trackerTable;
        QPointer<TrackerTableModel> trackerModel;

        trackerTableData(QPointer<QTableView> table,
                         QPointer<TrackerTableModel> model)
            : trackerTable(std::move(table)),
              trackerModel(std::move(model))
        {}
    };
    QVector<trackerTableData> trackerTableVec;

    //content tree
    void initContentTree();
    QPointer<QTreeView> initContentTreeView;
    QPointer<ContentTreeModel> initContentTreeModel;
    QPointer<QStackedWidget> contentTreeStack;
    struct contentTreeData
    {
        QPointer<QTreeView> contentTreeView;
        QPointer<ContentTreeModel> contentTreeModel;

        contentTreeData(QPointer<QTreeView> tree,
                         QPointer<ContentTreeModel> model)
            : contentTreeView(std::move(tree)),
              contentTreeModel(std::move(model))
        {}
    };
    QVector<contentTreeData> contentTreeVec;

    //transfers tab
    void initTransfersTab();
    QPointer<QListView> infoList;

    //log tab
    void initLogTab();
    QPointer<QTextEdit> textEdit;
    QPointer<QVBoxLayout> logLayout;

    //toolbar
    void initToolbar();
    QPointer<QToolBar> toolbar;
    QPointer<QAction> toolbar_addTorrent;
    QPointer<QAction> toolbar_deleteTorrent;
    QPointer<QAction> toolbar_resume;
    QPointer<QAction> toolbar_pause;
    QPointer<QAction> toolbar_maxPriority;
    QPointer<QAction> toolbar_increasePriority;
    QPointer<QAction> toolbar_decreasePriority;
    QPointer<QAction> toolbar_minPriority;
    QPointer<QAction> toolbar_settings;

    QPointer<QWidget> dummySpacer;
    QPointer<QLineEdit> searchFilter;

    //proxy models
    QPointer<TorrentSortFilterProxyModel> proxyModel;

    //Menus
    QPointer<QMenu> torrentTableHeaderMenu;
    QPointer<QMenu> torrentTableMainMenuData;
    QPointer<QMenu> torrentTableMainMenuOutside;
    bool isFirstTorrentHeaderMenu;
    bool isFirstTorrentTableMenuData;
    bool isFirstTorrentTableMenuOutside;

    //table main menu
    QPointer<QAction> a_addTorrent;
    QPointer<QAction> a_deleteTorrent;
    QPointer<QAction> a_resumeTorrent;
    QPointer<QAction> a_pauseTorrent;

    //Box
    QPointer<QVBoxLayout> trackerBox;

    //label
    QPointer<QLabel> trackersHeader;

    //add new torrent
    QPointer<QFileDialog> addTorrentDialog;
    QPointer<AddTorrentDialog> addTorInfoDialog;

    //create new torrent
    QPointer<CreateTorrent> createTorDialog;

    //settings menu
    QPointer<SettingsDialog> settingsDialog;

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

    //infoList
    QPointer<TorrentInfoList> infoListModel;

};

}


#endif // MAINWINDOW_H
