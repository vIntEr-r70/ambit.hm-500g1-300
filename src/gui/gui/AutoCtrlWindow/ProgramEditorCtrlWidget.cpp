#include "ProgramEditorCtrlWidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>

#include "Gui/Widgets/KbPushButton.h"

ProgramEditorCtrlWidget::ProgramEditorCtrlWidget(QWidget* parent)
    : QFrame(parent)
{
    QFrame::setFrameStyle(QFrame::StyledPanel);

    QVBoxLayout* layout = new QVBoxLayout(this);

    {
        QHBoxLayout* hL = new QHBoxLayout();

        QLabel* lbl = new QLabel(this);
        lbl->setText("Название программы: ");
        hL->addWidget(lbl);

        kbBtnName_ = new KbPushButton(this, Interact::KbAscii);
        connect(kbBtnName_, SIGNAL(kbAccept()), this, SLOT(onProgramNameSet()));
        hL->addWidget(kbBtnName_);

        hL->setStretch(1, 1);
        layout->addLayout(hL);
    }

    {
        QHBoxLayout* hL = new QHBoxLayout();

        QLabel* lbl = new QLabel(this);
        lbl->setText("Комментарии: ");
        hL->addWidget(lbl);

        kbBtnComments_ = new KbPushButton(this, Interact::KbAscii);
        connect(kbBtnComments_, SIGNAL(kbAccept()), this, SLOT(onCommentsSet()));
        hL->addWidget(kbBtnComments_);

        hL->setStretch(1, 1);

        layout->addLayout(hL);
    }

    {
        QHBoxLayout* hL = new QHBoxLayout();

        btnMakeEdit_ = createButton(":/EditFile");
        connect(btnMakeEdit_, SIGNAL(clicked(bool)), this, SLOT(onMakeEdit()));
        hL->addWidget(btnMakeEdit_);

        QList<QPair<QString, TableAc>> tableAcBtns = {
            { ":/AddOp",        AddAbsoluteRecord   },
            { ":/AddOp",        AddRelativeRecord   },
            { ":/AddPause",     AddPause            },
            { ":/AddTimedFc",   AddTimedFC          },
            { ":/AddGoTo",      AddGoTo             },
            { ":/PhaseRemove",  RemovePhase         }
        };

        editAcGroup_ = new QButtonGroup(this);
        connect(editAcGroup_, SIGNAL(buttonClicked(int)), this, SLOT(emitTableAc(int)));

        for (auto const& ac : tableAcBtns)
        {
            QPushButton* btn = createButton(ac.first);

            if (ac.second == AddRelativeRecord)
            {
                btn->setText("Δ");
                btn->setMaximumWidth(50);
            }

            hL->addWidget(btn);
            editAcGroup_->addButton(btn, ac.second);
        }

        hL->addStretch();


//        btnBaseArcSettings_ = createButton(":/Verify");
//        btnBaseArcSettings_->setText("Верификация");
//        hL->addWidget(btnBaseArcSettings_);
//        connect(btnBaseArcSettings_, SIGNAL(clicked(bool)), this, SLOT(onShowBaseArcSettings()));

        hL->addStretch();

        btnSaveChanges_ = createButton(":/SaveFile");
        hL->addWidget(btnSaveChanges_);
        connect(btnSaveChanges_, SIGNAL(clicked(bool)), this, SLOT(onSaveChanges()));

        btnStopEdit_ = createButton(":/Enter");
        connect(btnStopEdit_, SIGNAL(clicked(bool)), this, SLOT(onStopEdit()));
        hL->addWidget(btnStopEdit_);

        btnCreateNew_ = createButton(":/NewFile");
        connect(btnCreateNew_, SIGNAL(clicked(bool)), this, SLOT(onCreateNew()));
        hL->addWidget(btnCreateNew_);

        btnLoadFile_ = createButton(":/LoadFile");
        connect(btnLoadFile_, SIGNAL(clicked(bool)), this, SLOT(onFileLoad()));
        hL->addWidget(btnLoadFile_);

        layout->addLayout(hL);
    }

    toViewState();
}

void ProgramEditorCtrlWidget::addFcPreSetName(char const*) noexcept
{

}

QString ProgramEditorCtrlWidget::progName() const
{
    return kbBtnName_->text();
}

QString ProgramEditorCtrlWidget::userInfo() const
{
    return kbBtnComments_->text();
}

QPushButton* ProgramEditorCtrlWidget::createButton(QString const& imgName)
{       
    QPushButton* btn = new QPushButton(this);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setIconSize(QSize(25, 25));
    if (!imgName.isEmpty())
        btn->setIcon(QIcon(imgName));
    return btn;
}

void ProgramEditorCtrlWidget::toViewState()
{
    mode_ = &ProgramEditorCtrlWidget::toViewState;
    allowEdit_ = false;

    kbBtnName_->setEnabled(false);
    kbBtnComments_->setEnabled(false);

    btnMakeEdit_->setVisible(progId_ != 0);
//    btnBaseArcSettings_->setVisible(progId_ != 0);

    if (progId_ == 0)
    {
        kbBtnName_->setText("");
        kbBtnComments_->setText("");
        loadProgName_ = "";
    }

    btnSaveChanges_->hide();
    btnStopEdit_->hide();

    for (auto& btn : editAcGroup_->buttons())
        btn->hide();

    btnCreateNew_->setVisible(true);
    btnLoadFile_->setVisible(true);
}

void ProgramEditorCtrlWidget::toViewState(QString const& name, QString const& userInfo)
{
    toViewState();

    kbBtnName_->setText(name);
    kbBtnComments_->setText(userInfo);

    loadProgName_ = name;
}

void ProgramEditorCtrlWidget::toEditState()
{
    mode_ = &ProgramEditorCtrlWidget::toEditState;
    allowEdit_ = true;

    kbBtnName_->setEnabled(true);
    kbBtnComments_->setEnabled(true);

    btnMakeEdit_->hide();
//    btnBaseArcSettings_->hide();

    btnSaveChanges_->setVisible(modified_);
    btnStopEdit_->setIcon(QIcon(!modified_ ? ":/Enter" : ":/Cancel"));
    btnStopEdit_->show();

    for (auto& btn : editAcGroup_->buttons())
        btn->show();

    btnCreateNew_->setVisible(false);
    btnLoadFile_->setVisible(false);
}

void ProgramEditorCtrlWidget::toEditState(QString const& name, QString const& userInfo)
{
    toEditState();

    kbBtnName_->setText(name);
    kbBtnComments_->setText(userInfo);

    loadProgName_ = name;
}

bool ProgramEditorCtrlWidget::toInProcState()
{
    //! Если программа не сохранена на устройстве либо находится в режиме редактирования
    //! не разрешаем ее запуск
    if (progId_ == 0 || allowEdit())
        return false;

    kbBtnName_->setEnabled(false);
    kbBtnComments_->setEnabled(false);

    btnMakeEdit_->hide();
//    btnBaseArcSettings_->hide();

    btnSaveChanges_->hide();
    btnStopEdit_->hide();

    for (auto& btn : editAcGroup_->buttons())
        btn->hide();

    btnCreateNew_->hide();
    btnLoadFile_->hide();

    return true;
}


void ProgramEditorCtrlWidget::setModifyFlag()
{
    modified_ = true;
    (this->*mode_)();
}

void ProgramEditorCtrlWidget::clearModifyFlag()
{
    modified_ = false;
    (this->*mode_)();
}

void ProgramEditorCtrlWidget::setProgId(std::size_t progId)
{
    progId_ = progId;
    clearModifyFlag();
}

void ProgramEditorCtrlWidget::setSavedProgId(std::size_t progId)
{
    clearModifyFlag();

    if (progId == progId_)
        return;
    progId_ = progId;

    loadProgName_ = kbBtnName_->text();
}

void ProgramEditorCtrlWidget::onCreateNew()
{
    toEditState();

    setProgId(0);

    kbBtnName_->setText("");
    kbBtnComments_->setText("");

    loadProgName_ = "";

    emit createNew();
}

void ProgramEditorCtrlWidget::onSaveChanges()
{
    if (kbBtnName_->text().isEmpty())
    {
        Interact::message(Interact::MsgError, "Укажите название программы!");
        return;
    }

    bool const newName = (loadProgName_ != kbBtnName_->text());
    emit makeSaveProg(newName ? 0 : progId_);
}

void ProgramEditorCtrlWidget::onStopEdit()
{
    if (!modified_ && (progId_ != 0))
    {
        toViewState();
    }
    else if (modified_)
    {
        Interact::question("Изменения будут потеряны!\nПродолжить?", [this](bool yes)
                {
                    if (yes)
                         emit makeLoadById(progId_);
                });
    }
    else
    {
        emit makeLoadById(progId_);
    }
}

void ProgramEditorCtrlWidget::onMakeEdit()
{
    toEditState();
}

void ProgramEditorCtrlWidget::emitTableAc(int ac)
{
    setModifyFlag();
    emit makeTableAc(ac);
}


void ProgramEditorCtrlWidget::onProgramNameSet()
{
    setModifyFlag();
}

void ProgramEditorCtrlWidget::onFileLoad()
{
    emit makeLoadFile(false);
}

void ProgramEditorCtrlWidget::onQuestionAccepted(int)
{
    emit makeLoadById(progId_);
}


void ProgramEditorCtrlWidget::onCommentsSet()
{
    setModifyFlag();
}

//void ProgramEditorCtrlWidget::onShowBaseArcSettings() noexcept
//{
//    emit makeShowBaseArcSettings();
//}

//void ProgramEditorCtrlWidget::updateVerifyButtonIcon(bool const f1, bool const f2) noexcept
//{
//    QString iconName(":/VerifyOffOff");
//    if (f1 && f2)
//        iconName = ":/VerifyOnOn";
//   else if (f1 && !f2)
//        iconName = ":/VerifyOnOff";
//    else if (!f1 && f2)
//        iconName = ":/VerifyOffOn";
//
//    btnBaseArcSettings_->setIcon(QIcon(iconName));
//}




