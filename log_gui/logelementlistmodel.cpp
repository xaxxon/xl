#include "logelementlistmodel.h"
#include "QLabel"
#include "QCheckBox"
#include "QSignalBlocker"

#include "logelementlistmodel.h"

LogElementListModel::LogElementListModel(std::vector<std::pair<std::string, bool>> & elements) :
    elements(&elements)
{
}


void LogElementListModel::data_changed() {
  this->beginResetModel(); this->endResetModel();
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



int LogElementListModel::rowCount(const QModelIndex &parent) const {
    return this->elements->size();
}




