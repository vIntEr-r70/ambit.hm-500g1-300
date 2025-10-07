#include "ArcInfo.h"

#include <iostream>
#include <fstream>
#include <map>

void ArcInfo::reset() noexcept
{
    headerSize_ = 0;
//    data_.clear();
}


bool ArcInfo::append(uint8_t const* data, std::size_t size, std::size_t* leftSize) noexcept
{
    // std::size_t incomingDataSize = data_.size() + size;
    //
    // //! Данных пока не достаточно
    // if (incomingDataSize < sizeof(HeadSizeType))
    // {
    //     //! Сохраняем имеющееся
    //     data_.append(data, size);
    //     return false;
    // }
    // //! Не учитываем размер поля размера заголовка
    // incomingDataSize -= sizeof(HeadSizeType);
    //
    // //! Убеждаемся что в буффере есть нужное количество байт
    // if (data_.size() < sizeof(HeadSizeType))
    // {
    //     std::size_t extSize = sizeof(HeadSizeType) - data_.size();
    //     data_.append(data, extSize);
    //     data += extSize;
    //     size -= extSize;
    // }
    //
    // HeadSizeType const* headSize = reinterpret_cast<HeadSizeType const*>(data_.data());
    // //! Данных все еще недостаточно
    // if (incomingDataSize < *headSize)
    // {
    //     //! Сохраняем имеющееся
    //     data_.append(data, size);
    //     return false;
    // }
    //
    // headerSize_ = *headSize + sizeof(HeadSizeType);
    // if (leftSize != nullptr)
    //     *leftSize = size - (headerSize_ - data_.size());
    //
    // //! Сохраняем в буффер недостающие данные
    // data_.append(data, headerSize_ - data_.size());
    // data_.push_back('\0');
    //
    // //! Разбираем заголовок
    // doc_.Parse(reinterpret_cast<char const*>(data_.data() + sizeof(HeadSizeType)));
    // if (doc_.HasParseError())
    // {
    //     reset();
    //     return true;
    // }
    //
    // auto iter = doc_.MemberBegin();
    // for (; iter != doc_.MemberEnd(); ++iter)
    //     append(iter->name, iter->value);
    //
    // doc_.RemoveAllMembers();

    return true;
}


// ArcInfo::Handler ArcInfo::handler(char const* tagName) const
// {
//     static std::map<std::string, Handler> const handlers = {
//         // { "info", &ArcInfo::asInfo },
//         // { "header", &ArcInfo::asHeader }
//     };
//     auto it = handlers.find(tagName);
//     return (it == handlers.end()) ? nullptr : it->second;
// }

// void ArcInfo::asInfo(rapidjson::Value const& header)
// {
//     auto iter = header.MemberBegin();
//     for (; iter != header.MemberEnd(); ++iter)
//     {
//         auto& member = info[iter->name.GetString()];
//         auto const& v = iter->value;
//
//         if (v.IsString())
//             member = v.GetString();
//         else if (v.IsUint())
//             member = std::to_string(v.GetUint());
//         else if (v.IsInt())
//             member = std::to_string(v.GetInt());
//         else if (v.IsUint64())
//             member = std::to_string(v.GetUint64());
//         else if (v.IsInt64())
//             member = std::to_string(v.GetInt64());
//         else if (v.IsDouble())
//             member = std::to_string(v.GetDouble());
//     }
// }

// void ArcInfo::asHeader(rapidjson::Value const& header)
// {
//     cols.clear();
//     dgms.clear();
//
//     auto iter = header.Begin();
//     for (; iter != header.End(); ++iter)
//     {        
//         std::size_t cId = cols.size();
//
//         char const* const type = (*iter)["type"].GetString();
//         bool asFlags = (std::strcmp("bool", type) == 0);
//         cols.push_back(type);
//
//         auto id = iter->FindMember("id");
//         if (id != iter->MemberEnd())
//         {
//             DgmDesc dgm;
//             dgm.title = (*iter)["title"].GetString();
//             dgm.id = (*iter)["id"].GetString();
//             dgm.cId = cId;
//             dgm.asFlags = asFlags;
//             dgms.push_back(std::move(dgm));
//         }
//     }
// }

// void ArcInfo::append(rapidjson::Value const& name, rapidjson::Value const& value)
// {
//     Handler h = handler(name.GetString());
//     if (h == nullptr)
//         return;
//     (this->*h)(value);
// }

