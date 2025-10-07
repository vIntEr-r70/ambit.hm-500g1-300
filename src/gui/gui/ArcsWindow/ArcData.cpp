#include "ArcData.h"

#include <fstream>
#include <map>

void ArcData::StaticRecords::addColSize(std::size_t size)
{
    colsSize_.push_back(recordSize_);
    recordSize_ += size;
}

void ArcData::StaticRecords::updateDataSize(std::size_t newDataSize)
{
    rows_ = newDataSize / recordSize_;
}

std::size_t ArcData::StaticRecords::shift(std::size_t r, std::size_t c) const
{
    return recordSize_ * r + colsSize_[c];
}

ArcData::~ArcData() noexcept
{
    delete records_;
}


//! Инициируем состояние загрузки нового архива
void ArcData::reset(std::size_t aId) noexcept
{
    if (arcId_ == aId)
        return;
    arcId_ = aId;

    arcInfo.reset();
    loadOffset_ = 0;
    shouldContinue_ = true;

    if (records_ != nullptr)
        delete records_;
    records_ = nullptr;

    handlers_.clear();
}


bool ArcData::append(uint8_t const* data, std::size_t size) noexcept
{
    //! Считаем общее количество полученных данных архива
    loadOffset_ += size;

    //! Если пришло 0 байт для нашего архива, значит его загрузка завершена
    if (size == 0)
    {
        shouldContinue_ = false;
        return false;
    }

    //! Если заголовок еще не инициализирован, пытаемся инициализировать его
    if (!arcInfo.inited())
    {
        std::size_t leftSize;

        //! Данных для анализа заголовка пока не достаточно
        if (!arcInfo.append(data, size, &leftSize))
            return false;

        //! Если все необходимые данные получены но все равно не
        //! удалось инициализировать
        if (!arcInfo.inited())
        {
            shouldContinue_ = false;
            return false;
        }

        std::vector<TypeId> cols;
        bool ss = false;
        for (auto const& col : arcInfo.cols)
        {
            cols.push_back(type(col));
            ss |= (cols.back() == TypeId::t_string);
        }

        records_ = !ss ? new StaticRecords() : nullptr;

        for (auto const& type : cols)
        {
            records_->addColSize(typeSize(type));
            handlers_.push_back(typeHandler(type));
        }

        //! Часть данных может быть заголовком, учитываем это
        data_.clear();
        data_.append(data + (size - leftSize), leftSize);

        records_->updateDataSize(data_.size());

        return true;
    }

    //! Сохраняем данные во внутреннем буфере
    data_.append(data, size);
    records_->updateDataSize(data_.size());

    return false;
}


std::size_t ArcData::rows() const noexcept
{
    return records_ ? records_->rows() : 0;
}

double ArcData::asDouble(std::size_t r, std::size_t c) const
{
    if (r >= records_->rows() || c >= records_->cols()) return 0.0;
    return (this->*(handlers_[c]))(records_->shift(r, c));
}

int64_t ArcData::asInt64(std::size_t r, std::size_t c) const
{
    if (r >= records_->rows() || c >= records_->cols()) return 0;
    return static_cast<int64_t>((this->*(handlers_[c]))(records_->shift(r, c)));
}

std::string ArcData::asString(std::size_t r, std::size_t c) const
{
    if (r >= records_->rows() || c >= records_->cols()) return "";
    return "";
}

template <typename T>
double ft_base(uint8_t const* data) noexcept {
    return static_cast<double>(*reinterpret_cast<T const*>(data));
}

double ArcData::ft_bool(std::size_t shift) const noexcept {
    return ft_base<bool>(data_.cbegin() + shift);
}
double ArcData::ft_int8(std::size_t shift) const noexcept {
    return ft_base<int8_t>(data_.cbegin() + shift);
}
double ArcData::ft_int16(std::size_t shift) const noexcept {
    return ft_base<int16_t>(data_.cbegin() + shift);
}
double ArcData::ft_int32(std::size_t shift) const noexcept {
    return ft_base<int32_t>(data_.cbegin() + shift);
}
double ArcData::ft_int64(std::size_t shift) const noexcept {
    return ft_base<int64_t>(data_.cbegin() + shift);
}
double ArcData::ft_uint8(std::size_t shift) const noexcept {
    return ft_base<uint8_t>(data_.cbegin() + shift);
}
double ArcData::ft_uint16(std::size_t shift) const noexcept {
    return ft_base<uint16_t>(data_.cbegin() + shift);
}
double ArcData::ft_uint32(std::size_t shift) const noexcept {
    return ft_base<uint32_t>(data_.cbegin() + shift);
}
double ArcData::ft_uint64(std::size_t shift) const noexcept {
    return ft_base<uint64_t>(data_.cbegin() + shift);
}
double ArcData::ft_float(std::size_t shift) const noexcept {
    return ft_base<float>(data_.cbegin() + shift);
}
double ArcData::ft_double(std::size_t shift) const noexcept {
    return ft_base<double>(data_.cbegin() + shift);
}
double ArcData::ft_string(std::size_t shift) const noexcept {
    return std::stod(reinterpret_cast<char const*>(data_.cbegin() + shift));
}

ArcData::TypeId ArcData::type(char const* typeName) const noexcept
{
    static std::map<std::string, TypeId> types = {
        { "bool",       TypeId::t_bool    },
        { "int64",      TypeId::t_int64   },
        { "int32",      TypeId::t_int32   },
        { "int16",      TypeId::t_int16   },
        { "int8",       TypeId::t_int8    },
        { "uint64",     TypeId::t_uint64  },
        { "uint32",     TypeId::t_uint32  },
        { "uint16",     TypeId::t_uint16  },
        { "uint8",      TypeId::t_uint8   },
        { "float",      TypeId::t_float   },
        { "double",     TypeId::t_double  },
        { "string",     TypeId::t_int64   },
        { "time",       TypeId::t_int64   }
    };

    auto it = types.find(typeName);
    if (it != types.end())
        return it->second;

    return TypeId::t_string;
}

std::size_t ArcData::typeSize(TypeId ti) const noexcept
{
    switch(ti)
    {
    case TypeId::t_bool:
    case TypeId::t_int8:
    case TypeId::t_uint8:
        return 1;
    case TypeId::t_int16:
    case TypeId::t_uint16:
        return 2;
    case TypeId::t_int32:
    case TypeId::t_uint32:
    case TypeId::t_float:
        return 4;
    case TypeId::t_int64:
    case TypeId::t_uint64:
    case TypeId::t_double:
        return 8;
    default:
        return 0;
    }
}

ArcData::Handler ArcData::typeHandler(TypeId ti) const noexcept
{
    switch(ti)
    {
    case TypeId::t_bool:    return &ArcData::ft_bool;

    case TypeId::t_int8:    return &ArcData::ft_int8;
    case TypeId::t_int16:   return &ArcData::ft_int16;
    case TypeId::t_int32:   return &ArcData::ft_int32;
    case TypeId::t_int64:   return &ArcData::ft_int64;

    case TypeId::t_uint8:   return &ArcData::ft_uint8;
    case TypeId::t_uint16:  return &ArcData::ft_uint16;
    case TypeId::t_uint32:  return &ArcData::ft_uint32;
    case TypeId::t_uint64:  return &ArcData::ft_uint64;

    case TypeId::t_float:   return &ArcData::ft_float;
    case TypeId::t_double:  return &ArcData::ft_double;

    default:                return &ArcData::ft_string;
    }
}





