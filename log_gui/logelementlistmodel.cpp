#include "logelementlistmodel.h"
#include "QLabel"
#include "QCheckBox"
#include "QSignalBlocker"

#include "logelementlistmodel.h"

LogElementListModel::LogElementListModel(std::variant<bool, std::vector<std::pair<std::string, bool>>> & elements) :
    elements(&elements)
{
}


void LogElementListModel::data_changed() {
  this->beginResetModel(); this->endResetModel();
}


QVariant LogElementListModel::data(const QModelIndex &index, int role) const {

    if (role == Qt::CheckStateRole) {
        if (auto elements = std::get_if<std::vector<std::pair<std::string, bool>>>(this->elements)) {
            return (*elements)[index.row()].second ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
        } else {
            return std::get<bool>(*this->elements);
        }
    }

    if (role == Qt::DisplayRole) {
        if (auto elements = std::get_if<std::vector<std::pair<std::string, bool>>>(this->elements)) {
            return (*elements)[index.row()].first.c_str();
        }
    }
    return QVariant();

}



int LogElementListModel::rowCount(const QModelIndex &parent) const {

    if (auto elements = std::get_if<std::vector<std::pair<std::string, bool>>>(this->elements)) {
        return elements->size();
    } else {
        return 0;
    }
}




