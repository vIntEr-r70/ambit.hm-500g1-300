#pragma once

#include <QFrame>

class KbPushButton;
class QPushButton;
class QButtonGroup;

class ProgramEditorCtrlWidget
    : public QFrame
{
    Q_OBJECT

public:

    enum TableAc
    {
        AddAbsoluteRecord = 0,
        AddRelativeRecord,
        AddPause,
        AddGoTo,
        AddTimedFC,
        RemovePhase
    };

public:

    ProgramEditorCtrlWidget(QWidget*);

public:

    void addFcPreSetName(char const*) noexcept;

    inline bool allowEdit() const { return allowEdit_; }

    void toViewState();

    void toViewState(QString const&, QString const&);

    bool toInProcState();

    void toEditState();

    void toEditState(QString const&, QString const&);

    void setProgId(std::size_t);

    void setSavedProgId(std::size_t);

    inline std::size_t progId() const noexcept { return progId_; }

    void setModifyFlag();

    QString progName() const;

    QString userInfo() const;

//    void updateVerifyButtonIcon(bool, bool) noexcept;

signals:

    void makeSaveProg(std::size_t);

    void makeLoadFile(bool);

    void makeLoadById(std::size_t);

    void createNew();

    void makeTableAc(int);

//    void makeShowBaseArcSettings();

private slots:

    void onCreateNew();

    void onSaveChanges();

    void onStopEdit();

    void onFileLoad();

    void onQuestionAccepted(int);

    void onProgramNameSet();

    void onCommentsSet();

    void onMakeEdit();

//    void onShowBaseArcSettings() noexcept;

    void emitTableAc(int);

private:

    void clearModifyFlag();

    QPushButton* createButton(QString const&);

private:

    //! В процессе редактирования были внесены изменения
    bool modified_ = false;
    QString loadProgName_;

    //! Циклограмма сохранена в базе
    std::size_t progId_ = 0;

    KbPushButton* kbBtnName_;
    KbPushButton* kbBtnComments_;

    QPushButton* btnMakeEdit_;
    QButtonGroup* editAcGroup_;

    QPushButton* btnCreateNew_;
    QPushButton* btnLoadFile_;
    QPushButton* btnSaveChanges_;
    QPushButton* btnStopEdit_;
    QPushButton* btnBaseArcSettings_;

    void (ProgramEditorCtrlWidget::*mode_)() = {nullptr};
    bool allowEdit_{ false };
};

