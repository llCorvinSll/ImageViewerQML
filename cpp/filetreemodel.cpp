#include "filetreemodel.h"
#include <QDirModel>
#include <QDebug>
#include <QQmlContext>


class FileTreeModelPrivate
{
public:
    FileTreeModelPrivate()
        : sortField(FileTreeModel::Name), sortReversed(false), count(0) {
        nameFilters << QLatin1String("*");
    }

    void updateSorting() {
        QDir::SortFlags flags = 0;
        switch(sortField) {
        case FileTreeModel::Unsorted:
            flags |= QDir::Unsorted;
            break;
        case FileTreeModel::Name:
            flags |= QDir::Name;
            break;
        case FileTreeModel::Time:
            flags |= QDir::Time;
            break;
        case FileTreeModel::Size:
            flags |= QDir::Size;
            break;
        case FileTreeModel::Type:
            flags |= QDir::Type;
            break;
        }

        if (sortReversed)
            flags |= QDir::Reversed;

        model.setSorting(flags);
    }

    QDirModel model;
    QUrl folder;
    QStringList nameFilters;
    QModelIndex folderIndex;
    FileTreeModel::SortField sortField;
    bool sortReversed;
    int count;
};

FileTreeModel::FileTreeModel(QObject *parent)
    : QAbstractListModel(parent)
{
    d = new FileTreeModelPrivate();
    d->model.setFilter(QDir::AllDirs | QDir::Files | QDir::Drives | QDir::NoDotAndDotDot);
    connect(&d->model, SIGNAL(rowsInserted(const QModelIndex&,int,int))
            , this, SLOT(inserted(const QModelIndex&,int,int)));
    connect(&d->model, SIGNAL(rowsRemoved(const QModelIndex&,int,int))
            , this, SLOT(removed(const QModelIndex&,int,int)));
    connect(&d->model, SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&))
            , this, SLOT(handleDataChanged(const QModelIndex&,const QModelIndex&)));
    connect(&d->model, SIGNAL(modelReset()), this, SLOT(refresh()));
    connect(&d->model, SIGNAL(layoutChanged()), this, SLOT(refresh()));
}

FileTreeModel::~FileTreeModel()
{
    delete d;
}

QHash<int, QByteArray> FileTreeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[FileNameRole] = "fileName";
    roles[FilePathRole] = "filePath";
    return roles;
}

QVariant FileTreeModel::data(const QModelIndex &index, int role) const
{
    QVariant rv;
    QModelIndex modelIndex = d->model.index(index.row(), 0, d->folderIndex);
    if (modelIndex.isValid()) {
        if (role == FileNameRole)
            rv = d->model.data(modelIndex, QDirModel::FileNameRole).toString();
        else if (role == FilePathRole)
            rv = QUrl::fromLocalFile(d->model.data(modelIndex, QDirModel::FilePathRole).toString());
    }
    return rv;
}

int FileTreeModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return d->count;
}

QUrl FileTreeModel::folder() const
{
    return d->folder;
}

void FileTreeModel::setFolder(const QUrl &folder)
{
    if (folder == d->folder)
        return;
    QModelIndex index = d->model.index(folder.toLocalFile());
    if ((index.isValid() && d->model.isDir(index)) || folder.toLocalFile().isEmpty()) {

        d->folder = folder;
        QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection);
        emit folderChanged();
    }
}

QUrl FileTreeModel::parentFolder() const
{
    QString localFile = d->folder.toLocalFile();
    if (!localFile.isEmpty()) {
        QDir dir(localFile);
#if defined(Q_OS_SYMBIAN) || defined(Q_OS_WIN)
        if (dir.isRoot())
            dir.setPath("");
        else
#endif
            dir.cdUp();
        localFile = dir.path();
    } else {
        int pos = d->folder.path().lastIndexOf(QLatin1Char('/'));
        if (pos == -1)
            return QUrl();
        localFile = d->folder.path().left(pos);
    }
    return QUrl::fromLocalFile(localFile);
}

QStringList FileTreeModel::nameFilters() const
{
    return d->nameFilters;
}

void FileTreeModel::setNameFilters(const QStringList &filters)
{
    d->nameFilters = filters;
    d->model.setNameFilters(d->nameFilters);
}

void FileTreeModel::classBegin()
{
}

void FileTreeModel::componentComplete()
{
    if (!d->folder.isValid() || d->folder.toLocalFile().isEmpty() || !QDir().exists(d->folder.toLocalFile()))
        setFolder(QUrl(QLatin1String("file://")+QDir::currentPath()));

    if (!d->folderIndex.isValid())
        QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection);
}

FileTreeModel::SortField FileTreeModel::sortField() const
{
    return d->sortField;
}

void FileTreeModel::setSortField(SortField field)
{
    if (field != d->sortField) {
        d->sortField = field;
        d->updateSorting();
    }
}

bool FileTreeModel::sortReversed() const
{
    return d->sortReversed;
}

void FileTreeModel::setSortReversed(bool rev)
{
    if (rev != d->sortReversed) {
        d->sortReversed = rev;
        d->updateSorting();
    }
}

bool FileTreeModel::isFolder(int index) const
{
    if (index != -1) {
        QModelIndex idx = d->model.index(index, 0, d->folderIndex);
        if (idx.isValid())
            return d->model.isDir(idx);
    }
    return false;
}

void FileTreeModel::refresh()
{
    d->folderIndex = QModelIndex();
    if (d->count) {
        emit beginRemoveRows(QModelIndex(), 0, d->count-1);
        d->count = 0;
        emit endRemoveRows();
    }
    d->folderIndex = d->model.index(d->folder.toLocalFile());
    int newcount = d->model.rowCount(d->folderIndex);
    if (newcount) {
        emit beginInsertRows(QModelIndex(), 0, newcount-1);
        d->count = newcount;
        emit endInsertRows();
    }
}

void FileTreeModel::inserted(const QModelIndex &index, int start, int end)
{
    if (index == d->folderIndex) {
        emit beginInsertRows(QModelIndex(), start, end);
        d->count = d->model.rowCount(d->folderIndex);
        emit endInsertRows();
    }
}

void FileTreeModel::removed(const QModelIndex &index, int start, int end)
{
    if (index == d->folderIndex) {
        emit beginRemoveRows(QModelIndex(), start, end);
        d->count = d->model.rowCount(d->folderIndex);
        emit endRemoveRows();
    }
}

void FileTreeModel::handleDataChanged(const QModelIndex &start, const QModelIndex &end)
{
    if (start.parent() == d->folderIndex)
        emit dataChanged(index(start.row(),0), index(end.row(),0));
}

bool FileTreeModel::showDirs() const
{
    return d->model.filter() & QDir::AllDirs;
}

void  FileTreeModel::setShowDirs(bool on)
{
    if (!(d->model.filter() & QDir::AllDirs) == !on)
        return;
    if (on)
        d->model.setFilter(d->model.filter() | QDir::AllDirs | QDir::Drives);
    else
        d->model.setFilter(d->model.filter() & ~(QDir::AllDirs | QDir::Drives));
}

bool FileTreeModel::showDotAndDotDot() const
{
    return !(d->model.filter() & QDir::NoDotAndDotDot);
}

void  FileTreeModel::setShowDotAndDotDot(bool on)
{
    if (!(d->model.filter() & QDir::NoDotAndDotDot) == on)
        return;
    if (on)
        d->model.setFilter(d->model.filter() & ~QDir::NoDotAndDotDot);
    else
        d->model.setFilter(d->model.filter() | QDir::NoDotAndDotDot);
}

bool FileTreeModel::showOnlyReadable() const
{
    return d->model.filter() & QDir::Readable;
}

void FileTreeModel::setShowOnlyReadable(bool on)
{
    if (!(d->model.filter() & QDir::Readable) == !on)
        return;
    if (on)
        d->model.setFilter(d->model.filter() | QDir::Readable);
    else
        d->model.setFilter(d->model.filter() & ~QDir::Readable);
}
