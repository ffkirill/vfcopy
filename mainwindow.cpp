#include <QApplication>
#include <QMetaObject>
#include <tuple>
#include <memory>
#include <QProgressDialog>
#include <QMessageBox>
#include <QSemaphore>
#include <QDesktopServices>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QDir>
#include <QStorageInfo>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QDirIterator>
#include <QPixmap>
#include <QFileDialog>
#include <QDebug>
#include "qexifimageheader.h"
#include "filecopyworker.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->drivesListView->setModel(&this->disksModel);
    this->filesModel.setSortRole(USER_ROLE_EXTENSION);
    ui->filesListView->setModel(&this->filesModel);
    connect(&this->disksModel, &QStandardItemModel::itemChanged,
            this, &MainWindow::onDriveCheckedChanged);
    auto args = qApp->arguments();
    if (args.count() > 1) {
        ui->destinationTextEdit->setText(args.at(1));
    }
    on_pushButton_clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}

struct CreateListItemFromFile
{

    typedef QStandardItem* result_type;

    inline QPixmap generatePixmap(const QString& path) {
        QExifImageHeader exif(path);
        QImage thumbnail = exif.thumbnail();
        if (thumbnail.isNull()) {
            thumbnail.load(path);
        }
        return QPixmap::fromImage(thumbnail.scaled(64,
                                                   64,
                                                   Qt::KeepAspectRatio,
                                                   Qt::FastTransformation));
    }

    QStandardItem* operator()(const std::pair<QString, QString>& param)
    {
        const QString& path = param.first;
        const QString& root = param.second;
        QFileInfo fileInfo = QFileInfo(path);
        QString extension = fileInfo.suffix().toLower();
        QStandardItem *item;
        if (extension == "jpg") {
            item = new QStandardItem(generatePixmap(path), fileInfo.fileName());
        } else {
            item = new QStandardItem(m_iconProvider.icon(QFileInfo(path)),
                                     QString("%1 (%2 Mb)").arg(fileInfo.fileName())
                                     .arg(fileInfo.size() / (1024 * 1024)));
        }
        item->setCheckable(true);
        item->setData(Qt::Checked, Qt::CheckStateRole);
        item->setData(fileInfo.fileName(), MainWindow::USER_ROLE_FILE_NAME);
        item->setData(root, MainWindow::USER_ROLE_DRIVE_ROOT);
        item->setData(path, MainWindow::USER_ROLE_FULL_PATH);
        item->setData(extension, MainWindow::USER_ROLE_EXTENSION);
        return item;
    }

    static const QFileIconProvider m_iconProvider;
};

const QFileIconProvider CreateListItemFromFile::m_iconProvider;

void MainWindow::on_pushButton_clicked()
{
    QFileIconProvider iconProvider;
    disksModel.removeRows(0, disksModel.rowCount());
    filesModel.removeRows(0, filesModel.rowCount());
    for (auto volume : QStorageInfo::mountedVolumes()) {
        if (!volume.isReady()) continue;
        QStandardItem *item = new QStandardItem(
            iconProvider.icon(QFileInfo(volume.rootPath())),
            QString("%1 (%2 - %3Gb)")
                    .arg(volume.name())
                    .arg(volume.rootPath())
                    .arg(float(volume.bytesTotal()) / 1000000000, 0, 'f' , 2));
        item->setCheckable(true);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        item->setData(volume.rootPath(), USER_ROLE_DRIVE_ROOT);
        disksModel.appendRow(item);
    }
}

void MainWindow::onDriveCheckedChanged(QStandardItem *item)
{
    QString root = item->data(USER_ROLE_DRIVE_ROOT).toString();
    int checked = item->data(Qt::CheckStateRole).toInt();
    ui->filesListView->setUpdatesEnabled(false);
    if (checked == Qt::Checked) {
        QDirIterator it(root, QStringList() << "*.jpg" << "*.mp4",
                        QDir::Files, QDirIterator::Subdirectories);
        QList<std::pair<QString, QString>> files;
        while (it.hasNext()) {
            files << std::make_pair(it.next(), root);
            ui->statusBar->showMessage(QString("%1 файлов найдено")
                                       .arg(files.count()));
            qApp->processEvents();
        }
        QFuture<QStandardItem* > itemFuture = QtConcurrent::mapped(
            files,
            CreateListItemFromFile());
        QPointer<QFutureWatcher<QStandardItem *>> itemWatcher =
                new QFutureWatcher<QStandardItem *>();
        itemWatcher->setFuture(itemFuture);
        itemWatcher->connect(itemWatcher,
                             &QFutureWatcher<QStandardItem *>::resultReadyAt,
                             [itemFuture, this, itemWatcher](int index) {
            this->onFileListItemCreated(itemFuture.resultAt(index));
        });
        itemWatcher->connect(itemWatcher,
                             &QFutureWatcher<QStandardItem *>::finished,
                             [this, itemWatcher](){
            this->filesModel.sort(0);
            this->ui->filesListView->setUpdatesEnabled(true);
        });

    } else {
        QModelIndexList match = filesModel.match(filesModel.index(0, 0),
                                                 USER_ROLE_DRIVE_ROOT, root,
                                                 -1, Qt::MatchExactly);
        for (int i = match.count() - 1; i > -1; --i)
          filesModel.removeRow(match.at(i).row());
        ui->statusBar->showMessage(QString("%1 файлов найдено")
            .arg(filesModel.rowCount()));
    }
}

void MainWindow::onFileListItemCreated(QStandardItem *item)
{
    filesModel.appendRow(item);
}

void MainWindow::onFileCopyDone(int index, int)
{
    QModelIndex fileIndex = filesModel.index(index, 0);
    filesModel.setData(fileIndex, Qt::Unchecked, Qt::CheckStateRole);
}

void MainWindow::onCopyUndone()
{
    QMessageBox::warning(this, "Внимание", "Не все файлы удалось скопировать. Проверьте подключение карт памяти и повторите попытку.");
}

void MainWindow::on_browseDestinationButton_clicked()
{

    QString dir = QFileDialog::getExistingDirectory(
                this, "Укажите папку назначения",
                ui->destinationTextEdit->toPlainText().trimmed(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->destinationTextEdit->setText(dir+"/");
}

void MainWindow::on_copyFilesButton_clicked()
{
    QModelIndexList match = filesModel.match(filesModel.index(0, 0),
                                             Qt::CheckStateRole, Qt::Checked,
                                             -1, Qt::MatchExactly);
    QString destination = ui->destinationTextEdit->toPlainText().trimmed();

    QDir destDir = QDir(destination);
    QDir videoPath = destDir.absoluteFilePath("video");
    QDir photoPath = destDir.absoluteFilePath("photo");
    QDir root = QDir::root();

    if (match.empty() || destination.isEmpty()) return;

    root.mkpath(videoPath.absolutePath());
    root.mkpath(photoPath.absolutePath());

    QMap<QString, QList<FileCopyInfo>> filesMap;
    std::shared_ptr<QSemaphore> tasksSem = std::make_shared<QSemaphore>();
    std::shared_ptr<QSemaphore> filesSem = std::make_shared<QSemaphore>();
    filesSem->release(match.count());
    for (auto fileIndex : match) {
        QString path = filesModel.data(fileIndex, USER_ROLE_FULL_PATH).toString();
        QString name = filesModel.data(fileIndex, MainWindow::USER_ROLE_FILE_NAME).toString();
        QString ext = filesModel.data(fileIndex, USER_ROLE_EXTENSION).toString();
        QString driveRoot = filesModel.data(fileIndex, USER_ROLE_DRIVE_ROOT).toString();
        filesMap[driveRoot] << FileCopyInfo(fileIndex.row(), driveRoot, path, name, ext);
    }
    for (auto key : filesMap.keys()) {
        auto files = filesMap[key];
        tasksSem->release();
        QPointer<QThread> workerThread = new QThread;
        QPointer<FileCopyWorker> worker = new FileCopyWorker(0, photoPath, videoPath);
        QPointer<QProgressDialog> dialog = new QProgressDialog("Копирование файлов...",
                                                               "Отменить", 0,
                                                               files.count(),
                                                               this);
        dialog->show();
        connect(worker, &FileCopyWorker::fileDone,
                this, &MainWindow::onFileCopyDone);
        connect(worker, &FileCopyWorker::fileDone,
                [dialog, filesSem](int, int count) {
            QMetaObject::invokeMethod(dialog, "setValue", Qt::QueuedConnection,
                                      Q_ARG(int, count));
            filesSem->tryAcquire();
        });
        connect(worker, &FileCopyWorker::workDone, [tasksSem, filesSem, destination, worker, dialog, this](){
            tasksSem->tryAcquire();
            if (tasksSem->available() == 0) {
                if (filesSem->available() != 0) {
                    QMetaObject::invokeMethod(dialog, "close", Qt::QueuedConnection);
                    QMetaObject::invokeMethod(this, "onCopyUndone", Qt::QueuedConnection);
                } else {
                    QUrl url = QUrl::fromLocalFile(destination);
                    QDesktopServices::openUrl(url);
                }
            }
            disconnect(worker, 0, 0, 0);
        });

        worker->moveToThread(workerThread);
        workerThread->start();
        QMetaObject::invokeMethod(worker, "doWork", Qt::QueuedConnection,
                                  Q_ARG(QList<FileCopyInfo>, files));
    }
}

void MainWindow::on_filesListView_doubleClicked(const QModelIndex &index)
{
    QUrl url = QUrl::fromLocalFile(filesModel.data(index, USER_ROLE_FULL_PATH)
        .toString());
    QDesktopServices::openUrl(url);
}

void MainWindow::on_drivesListView_doubleClicked(const QModelIndex &index)
{
    QUrl url = QUrl::fromLocalFile(disksModel.data(index, USER_ROLE_DRIVE_ROOT)
        .toString());
    QDesktopServices::openUrl(url);
}
