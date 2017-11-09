#pragma once

#include <memory>
#include "QAbstractListModel"
#include "QListView"

#define XL_USE_PCRE
#include <xl/regex/regexer.h>

#include <xl/log.h>


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



/**
 * A meta-model for the models for level and subject tree views
 */
class LogStatusFileGuiWrapper
{
private:
    std::unique_ptr<xl::LogStatusFile> status_file;
    QListView * levelList;
    QListView * subjectList;

    LogElementListModel level_model;
    LogElementListModel subject_model;

public:
    LogStatusFileGuiWrapper(QString filename, QListView * levelList, QListView * subjectList);
    void open_filename(QString filename);


};

