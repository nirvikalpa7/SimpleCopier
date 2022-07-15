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

const std::string tempFN = "copy_plan_";
const std::string tempExten = ".txt";

//===================================================================================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("Simple Copier v 1.0.0 / Sidelnikov Dmitry");

    //const QString curPath = fs::current_path().string().c_str();
    const QString path1 = "C:\\Temp\\Origin\\Test1";
    ui->lineEditOrigin->setText(path1);
    const QString path2 = "C:\\Temp\\Dest";
    ui->lineEditDestination->setText(path2);

    hardwConcur = std::thread::hardware_concurrency();
    const QString message = QString("Available hardware concurrency: ") + std::to_string(hardwConcur).c_str() + " threads";
    ui->statusbar->showMessage(message);

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
    const QString dest = QFileDialog::getExistingDirectory(this, "Select destination directory", ui->lineEditDestination->text(),
                                                           QFileDialog::ShowDirsOnly);
    ui->lineEditDestination->setText(dest);
}

//===================================================================================================================================

void MainWindow::on_pushButtonOrigin_clicked()
{
    const QString origin = QFileDialog::getExistingDirectory(this, "Select origin directory", ui->lineEditOrigin->text(),
                                                             QFileDialog::ShowDirsOnly);
    ui->lineEditOrigin->setText(origin);
}

//===================================================================================================================================

void MainWindow::startCopy()
{
    copiedFileSize.store(0U);
    copiedFileNum.store(0U);
    copyCancel.store(false);
    ui->progressBar->setValue(0);

    CopyLib::copyDirStructure();

    if (fileNum == 0U)
    {
        // no files to copy
        return;
    }

    // Start threads
    std::thread ** ppThreads = new (std::nothrow) std::thread * [hardwConcur];
    if (ppThreads == nullptr)
    {
        QMessageBox::warning(this, "Fatal error", "Sorry not enought memory, can not alloc memory!");
        return;
    }
    std::string tempDir = fs::temp_directory_path().string();
    for(size_t i = 0U; i < hardwConcur; i++)
    {
        const std::string path = tempDir + tempFN + std::to_string(i) + tempExten;

        ppThreads[i] = new (std::nothrow) std::thread(CopyLib::worker, path,
                                                      std::ref(copiedFileSize),
                                                      std::ref(copiedFileNum),
                                                      std::ref(copyCancel));

        if (ppThreads[i] == nullptr) // Safe start canceling
        {
            QMessageBox::warning(this, "Fatal error", "Sorry not enought memory, can not alloc memory!");
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
        // Update progress bar
        const int value = (copiedFileSize * 100.0f) / scopeSize;
        ui->progressBar->setValue(value);

        // Update info label
        const std::string message = "Copied files: " + std::to_string(copiedFileNum) + ", bytes: " + std::to_string(copiedFileSize);
        ui->labelStatus->setText(message.c_str());
        std::this_thread::sleep_for(30ms);
        QApplication::processEvents();
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

void MainWindow::on_pushButtonCancel_clicked()
{
    copyCancel.store(true);
}

//===================================================================================================================================