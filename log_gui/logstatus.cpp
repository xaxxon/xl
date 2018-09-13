#include "logstatus.h"
#include "ui_logstatus.h"


void update_master_checkbox(QCheckBox * checkbox, std::variant<bool, std::vector<std::pair<std::string, bool>>> variant) {

    int check_state = -1;
    if (bool * all_status = std::get_if<bool>(&variant)) {
//        std::cerr << fmt::format("updating master checkbox to {}", *all_status) << std::endl;
        check_state = *all_status ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
    } else {

        auto & values = std::get<std::vector<std::pair<std::string, bool>>>(variant);
        for (auto const & pair : values) {
            int this_entry_check_state = pair.second ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
            if (check_state == -1) {
                check_state = this_entry_check_state;
            } else if (check_state != this_entry_check_state) {
                check_state = Qt::CheckState::PartiallyChecked;
                break;
            }
        }
    }
    QSignalBlocker qs(checkbox);
    checkbox->setCheckState((Qt::CheckState)check_state);
}


LogStatus::LogStatus(QString const & filename, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogStatus),
    status_file(std::make_unique<xl::log::LogStatusFile>(filename.toStdString())),
    level_model(status_file->levels),
    subject_model(status_file->subjects)
{
    ui->setupUi(this);

    this->ui->levelList->setModel(&this->level_model);
    this->ui->subjectList->setModel(&this->subject_model);

    QObject::connect(this->ui->regexEdit, &QLineEdit::editingFinished, [this]() {
        this->status_file->regex_filter = this->ui->regexEdit->text().toStdString();
        this->status_file->write();
    });

    this->ui->levelList->connect(this->ui->levelList, &QListView::pressed, [this](const QModelIndex &index){
        status_file->level_vector()[index.row()].second = !status_file->level_vector()[index.row()].second;
        status_file->write();
        this->level_model.data_changed();

        update_master_checkbox(this->ui->allLevels, this->status_file->levels);

    });
    this->ui->allLevels->connect(this->ui->allLevels, &QCheckBox::stateChanged, [this](int checked){
        this->ui->allLevels->setTristate(false);
        if (std::get_if<bool>(&this->status_file->levels)) {
            bool status = checked == Qt::CheckState::Checked;
//            std::cerr << fmt::format("setting levels to {}", status) << std::endl;
            this->status_file->levels = status;
        } else {
            checked = this->ui->allLevels->checkState() == Qt::CheckState::Checked;
            for (auto & [name, status] : this->status_file->level_vector()) {
                status = checked;
            }
        }
        status_file->write();
        this->level_model.data_changed();
    });

    this->ui->subjectList->connect(this->ui->subjectList, &QListView::pressed, [this](const QModelIndex &index){
        status_file->subject_vector()[index.row()].second = !status_file->subject_vector()[index.row()].second;
        status_file->write();
        this->subject_model.data_changed();


        update_master_checkbox(this->ui->allSubjects, this->status_file->subjects);
    });

    this->ui->allSubjects->connect(this->ui->allSubjects, &QCheckBox::stateChanged, [this](int checked){

        this->ui->allSubjects->setTristate(false);

        if (std::get_if<bool>(&this->status_file->subjects)) {
            bool status = checked == Qt::CheckState::Checked;
//            std::cerr << fmt::format("setting subjects to {} - check state: {}", status, checked) << std::endl;
            this->status_file->subjects = status;
        } else {
            checked = this->ui->allSubjects->checkState() == Qt::CheckState::Checked;

            for (auto & [name, status] : this->status_file->subject_vector()) {
                status = checked;
            }
        }
        status_file->write();
        this->subject_model.data_changed();

    });
    this->timer.connect(&this->timer, &QTimer::timeout, [this]{
        if (this->status_file->check()) {
            this->subject_model.data_changed();
            this->level_model.data_changed();
            this->ui->regexEdit->setText(this->status_file->regex_filter.c_str());
        }
    });
    this->timer.start(1000);

    update_master_checkbox(this->ui->allLevels, this->status_file->levels);
    update_master_checkbox(this->ui->allSubjects, this->status_file->subjects);


}

LogStatus::~LogStatus()
{
    delete ui;
}


