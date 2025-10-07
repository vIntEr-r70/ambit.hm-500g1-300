#include "ProgramOpFactory.h"

#include <map>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cstring>


#include <QString>

#include <rapidxml/rapidxml_utils.hpp>

#include "common/ProgramOpItems.h"

ProgramOpFactory::ProgramOpFactory()
{
    ops_ = {
        { "pos",        &ProgramOpFactory::pos      },
        { "spin",       &ProgramOpFactory::spin     },
        { "speed",      &ProgramOpFactory::speed    },
        { "power",      &ProgramOpFactory::power    },
        { "current",    &ProgramOpFactory::current  },
        { "sprayer",    &ProgramOpFactory::sprayer  },
    };

    phases_ = {
        { "Op",         &ProgramOpFactory::pOp      },
        { "Pause",      &ProgramOpFactory::pPause   },
        { "GoTo",       &ProgramOpFactory::pGoTo    },
        { "TimedFc",    &ProgramOpFactory::pTimedFc },
    };


    items_.push_back({ new OpSpeed(false, 0.0f),    "Скорость\nмм/с (об/мин)",  "speed" });
    items_.push_back({ new OpPower(0.0f),           "Мощность\nкВт",            "power" });
    items_.push_back({ new OpCurrent(0.0f),         "Ток\nА",                   "current" });
    items_.push_back({ new OpSprayer(false),        "Спрейер",                  "sprayer" });
}

// Проверяем программу на соответствие текущим настройкам
bool ProgramOpFactory::check_for_accordance(Program const&) const noexcept
{
    return true;
}

void ProgramOpFactory::addAxis(char axisId, std::string const& name, bool muGrads)
{
    std::stringstream aname;
    aname << name << "\n" << (muGrads ? "град" : "мм");
    items_.push_back({ new OpPos(axisId, muGrads, 0.0f), aname.str(), "pos" });

    if (muGrads)
    {
        aname = std::stringstream();
        aname << "Вращение" << "\n" << ("об/мин");
        items_.push_back({ new OpSpin(axisId, 0.0f), aname.str(), "spin" });
    }

    priority_.resize(items_.size());
    for (std::size_t i = 0; i < priority_.size(); ++i)
        priority_[i] = i;

    std::sort(priority_.begin(), priority_.end(), [&](std::size_t a, std::size_t b)
    {
        return items_[a].op->priority < items_[b].op->priority;
    });
}

void ProgramOpFactory::initOpState(Program& program, std::size_t phaseId) const
{
    std::size_t opId = 0;
    for (auto const& item : items_)
    {
        program.items_.insert(program.items_.begin() + opId, item.op->copy());
        ++opId;
    }
    program.applyPosRelations(phaseId, static_cast<std::size_t>(0));
}

void ProgramOpFactory::makeCode(Program const& program, std::string& code) const
{
    std::stringstream ss;

    std::size_t shift = 0;
    for (auto const& phase : program.phases_)
    {
        ss << "<" << phase->name();
        phase->writeArgs(ss);
        ss << ">\n";

        if (dynamic_cast<Op*>(phase))
        {
            for (std::size_t i = 0; i < items_.size(); ++i)
            {
                ss << "<" << items_[i].name << " ";
                program.items_[shift + i]->writeArgs(ss);
                ss << " />\n";
            }

            shift += items_.size();
        }

        ss << "</" << phase->name() << ">\n";
    }

    code = ss.str();
}

// void ProgramOpFactory::makeByteCode(Program const& program, Ex::Buffer& buf) const
// {
//     std::size_t itemInOpCount = program.itemInOpCount_;
//
//     //! Формируем последовательность вызовов для виртуальной машины
//     //! учитывая приоритет каждой операции
//     std::size_t opId = 0;
//     for (std::size_t phaseId = 0; phaseId < program.phases_.size(); ++phaseId)
//     {
//         auto const& phase = program.phases_[phaseId];
//
//         phase->makeByteCode(buf);
//
//         if (dynamic_cast<Op*>(phase))
//         {
//             for (std::size_t i = 0; i < itemInOpCount; ++i)
//             {
//                 std::size_t itemId = priority_[i];
//                 OpItem* item = program.items_[opId * itemInOpCount + itemId];
//
//                 if (program.isDiffer(phaseId, itemId))
//                     item->makeByteCode(buf);
//             }
//
//             ++opId;
//         }
//     }
// }

Program* ProgramOpFactory::create() const
{
    return new Program(items_.size());
}


bool ProgramOpFactory::loadProgram(Program* program, rapidxml::xml_node<>* node) const
{
    while (node)
    {
        auto it = phases_.find(node->name());
        if (it != phases_.end())
        {
            Phase* phase = (this->*(it->second))(*node);
            if (phase != nullptr)
            {
                program->phases_.push_back(phase);

                Op* op = dynamic_cast<Op*>(phase);
                if (op != nullptr)
                {
                    std::size_t opn = 0;

                    rapidxml::xml_node<>* snode = node->first_node();
                    while (snode)
                    {
                        auto it = ops_.find(snode->name());
                        if (it != ops_.end())
                        {
                            OpItem* item = (this->*(it->second))(*snode);
                            if (item != nullptr)
                            {
                                uint8_t const plan = items_[opn++].op->cmd();
                                uint8_t const fact = item->cmd();

                                // Неожиданная операция
                                if (plan != fact) return false;

                                program->items_.push_back(item);
                                program->applyPosRelations(program->items_.back(), op->absolute);
                            }
                            else
                            {
                                throw 1;
                                // TODO: Сообщение в лог о том, что не удалось создать операцию
                            }
                        }
                        else
                        {
                            throw 2;
                            // TODO: Сообщение в лог о том, что не нашли соответствующей операции
                        }

                        snode = snode->next_sibling();
                    }

                    // Количество элементов в этапе должно
                    // соответствовать текущим настройкам
                    if (opn != items_.size())
                        return false;
                }
            }
            else
            {
                throw 3;
                // TODO: Сообщение в лог о том, что не удалось создать этап
            }
        }
        else
        {
            throw 4;
            // TODO: Сообщение в лог о том, что не нашли соответствующего этапа
        }

        node = node->next_sibling();
    }

    return true;
}



Program* ProgramOpFactory::loadFromFile(char const* fileName, std::string& userInfo) const
{
    Program* program = new Program(items_.size());

    try
    {
        rapidxml::file<> file(fileName);

        rapidxml::xml_document<> doc;
        doc.parse<rapidxml::parse_default>(file.data());

        rapidxml::xml_node<>* node = doc.first_node();

        userInfo = node->value();
        node = node->next_sibling();

        if (!loadProgram(program, node))
        {
            delete program;
            return nullptr;
        }
    }
    catch(...)
    {
        delete program;
        return nullptr;
    }

    return program;
}

Program* ProgramOpFactory::loadFromCode(char const* code) const
{
    Program* program = new Program(items_.size());

    try
    {
        std::vector<char> buf(code, code + std::strlen(code) + 1);

        rapidxml::xml_document<> doc;
        doc.parse<rapidxml::parse_default>(buf.data());
        rapidxml::xml_node<>* node = doc.first_node();

        if (!loadProgram(program, node))
        {
            delete program;
            return nullptr;
        }
    }
    catch(...)
    {
        delete program;
        return nullptr;
    }

    return program;
}


void ProgramOpFactory::enumTitle(EnumTitleCallback const& cb) const
{
    for (auto const& item : items_)
        cb(item.title);
}

OpItem* ProgramOpFactory::pos(rapidxml::xml_node<> const& node) const
{
    rapidxml::xml_attribute<>* id = node.first_attribute("id");
    rapidxml::xml_attribute<>* muGrads = node.first_attribute("muGrads");
    rapidxml::xml_attribute<>* value = node.first_attribute("value");

    bool inGrad = muGrads ? (std::strcmp(muGrads->value(), "true") == 0) : false;
    float v = value ? QString(value->value()).toFloat() : 0.f;
    return (id == nullptr) ? nullptr : new OpPos(id->value()[0], inGrad, v);
}
OpItem* ProgramOpFactory::spin(rapidxml::xml_node<> const& node) const
{
    rapidxml::xml_attribute<>* id = node.first_attribute("id");
    rapidxml::xml_attribute<>* value = node.first_attribute("value");
    float v = value ? QString(value->value()).toFloat() : 0.f;
    return (id == nullptr) ? nullptr : new OpSpin(id->value()[0], v);
}
OpItem* ProgramOpFactory::speed(rapidxml::xml_node<> const& node) const
{
    rapidxml::xml_attribute<>* muGrads = node.first_attribute("muGrads");
    rapidxml::xml_attribute<>* value = node.first_attribute("value");
    bool inGrad = muGrads ? (std::strcmp(muGrads->value(), "true") == 0) : false;
    float v = value ? QString(value->value()).toFloat() : 0.f;
    return new OpSpeed(inGrad, v);
}
OpItem* ProgramOpFactory::power(rapidxml::xml_node<> const& node) const
{
    rapidxml::xml_attribute<>* value = node.first_attribute("value");
    float v = value ? QString(value->value()).toFloat() : 0.f;
    return new OpPower(v);
}
OpItem* ProgramOpFactory::current(rapidxml::xml_node<> const& node) const
{
    rapidxml::xml_attribute<>* value = node.first_attribute("value");
    float v = value ? QString(value->value()).toFloat() : 0.f;
    return new OpCurrent(v);
}
OpItem* ProgramOpFactory::sprayer(rapidxml::xml_node<> const& node) const
{
    rapidxml::xml_attribute<>* state = node.first_attribute("state");
    return new OpSprayer(state ? std::stoi(state->value()) : false);
}


Phase* ProgramOpFactory::pOp(rapidxml::xml_node<> const& node) const
{
    rapidxml::xml_attribute<>* state = node.first_attribute("absolute");
    return new Op(std::strcmp(state->value(), "true") == 0);
}
Phase* ProgramOpFactory::pPause(rapidxml::xml_node<> const& node) const
{
    rapidxml::xml_attribute<>* seconds = node.first_attribute("seconds");
    rapidxml::xml_attribute<>* ds = node.first_attribute("ds");
    uint32_t vsec = seconds ? std::stoul(seconds->value()) : 0;
    uint32_t vds = ds ? std::stoul(ds->value()) : 0;
    return seconds ? new Pause(vsec, vds) : nullptr;
}
Phase* ProgramOpFactory::pGoTo(rapidxml::xml_node<> const& node) const
{
    rapidxml::xml_attribute<>* opid = node.first_attribute("opid");
    rapidxml::xml_attribute<>* N = node.first_attribute("N");
    return (opid && N) ? new GoTo(std::stoul(opid->value()), std::stoul(N->value())) : nullptr;
}
Phase* ProgramOpFactory::pTimedFc(rapidxml::xml_node<> const& node) const
{
    rapidxml::xml_attribute<>* p  = node.first_attribute("p");
    rapidxml::xml_attribute<>* i  = node.first_attribute("i");
    rapidxml::xml_attribute<>* tv = node.first_attribute("tv");
    return (p && i && tv) ? new TimedFC(std::stof(p->value()), std::stof(i->value()), std::stof(tv->value())) : nullptr;
}
