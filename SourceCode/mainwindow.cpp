//===================================================================================================================================
//
// Description: simple multithreading GUI folder copier.
// Technologies: Qt 5.14, C++17, GoogleTests, filesystem, thread
// Version: 1.0.0
// Date: July 2022, Sidelnikov Dmitry (c)
//
// The SimpleCopier v 1.0.0 program is a simple multithreaded GUI directory copier. I think the code is far from perfect.
// The development time is 3 days. It is unlikely that there are no bugs in it and I doubt that all corner cases are taken into account.
// Threads for copying are created as many as there are available cores on your CPU. Only errors occurring in threads are logged,
// higher-level errors are output to the GUI via messageboxes. Googletest's created in the latest version of Microsoft Visual Studio.
// It supports them just out of the box very conveniently. Unit tests do not cover all the functions that are needed (7 out of 9)
// from the file copylib.cpp. 10 unit tests were added. There is something to work on in the next version of the program. :)
// All comments in the source code are in English. I tried to apply the best practices known to me.
// The libraries filesystem, thread, C++17 standard are used. Atomics, mutexes, the singleton pattern are used.
// You can improve the quality of the code: attract a testing team, add more unit tests, use static code analyzer,
// add a code review procedure, increase development time. I am open to any comments. :)
//
//===================================================================================================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "copylib.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QString>

#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

using namespace std::chrono_literals;

namespace
{
    const auto guiUpdateInterval { 30ms };

    const QString appName{ "SimpleCopier" };
    const QString appVersion{ "v1.0.0" };
    const QString author{ "Sidelnikov Dmitry" };

}; // namespace

//===================================================================================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle(appName + " " + appVersion + " / " + author);

    // Init ogigin path
    const QString curPath = fs::current_path().string().c_str();
    const QString originPath = "C:\\Temp\\Origin\\Test1";
    if (fs::exists(originPath.toStdString()))
    {
        ui->lineEditOrigin->setText(originPath);
    }
    else
    {
        ui->lineEditOrigin->setText(curPath);
    }

    // Init destination path
    const QString destPath = "C:\\Temp\\Dest";
    if (fs::exists(destPath.toStdString()))
    {
        ui->lineEditDestination->setText(destPath);
    }
    else
    {
        ui->lineEditDestination->setText(curPath);
    }

    hardwConcur = std::thread::hardware_concurrency();
    const QString statusBarMessage = QString("Available hardware concurrency: ") + std::to_string(hardwConcur).c_str() + " threads";
    ui->statusbar->showMessage(statusBarMessage);

    ui->pushButtonCancel->setEnabled(false);
}

//===================================================================================================================================

MainWindow::~MainWindow()
{
    delete ui;
}

//===================================================================================================================================

void MainWindow::on_pushButtonDestination_clicked()
{
    const QString dest = QFileDialog::getExistingDirectory(this, "Select destination directory",
                                                           ui->lineEditDestination->text(),
                                                           QFileDialog::ShowDirsOnly);
    if (!dest.isEmpty())
    {
        ui->lineEditDestination->setText(dest);
    }
}

//===================================================================================================================================

void MainWindow::on_pushButtonOrigin_clicked()
{
    const QString origin = QFileDialog::getExistingDirectory(this, "Select origin directory",
                                                             ui->lineEditOrigin->text(),
                                                             QFileDialog::ShowDirsOnly);
    if (!origin.isEmpty())
    {
        ui->lineEditOrigin->setText(origin);
    }
}

//===================================================================================================================================

void MainWindow::on_pushButtonCancel_clicked()
{
    copyCancel.store(true);
}

//===================================================================================================================================

void MainWindow::on_pushButtonStartCopy_clicked()
{
    const QString origin = ui->lineEditOrigin->text();
    const QString dest = ui->lineEditDestination->text();

    const fs::path fsOrigin = origin.toStdString();
    const fs::path fsDest = dest.toStdString();

    if (!fs::exists(fsOrigin) || !fs::exists(fsDest))
    {
        QMessageBox::warning(this, "Error", "Origin or destanation directories do not exist!");
    }

    if (fs::is_empty(fsOrigin))
    {
        QMessageBox::warning(this, "Error", "Origin directory is empty! Nothing to copy!");
    }

    if (origin != dest && fsOrigin != fsDest)
    {
        if (CopyLib::isEnoughSpace(dest.toStdString(), scopeSize))
        {
            const auto ret = CopyLib::createCopyQueues(origin.toStdString(), dest.toStdString(), hardwConcur, scopeSize, fileNum);
            if (ret)
            {
                ui->pushButtonStartCopy->setEnabled(false);
                ui->pushButtonOrigin->setEnabled(false);
                ui->pushButtonDestination->setEnabled(false);

                const auto start = std::chrono::steady_clock::now();
                
				startCopy();
                CopyLib::removeCopyQueues(hardwConcur);
                
				const auto end = std::chrono::steady_clock::now();
                const auto diff = end - start;
                const auto time = std::chrono::duration <double, std::milli> (diff).count();

                // Update status label
                std::string message;
                if (copyCancel.load())
                {
                    message = "Copy is CANCELED! Copied files: " + std::to_string(copiedFileNum) + " from "
                            + std::to_string(fileNum) + ", Copied size: "
                            + std::to_string(copiedFileSize/1'048'576.0f) + " MBytes, Took time: "
                            + std::to_string(time/1000.0f) + " sec.";

                    // Update progress bar
                    const int value = (copiedFileSize * 100.0f) / scopeSize;
                    ui->progressBar->setValue(value);
                }
                else
                {
                    message = "Copy is DONE. Copied files: " + std::to_string(copiedFileNum) + ", Total size: "
                            + std::to_string(copiedFileSize/1'048'576.0f) + " MBytes, Took time: "
                            + std::to_string(time/1000.0f) + " sec.";
                }
                ui->labelStatus->setText(message.c_str());

                if (CopyLib::isCopyErrorHappened())
                {
                    QMessageBox::warning(this, "Error", "Some files were not copied! Because of lack of permission or files were opened.");
                }

                ui->pushButtonStartCopy->setEnabled(true);
                ui->pushButtonOrigin->setEnabled(true);
                ui->pushButtonDestination->setEnabled(true);
            }
        }
        else
        {
            QMessageBox::warning(this, "Error", "Not enough space on a destination!");
        }
    }
    else
    {
        QMessageBox::warning(this, "Error", "Origin directory must not be equal destinition directory!");
    }
}

//===================================================================================================================================


void MainWindow::startCopy()
{
    copiedFileSize.store(0U);
    copiedFileNum.store(0U);
    copyCancel.store(false);
    ui->progressBar->setValue(0);

    CopyLib::copyDirStructure();

    if (CopyLib::isCopyErrorHappened())
    {
        return;
    }

    if (fileNum == 0U)
    {
        // no files to copy
        return;
    }

    // Start threads
    std::thread ** ppThreads = new (std::nothrow) std::thread * [hardwConcur];
    if (ppThreads == nullptr)
    {
        QMessageBox::warning(this, "Fatal error", QString(__FUNCTION__) + " - Sorry not enought memory, can not alloc memory for threads!");
        return;
    }
    const auto tempDir = fs::temp_directory_path().string();
    for(size_t i = 0U; i < hardwConcur; i++)
    {
        const std::string path = tempDir + CopyLib::getTempFN() + std::to_string(i) + CopyLib::getTempExten();

        ppThreads[i] = new (std::nothrow) std::thread(CopyLib::worker, path,
                                                      std::ref(copiedFileSize),
                                                      std::ref(copiedFileNum),
                                                      std::ref(copyCancel));

        if (ppThreads[i] == nullptr) // Safe start canceling
        {
            QMessageBox::warning(this, "Fatal error", QString(__FUNCTION__) + " - Sorry not enought memory, can not alloc memory for a thread!");
            for(size_t j = 0U; j < i; j++)
            {
                ppThreads[j]->join();
                delete ppThreads[j];
            }
            delete [] ppThreads;
            return;
        }
    }

    ui->pushButtonCancel->setEnabled(true);

    // Update the progress
    while(fileNum != copiedFileNum && !copyCancel.load())
    {
        std::this_thread::sleep_for(guiUpdateInterval);
        QApplication::processEvents();

        // Update info label
        const std::string message = "Copied files: " + std::to_string(copiedFileNum) + " from "
                + std::to_string(fileNum) + ", copied size: "
                + std::to_string(copiedFileSize/1'048'576.0f) + " MBytes";
        ui->labelStatus->setText(message.c_str());

        // Update progress bar
        const int value = (copiedFileSize * 100.0f) / scopeSize;
        ui->progressBar->setValue(value);
    }

    if (!copyCancel.load())
    {
        ui->progressBar->setValue(100);
    }
    ui->pushButtonCancel->setEnabled(false);

    // Finishing the threads
    for(size_t i = 0U; i < hardwConcur; i++)
    {
        ppThreads[i]->join();
        delete ppThreads[i];
    }
    delete [] ppThreads;
}

//===================================================================================================================================
