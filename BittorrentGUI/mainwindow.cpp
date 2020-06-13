#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tableModel.h"
#include "progressDelegate.h"
#include "Decoder.h"
#include "loguru.h"

#include <QtWidgets>
#include <QtConcurrent/QtConcurrent>
#include <iostream>
#include <thread>
#include <sstream>

#include <QAbstractItemModelTester>
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>

//stringize
#define NAMEOF(variable) ((void)variable, #variable)

namespace Bittorrent
{


using namespace utility;

MainWindow::MainWindow(Client* client, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      ioClient(client), isFirstTorrentHeaderMenu{true},
      isFirstTorrentTableMenuData{true},
      isFirstTorrentTableMenuOutside{true}
{
    //General Layout
    ui->setupUi(this);
    this->setWindowTitle("ioTorrent");

    QSplitter *splitter1 = new QSplitter(this);
    splitter1->setOrientation(Qt::Horizontal);

    QTabWidget* leftWidget = new QTabWidget(this);

    QWidget* transfersTab = new QWidget();
    QWidget* logTab = new QWidget();

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
    m_dockWidget1->setTitleBarWidget(new QWidget()); // remove title bar
    m_dockWidget1->setAllowedAreas(Qt::NoDockWidgetArea); // do not allow to dock

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

    setCentralWidget(splitter1);

    // Configure the table view
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

    model = new TestModel(ioClient, this);

    progressDelegate* delegate = new progressDelegate(torrentTable);
    torrentTable->setItemDelegateForColumn(5, delegate);

    //sort/filter
    proxyModel = new TorrentSortFilterProxyModel(ioClient, this);
    proxyModel->setSourceModel(model);
    torrentTable->setModel(proxyModel);
    torrentTable->setSortingEnabled(true);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    m_dockWidget1->setWidget(torrentTable);

    torrentTable->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    torrentTable-> setContextMenuPolicy(Qt::CustomContextMenu);

    //Initialise toolbar
    initToolbar();

    connect(torrentTable->horizontalHeader(),
            &QTableView::customContextMenuRequested,
            this, &MainWindow::customHeaderMenuRequested);

    connect(torrentTable, &QTableView::customContextMenuRequested,
            this, &MainWindow::customTorrentSelectRequested);

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


//    auto tester =
//            new QAbstractItemModelTester(
//                model, QAbstractItemModelTester::FailureReportingMode::Fatal, this);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_rightSideWindow;
}

void MainWindow::initToolbar()
{
    toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    this->addToolBar(Qt::TopToolBarArea, toolbar);

    toolbar_addTorrent = new QAction(QIcon(":/imgs/Icons/addTorrent.png"), "");
    toolbar_addTorrent->setToolTip("Add Torrent");
    toolbar_deleteTorrent = new QAction(QIcon(":/imgs/Icons/deleteTorrent.png"), "");
    toolbar_deleteTorrent->setToolTip("Delete Torrent");
    connect(toolbar_addTorrent, &QAction::triggered, this,
            &MainWindow::on_actionAdd_Torrent_triggered);
    toolbar->addAction(toolbar_addTorrent);
    connect(toolbar_deleteTorrent, &QAction::triggered, this,
            &MainWindow::on_actionDelete_triggered);
    toolbar->addAction(toolbar_deleteTorrent);

    toolbar->addSeparator();

    toolbar_resume = new QAction(QIcon(":/imgs/Icons/resume.png"), "");
    toolbar_resume->setToolTip("Resume");
    toolbar_pause = new QAction(QIcon(":/imgs/Icons/pause.png"), "");
    toolbar_pause->setToolTip("Pause");
    toolbar->addAction(toolbar_resume);
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

    QPointer<QPushButton> btn_showAll =
            new QPushButton("Show All Torrents", this);
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

void MainWindow::trackerListItemSelected(const QModelIndex& index)
{
    emit model->dataChanged(QModelIndex(), QModelIndex());
    if (index.isValid())
    {
        //store infoHashes of torrents associated with selected tracker
        //so that they can be used to filter torrent table rows
        auto torList = ioClient->workingTorrentList;
        auto trackerAdd = torList.trackerTorrentMap.keys().at(index.row());
        auto infoHashes = torList.trackerTorrentMap.value(trackerAdd);
        for (auto str : infoHashes)
        {
            proxyModel->infoHashList.push_back(str);
        }

        //placeholder dummy regex to trigger reapplication of filter
        //infoHashList is not empty so tracker filter will trigger instead of
        //search filter (see subclass function definition)
        proxyModel->setFilterRegExp(QRegExp(""));
    }

    for (auto i : ioClient->workingTorrentList.trackerTorrentMap.value(ioClient->workingTorrentList.trackerTorrentMap.keys().at(index.row())))
    {
        LOG_F(INFO, "torrent: %s", i.toStdString().c_str());
    }


    //clear so that future search filter calls can be safely made
    proxyModel->infoHashList.clear();
}

void MainWindow::customTorrentSelectRequested(const QPoint& pos)
{
    //if right clicked in a valid row (i.e. only where there is data)
    if (torrentTable->indexAt(pos).isValid())
    {
        if (isFirstTorrentTableMenuData)
        {
            torrentTableMainMenuData = new QMenu(this);

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
    emit model->layoutAboutToBeChanged();

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

    emit model->layoutChanged();
}

void MainWindow::loadTorrent(std::string filePath, std::string& buffer)
{
    LOG_F(INFO, "filename: %s", filePath.c_str());
    if (buffer.empty())
    {
        LOG_F(ERROR, "Torrent is empty!");
        return;
    }

    //update all relevant data models
    model->addNewTorrent(filePath, buffer);
    infoListModel->update();
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
                LOG_F(INFO, "filename: %s", fileName.toStdString().c_str());
                std::string buffer = loadFromFile(fileName.toStdString().c_str());
                LOG_F(INFO, "buffer %s", buffer.c_str());

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

    //map selected view indices to source model rows
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
            model->removeTorrent(row);
        }
    }

    infoListModel->update();
}


void MainWindow::on_actionTorrent_Creator_triggered()
{
    createTorDialog = new CreateTorrent(this);

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



}
