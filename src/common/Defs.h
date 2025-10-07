#pragma once

#include <aem/types.h>

enum fc_set_param : std::size_t
{
    I = 0,
    U,
    P
};

enum class cnc_status : std::size_t
{
    unknown,
    idle,
    run,
    hold,
    queue
};

enum class centering_type : aem::uint8
{
    not_active,
    tooth_in,
    tooth_out,
    shaft
};

namespace on_off_unit
{
    enum : std::size_t 
    {
        pump = 0,
        sprayer1,
        sprayer2,
        sprayer3,

        count
    };
}

struct uArcs
{
    static constexpr char const* arc_createNew              = "arc-create-new";
    static constexpr char const* arc_remove                 = "arc-remove";
    static constexpr char const* arc_close                  = "arc-close";
    static constexpr char const* arc_writeTo                = "arc-write-to";
    static constexpr char const* arc_init                   = "arc-init";
    static constexpr char const* arc_initList               = "arc-init-list";
    static constexpr char const* arc_loadList               = "arc-load-list";
    static constexpr char const* arc_loadData               = "arc-load-data";
    static constexpr char const* arc_getLastId              = "arc-get-last-id";
    static constexpr char const* arc_loadInfo               = "arc-load-info";

    static constexpr char const* markArcAsBase              = "mark-arc-as-base";

    static constexpr char const* prog_create               = "prog-create";
    static constexpr char const* prog_update               = "prog-update";
    static constexpr char const* prog_getIdByName          = "prog-get-id-by-name";
    static constexpr char const* prog_loadList             = "prog-load-list";
    static constexpr char const* prog_load                 = "prog-load";
    static constexpr char const* prog_remove               = "prog-remove";
    static constexpr char const* prog_updateCtrl           = "prog-update-ctrl";
    static constexpr char const* prog_updateVerify         = "prog_update-verify";
};

struct Arcs
{
    enum Type : aem::uint8
    {
        SysMessagesType = 0,
        HeatingType     = 1,
        AutoType        = 2,
        NoType          = 0xFF
    };

#pragma pack(push, 1)
    struct HeatingRecord
    {
        aem::uint32 proc_ms;
        aem::uint32 sys_s;

        float i_A;
        float u_V;
        float p_Wt;

        float dtf_grad;
        float dtp_grad;
    };
#pragma pack(pop)

};


struct Core
{
#pragma pack(push, 1)

    struct AxisDesc
    {
        bool allowInAuto = false;
        bool muGrads = false;

        char name[64] = {0};

        bool use() const { return name[0] != '\0'; }
        void do_not_use() { name[0] = '\0'; }

        float length = 0.0f;

        // Определяет какой из концевиков считается домом
        // false = MIN, true = MAX 
        bool home{ false };

        aem::uint8 limitSI = 0;
        float ratio = 1.0f;

        //! Скорость задается в мм/мин или рад/мин
        //! Ускорение в мм/мин2 либо рад/мин2
        //! Коэфициент в шаг/мм либо шаг/рад
        //! Именно с этими единицами работает плата Курина
        float acc = 1.0f;

        //! [мм/мин] [рад/мин]
        float speed[3] = {1.0f, 10.0f, 100.0f};

        // Ось, работающая в паре в противоположном направлении
        char pair_axis{ 0 };

        struct
        {
            float ratio = 1.0f;

            //! [мм/мин] [рад/мин]
            float speed[3] = {1.0f, 10.0f, 100.0f};
            char link = 0;

        } rcu;

        bool get_home_limit(bool min, bool max) const noexcept { return home ? max : min; }
    };

#pragma pack(pop)

    struct SysMode
    {
        enum : aem::uint8
        {
            No       = 0,
            Manual,
            Auto,
            Error,
        };
    };

    struct CtlMode
    {
        enum : aem::uint8
        {
            No      = 0,
            Panel,
            Rcu
        };
    };

    struct Arcs
    {
        enum : aem::uint8
        {
            SysMsgs = 1
        };
    };

    //<!-- 0: стоп, 1: запущен, 2: пауза, 3: авария  -->
    struct ModeStatus
    {
        enum : aem::uint8
        {
            NotReady    = 0,
            Error       = 1,
            Ready       = 2,
            InProc      = 3,
            Wait        = 4
        };
    };

    struct AutoModeNotify
    {
        enum : aem::uint8
        {
            EmptyProgramCode = 1,
            BadProgramCode,
            AxisMustBeInZerro,
            ZeroSpeedNotAllow,
            UnexpTermination,
            BadGoToLabelValue,

            NotAllowTimedFcMode,
            BadFcTimeValue,
            UnknownNotify,

            NeedStartCG,
            CGWasStoped,

            CGWasSuccessDone
        };
    };

    struct ErrorBits
    {
        enum
        {
            rs485Port,
            rs422Port,
            fcNoConnect,
            mkNoConnect,
            trmNoConnect,
            cncNoConnect,
            plcNoConnect,
            tx01NoConnect,
            fcError,
            EmergencyStop,
            plcWaterFlow,
            plcTSUOverhating,
            plcFcPowerLock,
            cncPosShift,
            cncCmdError,

            btnFc,
            btnSprayer,
            btnPump,

            Count
        };

        static bool check(aem::uint32 mask, std::size_t bitId)
        {
            return (mask & (1 << bitId)) != 0;
        }
    };
};

