#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tableModel.h"
#include "progressDelegate.h"
#include "Decoder.h"
#include "loguru.h"

#include <iostream>
#include <thread>
#include <sstream>

#include <QAbstractItemModelTester>
#include <QMessageBox>
#include <QtConcurrent>

namespace Bittorrent
{
using namespace utility;

void logCallback(void* user_data, const loguru::Message& message)
{
    if (message.verbosity == loguru::Verbosity_WARNING)
    {
        //orange
        reinterpret_cast<QTextEdit*>(user_data)->setTextColor(QColor("#FF9933"));
        reinterpret_cast<QTextEdit*>(user_data)->append(
                    QString(message.preamble) + QString(message.message));
    }
    else if (message.verbosity == loguru::Verbosity_ERROR)
    {
        reinterpret_cast<QTextEdit*>(user_data)->setTextColor(Qt::red);
        reinterpret_cast<QTextEdit*>(user_data)->append(
                    QString(message.preamble) + QString(message.message));
    }
}

MainWindow::MainWindow(Client* client, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      generalInfoTab(new Ui::generalInfo),
      ioClient(client), isFirstTorrentHeaderMenu{true},
      isFirstTorrentTableMenuData{true},
      isFirstTorrentTableMenuOutside{true}
{
    //initialise ui forms
    ui->setupUi(this);
    generalInfoTab->setupUi(this);

    this->setWindowTitle("ioTorrent");

    //initialise windows
    initWindows();

    //initialise log tab
    initLogTab();

    //Initialise toolbar
    initToolbar();

    //Initialise tables
    initTorrentTable();
    initTrackersTable();

    //initialise general info
    initGeneralInfo();

    //initialise content tree
    initContentTree();

    //initialise tabs
    initTransfersTab();

    LOG_F(INFO, "Client ready!");
    textEdit->append("Client Ready!");

//    auto tester =
//            new QAbstractItemModelTester(
//                torrentModel, QAbstractItemModelTester::FailureReportingMode::Fatal, this);
}

MainWindow::~MainWindow()
{
    delete ui;
    loguru::remove_callback("logToWidget_WAR");
    loguru::remove_callback("logToWidget_ERR");
}

void MainWindow::initWindows()
{
    splitter1 = new QSplitter(this);
    splitter1->setOrientation(Qt::Horizontal);
    setCentralWidget(splitter1);

    leftWidget = new QTabWidget(this);

    transfersTab = new QWidget();
    logTab = new QWidget();

    leftWidget->addTab(transfersTab, "Transfers");
    leftWidget->addTab(logTab, "Log");

    m_rightSideWindow = new QMainWindow(this);
    m_rightSideWindow->setWindowFlags(Qt::Widget);
    m_rightSideWindow->layout()->setContentsMargins(3, 3, 3, 3);

    splitter1->addWidget(leftWidget);
    splitter1->addWidget(m_rightSideWindow);

    //sub windows
    m_dockWidget1 = new QDockWidget("Torrents", this);
    m_rightSideWindow->addDockWidget(Qt::TopDockWidgetArea, m_dockWidget1);
    m_dockWidget1->setTitleBarWidget(new QWidget());
    m_dockWidget1->setAllowedAreas(Qt::NoDockWidgetArea);

    m_dockWidget2 = new QDockWidget("General", this);
    m_rightSideWindow->addDockWidget(Qt::BottomDockWidgetArea, m_dockWidget2);
    m_dockWidget2->setTitleBarWidget(new QWidget());
    m_dockWidget2->setAllowedAreas(Qt::NoDockWidgetArea);

    m_dockWidget3 = new QDockWidget("Trackers", this);
    m_rightSideWindow->addDockWidget(Qt::BottomDockWidgetArea, m_dockWidget3);
    m_dockWidget3->setTitleBarWidget(new QWidget());
    m_rightSideWindow->tabifyDockWidget(m_dockWidget2, m_dockWidget3);

    m_dockWidget4 = new QDockWidget("Content", this);
    m_rightSideWindow->addDockWidget(Qt::BottomDockWidgetArea, m_dockWidget4);
    m_dockWidget4->setTitleBarWidget(new QWidget());
    m_rightSideWindow->tabifyDockWidget(m_dockWidget2, m_dockWidget4);

    m_dockWidget5 = new QDockWidget("Speed", this);
    m_rightSideWindow->addDockWidget(Qt::BottomDockWidgetArea, m_dockWidget5);
    m_dockWidget5->setTitleBarWidget(new QWidget());
    m_rightSideWindow->tabifyDockWidget(m_dockWidget2, m_dockWidget5);

    //set as default tab
    m_dockWidget2->show();
    m_dockWidget2->raise();
}

void MainWindow::initLogTab()
{
    //add QTextEdit widget to log tab
    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QTextEdit::NoWrap);

    //add layout
    logLayout = new QVBoxLayout(this);
    logLayout->addWidget(textEdit);

    logTab->setLayout(logLayout);

    //add callback for logging so any logs are displayed in logTab
//    loguru::add_callback("logToWidget_WAR", logCallback, textEdit,
//                         loguru::Verbosity_WARNING);
//    loguru::add_callback("logToWidget_ERR", logCallback, textEdit,
//                         loguru::Verbosity_ERROR);
}

void MainWindow::initToolbar()
{
    toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    this->addToolBar(Qt::TopToolBarArea, toolbar);

    toolbar_addTorrent = new QAction(QIcon(":/imgs/Icons/addTorrent.png"), "");
    toolbar_addTorrent->setToolTip("Add Torrent");
    connect(toolbar_addTorrent, &QAction::triggered, this,
            &MainWindow::on_actionAdd_Torrent_triggered);
    toolbar->addAction(toolbar_addTorrent);

    toolbar_deleteTorrent = new QAction(QIcon(":/imgs/Icons/deleteTorrent.png"), "");
    toolbar_deleteTorrent->setToolTip("Delete Torrent");
    connect(toolbar_deleteTorrent, &QAction::triggered, this,
            &MainWindow::on_actionDelete_triggered);
    toolbar->addAction(toolbar_deleteTorrent);

    toolbar->addSeparator();

    toolbar_resume = new QAction(QIcon(":/imgs/Icons/resume.png"), "");
    toolbar_resume->setToolTip("Resume");
    connect(toolbar_resume, &QAction::triggered, this,
            &MainWindow::on_actionResume_triggered);
    toolbar->addAction(toolbar_resume);

    toolbar_pause = new QAction(QIcon(":/imgs/Icons/pause.png"), "");
    toolbar_pause->setToolTip("Pause");
    connect(toolbar_pause, &QAction::triggered, this,
            &MainWindow::on_actionPause_triggered);
    toolbar->addAction(toolbar_pause);

    toolbar->addSeparator();

    toolbar_maxPriority = new QAction(QIcon(":/imgs/Icons/maxPriority.png"), "");
    toolbar_maxPriority->setToolTip("Max Priority");
    toolbar_increasePriority = new QAction(QIcon(":/imgs/Icons/increasePriority.png"), "");
    toolbar_increasePriority->setToolTip("Increase Priority");
    toolbar_decreasePriority = new QAction(QIcon(":/imgs/Icons/decreasePriority.png"), "");
    toolbar_decreasePriority->setToolTip("Decrease Priority");
    toolbar_minPriority = new QAction(QIcon(":/imgs/Icons/minPriority.png"), "");
    toolbar_minPriority->setToolTip("Min Priority");
    toolbar->addAction(toolbar_maxPriority);
    toolbar->addAction(toolbar_increasePriority);
    toolbar->addAction(toolbar_decreasePriority);
    toolbar->addAction(toolbar_minPriority);

    toolbar->addSeparator();

    toolbar_settings = new QAction(QIcon(":/imgs/Icons/settings.png"), "");
    toolbar_settings->setToolTip("Settings");
    toolbar->addAction(toolbar_settings);

    toolbar->addSeparator();

    //"show all" button
    QPointer<QPushButton> btn_showAll =
            new QPushButton("Show All Torrents", this);
    connect(btn_showAll, &QPushButton::clicked, this,
            &MainWindow::showAllTorrents);
    toolbar->addWidget(btn_showAll);

    //create dummy spacer so that search bar is right aligned
    dummySpacer = new QWidget();
    dummySpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolbar->addWidget(dummySpacer);

    //add search bar to toolbar
    searchFilter = new QLineEdit(this);
    searchFilter->setSizePolicy(
                QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    searchFilter->setMaximumWidth(250);
    searchFilter->setPlaceholderText("Filter torrents");
    searchFilter->setClearButtonEnabled(true);
    searchFilter->addAction(
                QIcon(":/imgs/Icons/search.png"), QLineEdit::LeadingPosition);
    toolbar->addWidget(searchFilter);

    connect(searchFilter, &QLineEdit::textChanged, this,
            &MainWindow::textFilterChanged);
}

void MainWindow::initTorrentTable()
{
    torrentTable = new QTableView(m_dockWidget1);

    //torrentTable->horizontalHeader()->setSectionsMovable(true);
    torrentTable->verticalHeader()->setVisible(false);
    torrentTable->setShowGrid(false);
    torrentTable->setAlternatingRowColors(true);
    torrentTable->setStyleSheet("alternate-background-color: #F0F0F0;");
    torrentTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    torrentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    //disable bold header text when data is selected
    torrentTable->horizontalHeader()->setHighlightSections(false);
    //right click menus
    torrentTable->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    torrentTable->setContextMenuPolicy(Qt::CustomContextMenu);

    torrentModel = new TorrentTableModel(ioClient, this);

    progressDelegate* delegate = new progressDelegate(torrentTable);
    torrentTable->setItemDelegateForColumn(5, delegate);

    //sort/filter
    proxyModel = new TorrentSortFilterProxyModel(ioClient, this);
    proxyModel->setSourceModel(torrentModel);
    torrentTable->setModel(proxyModel);
    torrentTable->setSortingEnabled(true);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    connect(torrentTable->horizontalHeader(),
            &QTableView::customContextMenuRequested,
            this, &MainWindow::customHeaderMenuRequested);

    connect(torrentTable, &QTableView::customContextMenuRequested,
            this, &MainWindow::customTorrentSelectRequested);

    connect(torrentTable->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this, &MainWindow::torrentSelected);

    //connect duplicate torrent signal
    connect(torrentModel, &TorrentTableModel::duplicateTorrentSig, this,
            &MainWindow::duplicateTorrentSlot);

    m_dockWidget1->setWidget(torrentTable);
}

void MainWindow::initTrackersTable()
{
    initTrackerTable = new QTableView;

    initTrackerTable->verticalHeader()->setVisible(false);
    initTrackerTable->setShowGrid(false);
    initTrackerTable->setAlternatingRowColors(true);
    initTrackerTable->setStyleSheet("alternate-background-color: #F0F0F0;");
    initTrackerTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    initTrackerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    //disable bold header text when data is selected
    initTrackerTable->horizontalHeader()->setHighlightSections(false);
    //right click menu
    initTrackerTable->setContextMenuPolicy(Qt::CustomContextMenu);

    //set up initial empty model
    std::vector<trackerObj> initVec;
    initTrackerModel = new TrackerTableModel(&initVec, this);

    initTrackerTable->setModel(initTrackerModel);

    trackerTableStack = new QStackedWidget(m_dockWidget3);
    trackerTableStack->addWidget(initTrackerTable);

    m_dockWidget3->setWidget(trackerTableStack);
}

void MainWindow::initGeneralInfo()
{
    generalDataModel = new generalInfoModel(ioClient, this);
    generalInfoMapper = new QDataWidgetMapper();
    generalInfoMapper->setOrientation(Qt::Horizontal);
    generalInfoMapper->setModel(generalDataModel);

    //map information values
    generalInfoMapper->addMapping(generalInfoTab->totalSize_val, 0, "text");
    generalInfoMapper->addMapping(generalInfoTab->addedOn_val, 1, "text");
    generalInfoMapper->addMapping(generalInfoTab->createdOn_val, 2, "text");
    generalInfoMapper->addMapping(generalInfoTab->createdBy_val, 3, "text");
    generalInfoMapper->addMapping(generalInfoTab->comment_val, 4, "text");
    generalInfoMapper->addMapping(generalInfoTab->completedOn_val, 5, "text");
    generalInfoMapper->addMapping(generalInfoTab->pieces_val, 6, "text");
    generalInfoMapper->addMapping(generalInfoTab->torrentHash_val, 7, "text");
    generalInfoMapper->addMapping(generalInfoTab->savePath_val, 8, "text");

    //map transfer values
    generalInfoMapper->addMapping(generalInfoTab->timeActive_val, 9, "text");
    generalInfoMapper->addMapping(generalInfoTab->downloaded_val, 10, "text");
    generalInfoMapper->addMapping(generalInfoTab->dlSpeed_val, 11, "text");
    generalInfoMapper->addMapping(generalInfoTab->ETA_val, 12, "text");
    generalInfoMapper->addMapping(generalInfoTab->uploaded_val, 13, "text");
    generalInfoMapper->addMapping(generalInfoTab->uSpeed_val, 14, "text");
    generalInfoMapper->addMapping(generalInfoTab->connections_val, 15, "text");
    generalInfoMapper->addMapping(generalInfoTab->seeds_val, 16, "text");
    generalInfoMapper->addMapping(generalInfoTab->peers_val, 17, "text");

    m_dockWidget2->setWidget(generalInfoTab->generalBox);
}

void MainWindow::initContentTree()
{
    std::vector<fileObj> initVec;
    initContentTreeView = new QTreeView(this);
    initContentTreeModel = new ContentTreeModel(ioClient, &initVec, this);
    initContentTreeView->setModel(initContentTreeModel);
    initContentTreeView->setColumnWidth(0, 500);

    contentTreeStack = new QStackedWidget(m_dockWidget4);
    contentTreeStack->addWidget(initContentTreeView);

    m_dockWidget4->setWidget(contentTreeStack);
}

void MainWindow::initTransfersTab()
{
    infoList = new QListView(this);
    infoListModel = new TorrentInfoList(ioClient, this);
    infoList->setModel(infoListModel);
    infoList->setFrameStyle(QFrame::NoFrame);

    //Trackers list header
    trackerBox = new QVBoxLayout(transfersTab);
    trackersHeader = new QLabel("Trackers");
    QFont trackerFont = trackersHeader->font();
    trackerFont.setWeight(QFont::Bold);
    trackersHeader->setFont(trackerFont);
    trackerBox->addWidget(trackersHeader);

    trackerBox->addWidget(infoList);

    connect(infoList, &QListView::clicked, this,
            &MainWindow::trackerListItemSelected);
}

void MainWindow::showAllTorrents()
{
    //use match anything regex
    QRegExp torrentFilterRegExp(".*");
    torrentFilterRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterRegExp(torrentFilterRegExp);

    //clear any filter list selections
    infoList->clearSelection();
}

void MainWindow::trackerListItemSelected(const QModelIndex& index)
{
    emit torrentModel->dataChanged(QModelIndex(), QModelIndex());
    if (index.isValid())
    {
        //store infoHashes of torrents associated with selected tracker
        //so that they can be used to filter torrent table rows
        auto trackerAdd = ioClient->WorkingTorrents.trackerTorrentMap.keys().
                at(index.row());
        auto infoHashes = ioClient->WorkingTorrents.trackerTorrentMap.
                value(trackerAdd);

        for (auto str : infoHashes)
        {
            proxyModel->infoHashList.push_back(str);
        }

        //placeholder dummy regex to trigger reapplication of filter
        //infoHashList is not empty so tracker filter will trigger instead of
        //search filter (see subclass function definition)
        proxyModel->setFilterRegExp(QRegExp(""));
    }

    //clear so that future search filter calls can be safely made
    proxyModel->infoHashList.clear();
}

void MainWindow::torrentSelected(const QItemSelection &selected,
                                 const QItemSelection &deselected)
{
    //get source indices from proxy model
    auto mappedSelection = proxyModel->mapSelectionToSource(selected);

    //map selected view indices to source torrentModel rows
    //use QSet to remove duplicates (since multiple indices will share a row)
    QSet<int> selectedSourceRowSet;
    for (auto selectionIndex : mappedSelection.indexes())
    {
        selectedSourceRowSet.insert(selectionIndex.row());
    }

    for (auto row : selectedSourceRowSet)
    {
        auto ptr_trackerStruct = std::make_unique<trackerTableData>(
                    trackerTableVec.at(row));
        auto ptr_contentStruct = std::make_unique<contentTreeData>(
                    contentTreeVec.at(row));

        //set current stack widgets to appropriate view
        trackerTableStack->setCurrentWidget(ptr_trackerStruct->trackerTable);
        contentTreeStack->setCurrentWidget(ptr_contentStruct->contentTreeView);

        //set general info mapper to relevant row
        generalInfoMapper->setCurrentIndex(row);

        //emit trackerTableVec.at(row).trackerModel->dataChanged(QModelIndex(), QModelIndex());
    }
}

void MainWindow::customTorrentSelectRequested(const QPoint& pos)
{
    //if right clicked in a valid row (i.e. only where there is data)
    if (torrentTable->indexAt(pos).isValid())
    {
        if (isFirstTorrentTableMenuData)
        {
            torrentTableMainMenuData = new QMenu(this);

            a_resumeTorrent = new QAction("Resume", this);
            connect(a_resumeTorrent, &QAction::triggered, this,
                    &MainWindow::on_actionResume_triggered);
            a_resumeTorrent->setIcon(QIcon(":/imgs/Icons/resume.png"));
            torrentTableMainMenuData->addAction(a_resumeTorrent);

            a_pauseTorrent = new QAction("Pause", this);
            connect(a_pauseTorrent, &QAction::triggered, this,
                    &MainWindow::on_actionPause_triggered);
            a_pauseTorrent->setIcon(QIcon(":/imgs/Icons/pause.png"));
            torrentTableMainMenuData->addAction(a_pauseTorrent);

            torrentTableMainMenuData->addSeparator();

            a_deleteTorrent = new QAction("Delete", this);
            connect(a_deleteTorrent, &QAction::triggered, this,
                    &MainWindow::on_actionDelete_triggered);
            a_deleteTorrent->setIcon(QIcon(":/imgs/Icons/deleteTorrent.png"));
            torrentTableMainMenuData->addAction(a_deleteTorrent);

            //set flag
            isFirstTorrentTableMenuData = false;
        }

        torrentTableMainMenuData->
                popup(m_rightSideWindow->mapToGlobal(pos));
    }
    else
    {
        if (isFirstTorrentTableMenuOutside)
        {
            torrentTableMainMenuOutside = new QMenu(this);

            a_addTorrent = new QAction("Add Torrent", this);
            connect(a_addTorrent, &QAction::triggered, this,
                    &MainWindow::on_actionAdd_Torrent_triggered);
            a_addTorrent->setIcon(QIcon(":/imgs/Icons/addTorrent.png"));
            torrentTableMainMenuOutside->addAction(a_addTorrent);

            //set flag
            isFirstTorrentTableMenuOutside = false;
        }

        torrentTableMainMenuOutside->
                popup(m_rightSideWindow->mapToGlobal(pos));
    }

}

void MainWindow::customHeaderMenuRequested(const QPoint& pos)
{
    if (isFirstTorrentHeaderMenu)
    {
        actionList.push_back({"Added On", toggleDisplay_AddedOn});
        actionList.push_back({"Priority", toggleDisplay_Priority});
        actionList.push_back({"Name", toggleDisplay_Name});
        actionList.push_back({"Size", toggleDisplay_Size});
        actionList.push_back({"Progress", toggleDisplay_Progress});
        actionList.push_back({"Status", toggleDisplay_Status});
        actionList.push_back({"Seeds", toggleDisplay_Seeds});
        actionList.push_back({"Peers", toggleDisplay_Peers});
        actionList.push_back({"Download Speed", toggleDisplay_DownloadSpeed});
        actionList.push_back({"Upload Speed", toggleDisplay_UploadSpeed});
        actionList.push_back({"ETA", toggleDisplay_ETA});
        actionList.push_back({"Ratio", toggleDisplay_Ratio});
        actionList.push_back({"Tracker", toggleDisplay_Tracker});
        actionList.push_back({"Downloaded", toggleDisplay_Downloaded});
        actionList.push_back({"Uploaded", toggleDisplay_Uploaded});

        torrentTableHeaderMenu = new QMenu(this);

        //iterate through actions and configure menu options and slots
        for (int i = 0; i < actionList.size(); ++i)
        {
            actionList[i].second =
                    new TorrentHeaderCheckbox(
                        actionList[i].first, torrentTableHeaderMenu);

            auto widgetAction = new QWidgetAction(this);
            widgetAction->setDefaultWidget(actionList[i].second);

            //set internal data so it can be used for quick processing in slot
            actionList[i].second->setData(i);

            torrentTable->isColumnHidden(i) ?
                        actionList[i].second->setChecked(false) :
                        actionList[i].second->setChecked(true);

            connect(actionList[i].second,
                    &TorrentHeaderCheckbox::stateChanged, this,
                            &MainWindow::toggleColumnDisplay);

            torrentTableHeaderMenu->addAction(widgetAction);
        }

        isFirstTorrentHeaderMenu = false;
    }

    torrentTableHeaderMenu->popup(torrentTable->mapToGlobal(pos));
}

void MainWindow::toggleColumnDisplay()
{
    emit torrentModel->layoutAboutToBeChanged();

    //get sender action
    TorrentHeaderCheckbox* chkBox =
            qobject_cast<TorrentHeaderCheckbox*>(sender());

    if (chkBox->isChecked())
    {
        //use stored action data to determine position in action list
        torrentTable->showColumn(chkBox->data().toInt());
    }
    else
    {
        torrentTable->hideColumn(chkBox->data().toInt());
    }

    emit torrentModel->layoutChanged();
}

void MainWindow::loadTorrent(std::string filePath, std::string& buffer)
{
    using namespace torrentManipulation;

    if (buffer.empty())
    {
        LOG_F(ERROR, "Torrent is empty!");
        return;
    }

    //create dialog
    addTorInfoDialog = new AddTorrentDialog(filePath, buffer, this);

    //remove question mark from dialog
    addTorInfoDialog->setWindowFlags(
                addTorInfoDialog->windowFlags() &
                ~Qt::WindowContextHelpButtonHint);

    addTorInfoDialog->setMinimumWidth(700);
    addTorInfoDialog->show();

    //connect custom signal from AddTorrentDialog to send modified torrent
    //back to MainWindow and then pass its bencoded data
    //via the loadCreatedTorrent slot
    connect(addTorInfoDialog, &AddTorrentDialog::sendModifiedTorrent, this,
            [this](Torrent modifiedTorrent){
        MainWindow::handleNewTorrent(modifiedTorrent);}
    );
}

void MainWindow::handleNewTorrent(Torrent modifiedTorrent)
{
    //update relevant data models
    torrentModel->addNewTorrent(&modifiedTorrent);
    infoListModel->update();

    //create new trackerTableview and associated model
    //and add to tableview stack
    QPointer<QTableView> dynTrackerTableView = new QTableView;
    QPointer<TrackerTableModel> dynTrackerModel =
            new TrackerTableModel(&ioClient->WorkingTorrents.torrentList.back()->
                                  generalData.trackerList, this);

    //configure new table
    dynTrackerTableView->setModel(dynTrackerModel);
    dynTrackerTableView->resizeColumnsToContents();
    dynTrackerTableView->verticalHeader()->setVisible(false);
    dynTrackerTableView->setShowGrid(false);
    dynTrackerTableView->setAlternatingRowColors(true);
    dynTrackerTableView->setStyleSheet("alternate-background-color: #F0F0F0;");
    dynTrackerTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    dynTrackerTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    //disable bold header text when data is selected
    dynTrackerTableView->horizontalHeader()->setHighlightSections(false);
    //right click menus
    dynTrackerTableView->horizontalHeader()->
            setContextMenuPolicy(Qt::CustomContextMenu);
    dynTrackerTableView->setContextMenuPolicy(Qt::CustomContextMenu);

    //add to struct
    trackerTableData dynTrackerTable{std::move(dynTrackerTableView),
                std::move(dynTrackerModel)};

    //add struct to vector and stack
    trackerTableVec.push_back(dynTrackerTable);
    trackerTableStack->addWidget(dynTrackerTable.trackerTable);

    ////////////////////////////////////////////////////////////////////////////

    //create new QTreeView and associated model
    //and add to contentTreeStack
    QPointer<QTreeView> dynContentTreeView = new QTreeView;
    QPointer<ContentTreeModel> dynContentTreeModel = new ContentTreeModel(
                ioClient, &ioClient->WorkingTorrents.torrentList.back()->fileList,
                this);

    //configure new QTreeView
    dynContentTreeView->setModel(dynContentTreeModel);
    dynContentTreeView->setColumnWidth(0, 500);
    dynContentTreeView->resizeColumnToContents(1);
    dynContentTreeView->setAlternatingRowColors(true);
    dynContentTreeView->setStyleSheet("alternate-background-color: #F0F0F0;");
    dynContentTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    //expand top parent
    dynContentTreeView->expand(dynContentTreeModel->index(0,0));

    //add to struct
    contentTreeData dynContentTree{std::move(dynContentTreeView),
                std::move(dynContentTreeModel)};

    //add struct to vector and stack
    contentTreeVec.push_back(dynContentTree);
    contentTreeStack->addWidget(dynContentTree.contentTreeView);
}

void MainWindow::duplicateTorrentSlot(QString torrentName)
{
    QMessageBox::information(this, "Torrent is already present",
                             QString("Torrent '%1' is already in the transfer "
                             "list. Trackers have been merged").arg(torrentName),
                             QMessageBox::Ok);
}

void MainWindow::on_actionAdd_Torrent_triggered()
{
    addTorrentDialog = new QFileDialog(this);
    addTorrentDialog->setNameFilter(tr("*.torrent"));
    addTorrentDialog->setViewMode(QFileDialog::Detail);
    addTorrentDialog->setFileMode(QFileDialog::ExistingFiles);

    if ( QDialog::Accepted == addTorrentDialog->exec() )
    {
        QStringList filenames = addTorrentDialog->selectedFiles();
        QStringList::const_iterator it = filenames.begin();
        QStringList::const_iterator eIt = filenames.end();
        while ( it != eIt )
        {
            QString fileName = *it++;
            if ( !fileName.isEmpty() )
            {
                std::string buffer = loadFromFile(fileName.toStdString().c_str());
                loadTorrent(fileName.toStdString(), buffer);
            }
        }
    }
}

void MainWindow::on_actionExit_Client_triggered()
{


    const QMessageBox::StandardButton res = QMessageBox::warning(
                this, tr("Exit Client"),
                         "Are you sure you want to exit the client?",
                         QMessageBox::Yes | QMessageBox::No);

    if(res == QMessageBox::Yes)
    {
        QApplication::closeAllWindows();
    }
    else
    {
       return;
    }
}


void MainWindow::on_actionDelete_triggered()
{
    //map proxy selection to source
    auto mappedSelection = proxyModel->mapSelectionToSource(
                torrentTable->selectionModel()->selection());

    //get selected indices
    QModelIndexList selectedViewIdxList =
            mappedSelection.indexes();

    //map selected view indices to source torrentModel rows
    //use QSet to remove duplicates (since multiple indices will share a row)
    QSet<int> selectedSourceRowSet;
    for (auto selectedViewIndex : selectedViewIdxList)
    {
        selectedSourceRowSet.insert(selectedViewIndex.row());
    }

    //transform to QList for simpler sort and iteration
    QList<int> selectedSourceRowList(selectedSourceRowSet.begin(),
                                     selectedSourceRowSet.end());

    //sort by descending row num so deletion doesn't upset indices
    std::sort(selectedSourceRowList.begin(), selectedSourceRowList.end(),
          std::greater<int>());

    //update all relevant data models
    if (!selectedSourceRowList.isEmpty())
    {
        for (auto row : selectedSourceRowList)
        {
            torrentModel->removeTorrent(row);

            //erase from relevant vectors used for QStackedWidget
            trackerTableVec.erase(trackerTableVec.begin() + row);
            contentTreeVec.erase(contentTreeVec.begin() + row);
        }
    }

    infoListModel->update();
}


void Bittorrent::MainWindow::on_actionResume_triggered()
{
    //map proxy selection to source
    auto mappedSelection = proxyModel->mapSelectionToSource(
                torrentTable->selectionModel()->selection());

    //get selected indices
    QModelIndexList selectedViewIdxList =
            mappedSelection.indexes();

    //map selected view indices to source torrentModel rows
    //use QSet to remove duplicates (since multiple indices will share a row)
    QSet<int> selectedSourceRowSet;
    for (auto selectedViewIndex : selectedViewIdxList)
    {
        selectedSourceRowSet.insert(selectedViewIndex.row());
    }

    //transform to QList for simpler sort and iteration
    QList<int> selectedSourceRowList(selectedSourceRowSet.begin(),
                                     selectedSourceRowSet.end());

    //start threads for tracker updates and send signal to update model when done
    if (!selectedSourceRowList.isEmpty())
    {
        for (auto row : selectedSourceRowList)
        {
            //new thread to run in background
            QFuture<void> t1 =
                    QtConcurrent::run(&this->ioClient->WorkingTorrents,
                                      &WorkingTorrents::start,
                                      row);
        }
    }
}

void Bittorrent::MainWindow::on_actionPause_triggered()
{
    //map proxy selection to source
    auto mappedSelection = proxyModel->mapSelectionToSource(
                torrentTable->selectionModel()->selection());

    //get selected indices
    QModelIndexList selectedViewIdxList =
            mappedSelection.indexes();

    //map selected view indices to source torrentModel rows
    //use QSet to remove duplicates (since multiple indices will share a row)
    QSet<int> selectedSourceRowSet;
    for (auto selectedViewIndex : selectedViewIdxList)
    {
        selectedSourceRowSet.insert(selectedViewIndex.row());
    }

    //transform to QList for simpler sort and iteration
    QList<int> selectedSourceRowList(selectedSourceRowSet.begin(),
                                     selectedSourceRowSet.end());

    //start threads for tracker updates and send signal to update model when done
    if (!selectedSourceRowList.isEmpty())
    {
        for (auto row : selectedSourceRowList)
        {
            //new thread to run in background
            QFuture<void> t1 =
                    QtConcurrent::run(&this->ioClient->WorkingTorrents,
                                      &WorkingTorrents::stop,
                                      row);
        }
    }
}


void MainWindow::on_actionTorrent_Creator_triggered()
{
    createTorDialog = new CreateTorrent(this);

    createTorDialog->setWindowTitle("Create a torrent");

    //remove question mark from dialog
    createTorDialog->setWindowFlags(
                createTorDialog->windowFlags() &
                ~Qt::WindowContextHelpButtonHint);

    createTorDialog->show();

    //connect custom signal from CreateTorrent to send created torrent
    //file path back to MainWindow and then load its bencoded data
    //via the loadCreatedTorrent slot
    connect(createTorDialog, &CreateTorrent::sendfilePath, this,
            [this](const QString& fileName){
        MainWindow::loadCreatedTorrent(fileName);}
    );
}

void MainWindow::loadCreatedTorrent(QString filePath)
{
    std::string buffer = loadFromFile(filePath.toStdString().c_str());
    loadTorrent(filePath.toStdString(), buffer);
}

void MainWindow::textFilterChanged()
{
    QRegExp torrentFilterRegExp(searchFilter->text());
    torrentFilterRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterRegExp(torrentFilterRegExp);
}

void MainWindow::on_actionOptions_triggered()
{
    settingsDialog = new SettingsDialog(
                &ioClient->WorkingTorrents.defaultSettings, this);

    settingsDialog->setWindowTitle("Settings");

    //remove question mark from dialog
    settingsDialog->setWindowFlags(
                settingsDialog->windowFlags() &
                ~Qt::WindowContextHelpButtonHint);

    settingsDialog->show();

    //connect custom signal from SettingsDialog to send user-set data
    //back to MainWindow and then apply to WorkingTorrent class members
    connect(settingsDialog, &SettingsDialog::sendModifiedSettings, this,
            [this](const WorkingTorrents::settings& modifiedSettings){
        MainWindow::on_settingsChange(modifiedSettings);}
    );

}

void MainWindow::on_settingsChange(WorkingTorrents::settings modifiedSettings)
{
    //apply settings
    ioClient->WorkingTorrents.defaultSettings.httpPort = modifiedSettings.httpPort;
    ioClient->WorkingTorrents.defaultSettings.udpPort = modifiedSettings.udpPort;
    ioClient->WorkingTorrents.defaultSettings.tcpPort = modifiedSettings.tcpPort;

    ioClient->WorkingTorrents.defaultSettings.maxDLSpeed =
            modifiedSettings.maxDLSpeed;
    ioClient->WorkingTorrents.defaultSettings.maxULSpeed =
            modifiedSettings.maxULSpeed;

    ioClient->WorkingTorrents.defaultSettings.maxSeeders =
            modifiedSettings.maxSeeders;
    ioClient->WorkingTorrents.defaultSettings.maxLeechers =
            modifiedSettings.maxLeechers;

    ioClient->WorkingTorrents.downloadThrottle.maxDataSize =
            modifiedSettings.maxDLSpeed;
    ioClient->WorkingTorrents.uploadThrottle.maxDataSize =
            modifiedSettings.maxULSpeed;


    LOG_F(INFO, "Settings successfully changed by user.");
}

}
