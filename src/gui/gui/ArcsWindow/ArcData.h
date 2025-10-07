#pragma once

#include "ArcInfo.h"

#include <aem/buffer.h>

class ArcData
{
    typedef double (ArcData::*Handler)(std::size_t) const;

    enum class TypeId : uint8_t
    {
        t_bool,
        t_int64,
        t_int32,
        t_int16,
        t_int8,
        t_uint64,
        t_uint32,
        t_uint16,
        t_uint8,
        t_float,
        t_double,
        t_string
    };

    struct Records
    {
        virtual ~Records() = default;

        virtual void updateDataSize(std::size_t) = 0;

        virtual void addColSize(std::size_t) = 0;

        virtual std::size_t shift(std::size_t, std::size_t) const = 0;

        virtual std::size_t rows() const noexcept = 0;

        virtual std::size_t cols() const noexcept = 0;
    };

    struct StaticRecords
        : public Records
    {
    public:

        void addColSize(std::size_t) override;

        void updateDataSize(std::size_t) override;

    private:

        std::size_t rows() const noexcept override { return rows_; }

        std::size_t cols() const noexcept override { return colsSize_.size(); }

        std::size_t shift(std::size_t, std::size_t) const override;

    private:

        std::vector<std::size_t> colsSize_;
        std::size_t recordSize_{ 0 };
        std::size_t rows_{ 0 };
    };

public:

    ~ArcData() noexcept;

public:

    bool append(uint8_t const*, std::size_t) noexcept;

    inline bool shouldContinue() { return shouldContinue_; }

public:

    void reset(std::size_t) noexcept;

    inline bool needLoad() const noexcept { return (arcId_ != 0) && !arcInfo.inited(); }

    inline std::size_t arcId() const noexcept { return arcId_; }

    inline std::size_t loadOffset() const noexcept { return loadOffset_; }

    std::size_t rows() const noexcept;

    double asDouble(std::size_t, std::size_t) const;

    int64_t asInt64(std::size_t, std::size_t) const;

    std::string asString(std::size_t, std::size_t) const;

private:

    double ft_bool(std::size_t) const noexcept;
    double ft_int8(std::size_t) const noexcept;
    double ft_int16(std::size_t) const noexcept;
    double ft_int32(std::size_t) const noexcept;
    double ft_int64(std::size_t) const noexcept;
    double ft_uint8(std::size_t) const noexcept;
    double ft_uint16(std::size_t) const noexcept;
    double ft_uint32(std::size_t) const noexcept;
    double ft_uint64(std::size_t) const noexcept;
    double ft_float(std::size_t) const noexcept;
    double ft_double(std::size_t) const noexcept;
    double ft_string(std::size_t) const noexcept;

    TypeId type(char const*) const noexcept;
    std::size_t typeSize(TypeId) const noexcept;
    Handler typeHandler(TypeId) const noexcept;

public:

    ArcInfo arcInfo;

private:

    std::size_t arcId_{ 0 };
    std::size_t loadOffset_{ 0 };
    bool shouldContinue_{ false };

    aem::buffer data_;
    std::vector<Handler> handlers_;
    Records* records_{ nullptr };

public:

    uint64_t beginDate;
    std::string progName;
};

