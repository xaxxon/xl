#pragma once

#include <memory>
#include "QAbstractListModel"
#include "QListView"
#include "QCheckBox"
#include "QTimer"


#define XL_USE_PCRE
#include <xl/regex/regexer.h>

#include <xl/log.h>




class ConnectGuard {

public:
    virtual ~ConnectGuard() {}
};

template<class Sender, class SenderFunction, class Receiver, class ReceiverFunction>
class ConnectGuardImpl : public ConnectGuard {

    Sender sender;
    SenderFunction sender_function;

    Receiver receiver;
    ReceiverFunction receiver_function;

public:
    ConnectGuardImpl(Sender sender, SenderFunction sender_function, Receiver receiver, ReceiverFunction receiver_function) :
        sender(sender), sender_function(sender_function),
        receiver(receiver), receiver_function(receiver_function) {
        QObject::connect(sender, sender_function, receiver, receiver_function);
    }

    ~ConnectGuardImpl(){
        QObject::disconnect(sender, sender_function, receiver, receiver_function);
    }
};

class ConnectionList {
    std::vector<std::unique_ptr<ConnectGuard>> connections;

public:

    template<class Sender, class SenderFunction, class Receiver, class ReceiverFunction>
    void connect(Sender sender, SenderFunction sender_function, Receiver receiver, ReceiverFunction receiver_function) {
        this->connections.push_back(std::make_unique<ConnectGuardImpl>(sender, sender_function, receiver, receiver_function));
    }

    void clear() {
        this->connections.clear();
    }
};

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
    std::unique_ptr<xl::log::LogStatusFile> status_file;
    QListView * levelList;
    QCheckBox * allLevels;
    QListView * subjectList;
    QCheckBox * allSubjects;

    LogElementListModel level_model;
    LogElementListModel subject_model;

    QTimer timer;

    ConnectionList connections;


public:
    LogStatusFileGuiWrapper(QString filename, QListView * levelList, QCheckBox * allLevels, QListView * subjectList, QCheckBox * allSubjects);
    ~LogStatusFileGuiWrapper();


};

