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


namespace Bittorrent
{


using namespace utility;

MainWindow::MainWindow(Client* client, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    ioClient(client), numTorrentTableHeaderMenu{0}
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

    m_dockWidget1 = new QDockWidget("Dock 1", this);
    m_rightSideWindow->addDockWidget(Qt::TopDockWidgetArea, m_dockWidget1);
    m_dockWidget1->setTitleBarWidget(new QWidget()); // remove title bar
    m_dockWidget1->setAllowedAreas(Qt::NoDockWidgetArea); // do not allow to dock

    m_dockWidget2 = new QDockWidget("Dock 2", this);
    m_rightSideWindow->addDockWidget(Qt::BottomDockWidgetArea, m_dockWidget2);
    m_dockWidget2->setTitleBarWidget(new QWidget());
    m_dockWidget2->setAllowedAreas(Qt::NoDockWidgetArea);

    setCentralWidget(splitter1);

    // Configure the table view
    torrentTable = new QTableView(m_dockWidget1);
    //torrentTable->horizontalHeader()->setSectionsMovable(true);
    torrentTable->verticalHeader()->setVisible(false);
    torrentTable->setShowGrid(false);
    torrentTable->setAlternatingRowColors(true);

    torrentTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    torrentTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    model = new TestModel(ioClient, this);

//    progressDelegate* delegate = new progressDelegate(torrentTable);
//    torrentTable->setItemDelegateForColumn(5, delegate);

    //proxyModel->setSourceModel(model);
    torrentTable->setModel(model);
    m_dockWidget1->setWidget(torrentTable);
    m_dockWidget1->show();


    torrentTable->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    torrentTable->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(torrentTable->horizontalHeader(),
            &QTableView::customContextMenuRequested,
            this, &MainWindow::customHeaderMenuRequested);

    connect(torrentTable, &QTableView::customContextMenuRequested,
            this, &MainWindow::customMainMenuRequested);

    auto tester =
            new QAbstractItemModelTester(
                model, QAbstractItemModelTester::FailureReportingMode::Fatal, this);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_rightSideWindow;
}

void MainWindow::customMainMenuRequested(const QPoint& pos)
{
    torrentTableMainMenu = new QMenu(this);
    a_deleteTorrent = new QAction("Delete", this);
    connect(a_deleteTorrent, &QAction::triggered, this,
            &MainWindow::deleteTorrent);
    torrentTableMainMenu->addAction(a_deleteTorrent);
    torrentTableMainMenu->popup(m_rightSideWindow->mapToGlobal(pos));
}

void MainWindow::customHeaderMenuRequested(const QPoint& pos)
{
   // int column=torrentTable->horizontalHeader()->logicalIndexAt(pos);
    if (++numTorrentTableHeaderMenu == 1)
    {
        torrentTableHeaderMenu = new QMenu(this);

        toggleDisplayPriority = new QAction("Priority", this);
        toggleDisplayPriority->setCheckable(true);
        connect(toggleDisplayPriority, &QAction::triggered, this,
                &MainWindow::toggleColumnDisplay);
        if (torrentTable->isColumnHidden(0))
        {
            toggleDisplayPriority->setChecked(false);
        }
        else
        {
            toggleDisplayPriority->setChecked(true);
        }

        toggleDisplayAddedOn = new QAction("Added On", this);
        toggleDisplayAddedOn->setCheckable(true);
        connect(toggleDisplayAddedOn, &QAction::triggered, this,
                &MainWindow::toggleColumnDisplay);
        if (torrentTable->isColumnHidden(1))
        {
            toggleDisplayAddedOn->setChecked(false);
        }
        else
        {
            toggleDisplayAddedOn->setChecked(true);
        }

        toggleDisplayName = new QAction("Name", this);
        toggleDisplayName->setCheckable(true);
        if (torrentTable->isColumnHidden(2))
        {
            toggleDisplayName->setChecked(false);
        }
        else
        {
            toggleDisplayName->setChecked(true);
        }

        toggleDisplaySize = new QAction("Size", this);
        toggleDisplaySize->setCheckable(true);
        if (torrentTable->isColumnHidden(3))
        {
            toggleDisplaySize->setChecked(false);
        }
        else
        {
            toggleDisplaySize->setChecked(true);
        }


        toggleDisplayStatus = new QAction("Status", this);
        toggleDisplayStatus->setCheckable(true);
        if (torrentTable->isColumnHidden(4))
        {
            toggleDisplayStatus->setChecked(false);
        }
        else
        {
            toggleDisplayStatus->setChecked(true);
        }

        toggleDisplayProgress = new QAction("Progress", this);
        toggleDisplayProgress->setCheckable(true);
        if (torrentTable->isColumnHidden(5))
        {
            toggleDisplayProgress->setChecked(false);
        }
        else
        {
            toggleDisplayProgress->setChecked(true);
        }

        toggleDisplaySeeds = new QAction("Seeds", this);
        toggleDisplaySeeds->setCheckable(true);
        if (torrentTable->isColumnHidden(6))
        {
            toggleDisplaySeeds->setChecked(false);
        }
        else
        {
            toggleDisplaySeeds->setChecked(true);
        }

        toggleDisplayPeers = new QAction("Peers", this);
        toggleDisplayPeers->setCheckable(true);
        if (torrentTable->isColumnHidden(7))
        {
            toggleDisplayPeers->setChecked(false);
        }
        else
        {
            toggleDisplayPeers->setChecked(true);
        }

        toggleDisplayDSpeed = new QAction("Download Speed", this);
        toggleDisplayDSpeed->setCheckable(true);
        if (torrentTable->isColumnHidden(8))
        {
            toggleDisplayDSpeed->setChecked(false);
        }
        else
        {
            toggleDisplayDSpeed->setChecked(true);
        }

        toggleDisplayUSpeed = new QAction("Upload Speed", this);
        toggleDisplayUSpeed->setCheckable(true);
        if (torrentTable->isColumnHidden(9))
        {
            toggleDisplayUSpeed->setChecked(false);
        }
        else
        {
            toggleDisplayUSpeed->setChecked(true);
        }

        toggleDisplayETA = new QAction("ETA", this);
        toggleDisplayETA->setCheckable(true);
        if (torrentTable->isColumnHidden(10))
        {
            toggleDisplayETA->setChecked(false);
        }
        else
        {
            toggleDisplayETA->setChecked(true);
        }

        toggleDisplayRatio = new QAction("Ratio", this);
        toggleDisplayRatio->setCheckable(true);
        if (torrentTable->isColumnHidden(11))
        {
            toggleDisplayRatio->setChecked(false);
        }
        else
        {
            toggleDisplayRatio->setChecked(true);
        }

        toggleDisplayTracker = new QAction("Tracker", this);
        toggleDisplayTracker->setCheckable(true);
        if (torrentTable->isColumnHidden(12))
        {
            toggleDisplayTracker->setChecked(false);
        }
        else
        {
            toggleDisplayTracker->setChecked(true);
        }

        toggleDisplayTimeActive = new QAction("Time Active", this);
        toggleDisplayTimeActive->setCheckable(true);
        if (torrentTable->isColumnHidden(13))
        {
            toggleDisplayTimeActive->setChecked(false);
        }
        else
        {
            toggleDisplayTimeActive->setChecked(true);
        }

        toggleDisplayDownloaded = new QAction("Downloaded", this);
        toggleDisplayDownloaded->setCheckable(true);
        if (torrentTable->isColumnHidden(14))
        {
            toggleDisplayDownloaded->setChecked(false);
        }
        else
        {
            toggleDisplayDownloaded->setChecked(true);
        }

        toggleDisplayUploaded = new QAction("Uploaded", this);
        toggleDisplayUploaded->setCheckable(true);
        connect(toggleDisplayUploaded, &QAction::triggered, this,
                &MainWindow::toggleColumnDisplay);
        if (torrentTable->isColumnHidden(15))
        {
            toggleDisplayUploaded->setChecked(false);
        }
        else
        {
            toggleDisplayUploaded->setChecked(true);
        }

        torrentTableHeaderMenu->addAction(toggleDisplayPriority);
        torrentTableHeaderMenu->addAction(toggleDisplayAddedOn);
        torrentTableHeaderMenu->addAction(toggleDisplayName);
        torrentTableHeaderMenu->addAction(toggleDisplaySize);
        torrentTableHeaderMenu->addAction(toggleDisplayProgress);
        torrentTableHeaderMenu->addAction(toggleDisplayStatus);
        torrentTableHeaderMenu->addAction(toggleDisplaySeeds);
        torrentTableHeaderMenu->addAction(toggleDisplayPeers);
        torrentTableHeaderMenu->addAction(toggleDisplayDSpeed);
        torrentTableHeaderMenu->addAction(toggleDisplayUSpeed);
        torrentTableHeaderMenu->addAction(toggleDisplayETA);
        torrentTableHeaderMenu->addAction(toggleDisplayRatio);
        torrentTableHeaderMenu->addAction(toggleDisplayTracker);
        torrentTableHeaderMenu->addAction(toggleDisplayTimeActive);
        torrentTableHeaderMenu->addAction(toggleDisplayDownloaded);
        torrentTableHeaderMenu->addAction(toggleDisplayUploaded);
    }

    torrentTableHeaderMenu->popup(m_rightSideWindow->mapToGlobal(pos));
}

void MainWindow::toggleColumnDisplay(bool checked)
{



    //get sender action
    QAction* action = qobject_cast<QAction*>(sender());
    qDebug() << "Triggered: " << action->text();
    qDebug() << "Checked: " << action->isChecked();
    //action changes state after clicking, so need to update visibility
    //according to new state
    if (action->isChecked())
    {
        if (action == toggleDisplayPriority)
        {
            torrentTable->showColumn(0);
        }
        else if (action == toggleDisplayAddedOn)
        {
            torrentTable->setColumnHidden(1, false);
        }
        else if (action == toggleDisplayName)
        {
            torrentTable->setColumnHidden(2, false);
        }
        else if (action == toggleDisplaySize)
        {
            torrentTable->setColumnHidden(3, false);
        }
        else if (action == toggleDisplayStatus)
        {
            torrentTable->setColumnHidden(4, false);
        }
        else if (action == toggleDisplayProgress)
        {
            torrentTable->setColumnHidden(5, false);
        }
        else if (action == toggleDisplaySeeds)
        {
            torrentTable->setColumnHidden(6, false);
        }
        else if (action == toggleDisplayPeers)
        {
            torrentTable->setColumnHidden(7, false);
        }
        else if (action == toggleDisplayDSpeed)
        {
            torrentTable->setColumnHidden(8, false);
        }
        else if (action == toggleDisplayUSpeed)
        {
            torrentTable->setColumnHidden(9, false);
        }
        else if (action == toggleDisplayETA)
        {
            torrentTable->setColumnHidden(10, false);
        }
        else if (action == toggleDisplayRatio)
        {
            torrentTable->setColumnHidden(11, false);
        }
        else if (action == toggleDisplayTracker)
        {
            torrentTable->setColumnHidden(12, false);
        }
        else if (action == toggleDisplayTimeActive)
        {
            torrentTable->setColumnHidden(13, false);
        }
        else if (action == toggleDisplayDownloaded)
        {
            torrentTable->setColumnHidden(14, false);
        }
        else if (action == toggleDisplayUploaded)
        {
            torrentTable->setColumnHidden(15, false);
        }

    }
    else
    {
        if (action == toggleDisplayPriority)
        {
            torrentTable->hideColumn(0);
        }
        else if (action == toggleDisplayAddedOn)
        {
            torrentTable->hideColumn(1);
        }
        else if (action == toggleDisplayName)
        {
            torrentTable->setColumnHidden(2, true);
        }
        else if (action == toggleDisplaySize)
        {
            torrentTable->setColumnHidden(3, true);
        }
        else if (action == toggleDisplayStatus)
        {
            torrentTable->setColumnHidden(4, true);
        }
        else if (action == toggleDisplayProgress)
        {
            torrentTable->setColumnHidden(5, true);
        }
        else if (action == toggleDisplaySeeds)
        {
            torrentTable->setColumnHidden(6, true);
        }
        else if (action == toggleDisplayPeers)
        {
            torrentTable->setColumnHidden(7, true);
        }
        else if (action == toggleDisplayDSpeed)
        {
            torrentTable->setColumnHidden(8, true);
        }
        else if (action == toggleDisplayUSpeed)
        {
            torrentTable->setColumnHidden(9, true);
        }
        else if (action == toggleDisplayETA)
        {
            torrentTable->setColumnHidden(10, true);
        }
        else if (action == toggleDisplayRatio)
        {
            torrentTable->setColumnHidden(11, true);
        }
        else if (action == toggleDisplayTracker)
        {
            torrentTable->setColumnHidden(12, true);
        }
        else if (action == toggleDisplayTimeActive)
        {
            torrentTable->setColumnHidden(13, true);
        }
        else if (action == toggleDisplayDownloaded)
        {
            torrentTable->setColumnHidden(14, true);
        }
        else if (action == toggleDisplayUploaded)
        {
            torrentTable->setColumnHidden(15, true);
        }
    }
}

void MainWindow::loadTorrent(std::string fileName, std::string& buffer)
{
    auto myid = std::this_thread::get_id();
    std::stringstream ss;
    ss << myid;
    std::string mystring = ss.str();

    LOG_F(INFO, "Current thread: %s", mystring.c_str());
    LOG_F(INFO, "filename: %s", fileName.c_str());
    if (buffer.empty())
    {
        LOG_F(ERROR, "Torrent is empty!");
        return;
    }

    model->addNewTorrent(fileName, buffer);
}

void MainWindow::on_actionAdd_Torrent_triggered()
{

    QString fileName = QFileDialog::getOpenFileName(
                this, tr("Choose a torrent file"), "", tr("*.torrent"));

    if (fileName.isEmpty())
    {
        return;
    }
    else
    {
        QFile file(fileName);

        if(!file.open(QIODevice::ReadOnly | QFile::Text))
        {
            QMessageBox::information(this, tr("Unable to open file: "),
                                 file.errorString());
            return;
        }

        LOG_F(INFO, "filename: %s", fileName.toStdString().c_str());
        std::string buffer = loadFromFile(fileName.toStdString().c_str());
        LOG_F(INFO, "buffer %s", buffer.c_str());

        loadTorrent(fileName.toStdString(), buffer);

        file.close();
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
        this->close();
    }
    else
    {
       //
    }
}

void MainWindow::deleteTorrent()
{
    QModelIndexList selection =
            torrentTable->selectionModel()->selectedRows();

    //sort by descending row num so deletion doesn't upset indices
    std::sort(selection.begin(), selection.end(),
              [](const QModelIndex& r1, const QModelIndex& r2){
        return r1.row() > r2.row();});

    for(int i=0; i<selection.count(); i++)
    {
        QModelIndex index = selection.at(i);
        model->removeTorrent(index.row());
    }
}



void Bittorrent::MainWindow::on_actionDelete_triggered()
{
    QModelIndexList selection =
            torrentTable->selectionModel()->selectedRows();

    //sort by descending row num so deletion doesn't upset indices
    std::sort(selection.begin(), selection.end(),
              [](const QModelIndex& r1, const QModelIndex& r2){
        return r1.row() > r2.row();});

    for(int i=0; i<selection.count(); i++)
    {
        QModelIndex index = selection.at(i);
        model->removeTorrent(index.row());
    }
}



}

