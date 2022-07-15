#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <atomic>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_pushButtonDestination_clicked();

    void on_pushButtonOrigin_clicked();

    void on_pushButtonStartCopy_clicked();

    void on_pushButtonCancel_clicked();

private:

    void startCopy(); // GUI fun to start copy

    Ui::MainWindow *ui;

    uint64_t scopeSize{ 0U }; // Size all files to copy
    uint64_t fileNum{ 0U };   // Files number to copy

    // Copied
    std::atomic<uint64_t> copiedFileSize{ 0U };
    std::atomic<uint64_t> copiedFileNum{ 0U };
    std::atomic<bool> copyCancel{ false };

    // Available CPU cores
    uint32_t hardwConcur{ 0U };
};
#endif // MAINWINDOW_H
