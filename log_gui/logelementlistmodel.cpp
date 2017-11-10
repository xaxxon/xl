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




LogStatusFileGuiWrapper::LogStatusFileGuiWrapper(QString filename, 
                                                 QListView * levelList, QCheckBox * allLevels, 
                                                 QListView * subjectList, QCheckBox * allSubjects) :
    status_file(std::make_unique<xl::LogStatusFile>(filename.toStdString())),
    levelList(levelList),
    allLevels(allLevels),
    subjectList(subjectList),
    allSubjects(allSubjects),
    level_model(status_file->level_names),
    subject_model(status_file->subject_names)
{

    std::cerr << fmt::format("creating log status file gui wrapper") << std::endl;

    levelList->setModel(&this->level_model);
    subjectList->setModel(&this->subject_model);

    levelList->connect(levelList, &QListView::pressed, [this](const QModelIndex &index){
        status_file->level_names[index.row()].second = !status_file->level_names[index.row()].second;
        status_file->write();
        this->level_model.data_changed();
    });
    allLevels->connect(allLevels, &QCheckBox::toggled, [this](bool checked){
        for (auto & [name, status] : this->status_file->level_names) {
            status = checked;
        }
        status_file->write();
        this->level_model.data_changed();
    });

    subjectList->connect(subjectList, &QListView::pressed, [this](const QModelIndex &index){
        status_file->subject_names[index.row()].second = !status_file->subject_names[index.row()].second;
        status_file->write();
        this->subject_model.data_changed();
    });

    allSubjects->connect(allSubjects, &QCheckBox::toggled, [this](bool checked){
        for (auto & [name, status] : this->status_file->subject_names) {
            status = checked;
        }
        status_file->write();
        this->subject_model.data_changed();
    });
    this->timer.connect(&this->timer, &QTimer::timeout, [this]{
//        std::cerr << fmt::format("checking status file:") ;
        if (this->status_file->check()) {
//            std::cerr << fmt::format("changed") << std::endl;
            this->subject_model.data_changed();
            this->level_model.data_changed();
        } else {
//            std::cerr << fmt::format("not changed") << std::endl;
        }
    });
    this->timer.start(1000);
}

LogStatusFileGuiWrapper::~LogStatusFileGuiWrapper() {
    std::cerr << fmt::format("destroying log status file gui wrapper") << std::endl;
    levelList->disconnect();
    allLevels->disconnect();
    subjectList->disconnect();
    allSubjects->disconnect();

}




