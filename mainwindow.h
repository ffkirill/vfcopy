#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    QStandardItemModel disksModel;
    QStandardItemModel filesModel;
    static const int USER_ROLE_DRIVE_ROOT = Qt::UserRole;
    static const int USER_ROLE_FULL_PATH = Qt::UserRole + 1;
    static const int USER_ROLE_EXTENSION = Qt::UserRole + 2;
    static const int USER_ROLE_FILE_SIZE = Qt::UserRole + 3;
    static const int USER_ROLE_FILE_NAME = Qt::UserRole + 4;

    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void onDriveCheckedChanged(QStandardItem *item);
    void onFileListItemCreated(QStandardItem *item);
    void onFileCopyDone(int index, int _);
    void onCopyUndone();

    void on_browseDestinationButton_clicked();

    void on_copyFilesButton_clicked();

    void on_filesListView_doubleClicked(const QModelIndex &index);

    void on_drivesListView_doubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
