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

//stringize
#define NAMEOF(variable) ((void)variable, #variable)

namespace Bittorrent
{


using namespace utility;

MainWindow::MainWindow(Client* client, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    ioClient(client), isFirstTorrentHeaderMenu{true}
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
    //disable bold header text when data is selected
    torrentTable->horizontalHeader()->setHighlightSections(false);

    model = new TestModel(ioClient, this);

//    progressDelegate* delegate = new progressDelegate(torrentTable);
//    torrentTable->setItemDelegateForColumn(5, delegate);

    //proxyModel->setSourceModel(model);
    torrentTable->setModel(model);
    m_dockWidget1->setWidget(torrentTable);
    m_dockWidget1->show();


    torrentTable->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    torrentTable-> setContextMenuPolicy(Qt::CustomContextMenu);

    connect(torrentTable->horizontalHeader(),
            &QTableView::customContextMenuRequested,
            this, &MainWindow::customHeaderMenuRequested);

    connect(torrentTable, &QTableView::customContextMenuRequested,
            this, &MainWindow::customTorrentSelectRequested);

//    auto tester =
//            new QAbstractItemModelTester(
//                model, QAbstractItemModelTester::FailureReportingMode::Fatal, this);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_rightSideWindow;
}

void MainWindow::customTorrentSelectRequested(const QPoint& pos)
{
    //if right clicked in a valid row (i.e. only when there is data)
    if (torrentTable->indexAt(pos).isValid())
    {
        torrentTableMainMenu = new QMenu(this);
        a_deleteTorrent = new QAction("Delete", this);
        connect(a_deleteTorrent, &QAction::triggered, this,
                &MainWindow::on_actionDelete_triggered);
        torrentTableMainMenu->addAction(a_deleteTorrent);
        torrentTableMainMenu->popup(m_rightSideWindow->mapToGlobal(pos));
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

        torrentTableMainMenu = new QMenu(this);

        //iterate through actions and configure menu options and slots
        for (int i = 0; i < actionList.size(); ++i)
        {
            actionList[i].second =
                    new TorrentHeaderCheckbox(
                        actionList[i].first, torrentTableMainMenu);

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

            torrentTableMainMenu->addAction(widgetAction);
        }

        isFirstTorrentHeaderMenu = false;
    }

    torrentTableMainMenu->popup(torrentTable->mapToGlobal(pos));
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
        LOG_F(INFO, "unhiding action: %d", chkBox->data().toInt());
        torrentTable->showColumn(chkBox->data().toInt());
    }
    else
    {
        LOG_F(INFO, "hiding action: %d", chkBox->data().toInt());
        torrentTable->hideColumn(chkBox->data().toInt());
    }

    emit model->layoutChanged();
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
    QFileDialog dialog(this);
    dialog.setNameFilter(tr("*.torrent"));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::ExistingFiles);

    if ( QDialog::Accepted == dialog.exec() )
    {
        QStringList filenames = dialog.selectedFiles();
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

