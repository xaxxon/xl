#include "logelementlistmodel.h"
#include "QLabel"
#include "QCheckBox"


LogElementListModel::LogElementListModel(std::vector<std::pair<std::string, bool>> & elements) :
    elements(&elements)
{
}



void LogElementListModel::data_changed() {
  this->beginResetModel(); this->endResetModel();

     //QAbstractItemModel::dataChanged()
}


QVariant LogElementListModel::data(const QModelIndex &index, int role) const {
//    std::cerr << fmt::format("data() was called for index: {} role: {}", index.row(), role) << std::endl;

    if (role == Qt::CheckStateRole) {
        return (*this->elements)[index.row()].second ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
    }

    if (role == Qt::DisplayRole) {
        return QVariant::fromValue(QString((*this->elements)[index.row()].first.c_str()));
    } else {
        return QVariant();
    }
}


int LogElementListModel::rowCount(const QModelIndex &parent) const {
//    std::cerr << fmt::format("row count: {}", this->elements->size()) << std::endl;
    return this->elements->size();
}



void LogStatusFileGuiWrapper::open_filename(QString filename) {
    std::cerr << fmt::format("opening filename: {}", filename.toStdString()) << std::endl;
    this->status_file = std::make_unique<xl::LogStatusFile>(filename.toStdString());
}


LogStatusFileGuiWrapper::LogStatusFileGuiWrapper(QString filename, QListView * levelList, QListView * subjectList) :
    status_file(std::make_unique<xl::LogStatusFile>(filename.toStdString())),
    levelList(levelList),
    subjectList(subjectList),
    level_model(status_file->level_names),
    subject_model(status_file->subject_names)
{
    levelList->setModel(&this->level_model);
    subjectList->setModel(&this->subject_model);

    levelList->connect(levelList, &QListView::pressed, [this](const QModelIndex &index){
        status_file->level_names[index.row()].second = !status_file->level_names[index.row()].second;
        status_file->write();
        this->level_model.data_changed();
    });
    subjectList->connect(subjectList, &QListView::pressed, [this](const QModelIndex &index){
        status_file->subject_names[index.row()].second = !status_file->subject_names[index.row()].second;
        status_file->write();
        this->subject_model.data_changed();
    });

}










