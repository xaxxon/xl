#include "logelementlistmodel.h"
#include "QLabel"
#include "QCheckBox"
#include "QSignalBlocker"

LogElementListModel::LogElementListModel(std::vector<std::pair<std::string, bool>> & elements) :
    elements(&elements)
{
}


void LogElementListModel::data_changed() {
  this->beginResetModel(); this->endResetModel();

     //QAbstractItemModel::dataChanged()
}


QVariant LogElementListModel::data(const QModelIndex &index, int role) const {

    if (role == Qt::CheckStateRole) {
        return (*this->elements)[index.row()].second ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
    }

    if (role == Qt::DisplayRole) {
        return QVariant::fromValue(QString((*this->elements)[index.row()].first.c_str()));
    } else {
        return QVariant();
    }
}

void update_master_checkbox(QCheckBox * checkbox, std::vector<std::pair<std::string, bool>> values) {
    int check_state = -1;
    for (auto const & pair : values) {
        int this_entry_check_state = pair.second ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
        if (check_state == -1) {
            check_state = this_entry_check_state;
        } else if (check_state != this_entry_check_state) {
            check_state = Qt::CheckState::PartiallyChecked;
            break;
        }
    }
    QSignalBlocker qs(checkbox);
    checkbox->setCheckState((Qt::CheckState)check_state);
}


int LogElementListModel::rowCount(const QModelIndex &parent) const {
    return this->elements->size();
}


LogStatusFileGuiWrapper::LogStatusFileGuiWrapper(QString filename, 
                                                 QListView * levelList, QCheckBox * allLevels, 
                                                 QListView * subjectList, QCheckBox * allSubjects) :
    status_file(std::make_unique<xl::log::LogStatusFile>(filename.toStdString())),
    levelList(levelList),
    allLevels(allLevels),
    subjectList(subjectList),
    allSubjects(allSubjects),
    level_model(status_file->level_names),
    subject_model(status_file->subject_names)
{
    levelList->setModel(&this->level_model);
    subjectList->setModel(&this->subject_model);

    levelList->connect(levelList, &QListView::pressed, [this](const QModelIndex &index){
        status_file->level_names[index.row()].second = !status_file->level_names[index.row()].second;
        status_file->write();
        this->level_model.data_changed();

        update_master_checkbox(this->allLevels, this->status_file->level_names);

    });
    allLevels->connect(allLevels, &QCheckBox::stateChanged, [this](int checked){
        this->allLevels->setTristate(false);
        checked = this->allLevels->checkState() == Qt::CheckState::Checked;
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


        update_master_checkbox(this->allSubjects, this->status_file->subject_names);
    });

    allSubjects->connect(allSubjects, &QCheckBox::stateChanged, [this](int checked){

        this->allSubjects->setTristate(false);
        checked = this->allSubjects->checkState() == Qt::CheckState::Checked;

        for (auto & [name, status] : this->status_file->subject_names) {
            status = checked;
        }
        status_file->write();
        this->subject_model.data_changed();
    });
    this->timer.connect(&this->timer, &QTimer::timeout, [this]{
        if (this->status_file->check()) {
            this->subject_model.data_changed();
            this->level_model.data_changed();
        }
    });
    this->timer.start(1000);

    update_master_checkbox(this->allLevels, this->status_file->level_names);
    update_master_checkbox(this->allSubjects, this->status_file->subject_names);

}

LogStatusFileGuiWrapper::~LogStatusFileGuiWrapper() {
    levelList->disconnect();
    allLevels->disconnect();
    subjectList->disconnect();
    allSubjects->disconnect();

}




