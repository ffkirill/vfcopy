#ifndef FILECOPYWORKER_H
#define FILECOPYWORKER_H

#include <QObject>
#include <QDir>


struct FileCopyInfo
{
    int index;
    QString driveRoot;
    QString path;
    QString name;
    QString ext;

    FileCopyInfo(){}

    FileCopyInfo(int index,
                 const QString& driveRoot,
                 const QString& path,
                 const QString& name,
                 const QString& ext):
        index(index),
        driveRoot(driveRoot),
        path(path),
        name(name),
        ext(ext) {}
};
Q_DECLARE_METATYPE(FileCopyInfo)
Q_DECLARE_METATYPE(QList<FileCopyInfo>)

class FileCopyWorker : public QObject
{
    Q_OBJECT

private:
    QString addUniqueSuffix(const QString &fileName);
    QDir m_photoPath;
    QDir m_videoPath;

public:
    explicit FileCopyWorker(QObject *parent = 0);
    FileCopyWorker(QObject *parent, const QDir photoPath, const QDir videoPath);

signals:
    void fileDone(int index, int count);
    void workDone(int count);


public slots:
    void doWork(const QList<FileCopyInfo> files);
};

#endif // FILECOPYWORKER_H
