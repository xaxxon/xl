#pragma once

#include <memory>
#include "QAbstractListModel"
#include "QListView"
#include "QCheckBox"
#include "QTimer"


/**
 * Model for a vector of pairs of names/bools
 */
class LogElementListModel : public QAbstractListModel {

    std::vector<std::pair<std::string, bool>> * elements;

public:

    LogElementListModel(std::vector<std::pair<std::string, bool>> & elements);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;

    // call this to tell the view to reload data from the model
    void data_changed();
};
