#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QString>
#include <QProgressDialog>
#include "filecopyworker.h"


FileCopyWorker::FileCopyWorker(QObject *parent) : QObject(parent)
{

}

FileCopyWorker::FileCopyWorker(QObject *parent,
                               const QDir photoPath, const QDir videoPath)
    : QObject(parent),
      m_photoPath(photoPath),
      m_videoPath(videoPath)
{

}

QString FileCopyWorker::addUniqueSuffix(const QString &fileName)
{
    // If the file doesn't exist return the same name.
    if (!QFile::exists(fileName)) {
        return fileName;
    }

    QFileInfo fileInfo(fileName);
    QString ret;

    // Split the file into 2 parts - dot+extension, and everything else. For
    // example, "path/file.tar.gz" becomes "path/file"+".tar.gz", while
    // "path/file" (note lack of extension) becomes "path/file"+"".
    QString secondPart = fileInfo.completeSuffix();
    QString firstPart;
    if (!secondPart.isEmpty()) {
        secondPart = "." + secondPart;
        firstPart = fileName.left(fileName.size() - secondPart.size());
    } else {
        firstPart = fileName;
    }

    // Try with an ever-increasing number suffix, until we've reached a file
    // that does not yet exist.
    for (int ii = 1; ; ii++) {
        // Construct the new file name by adding the unique number between the
        // first and second part.
        ret = QString("%1 (%2)%3").arg(firstPart).arg(ii).arg(secondPart);
        // If no file exists with the new name, return it.
        if (!QFile::exists(ret)) {
            return ret;
        }
    }
}

void FileCopyWorker::doWork(const QList<FileCopyInfo> files) {
    QString fullDestination;
    int count = 0;
    for (auto param : files) {
        if (m_abort) {
            m_abort = false;
            break;
        }
        if (param.ext == "jpg") {
            fullDestination = addUniqueSuffix(
                        m_photoPath.absoluteFilePath(param.name));
        } else {
            fullDestination = addUniqueSuffix(
                        m_videoPath.absoluteFilePath(param.name));
        }
        if (QFile(param.path).copy(fullDestination)) {
            ++count;
            emit fileDone(param.index, count);
        }
        QCoreApplication::processEvents();
    }
    emit workDone(count);
}

void FileCopyWorker::abort()
{
    m_abort = true;
}


