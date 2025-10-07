#include "Program.h"

#include "ProgramOpItems.h"

#include <cassert>

Program::Program(std::size_t itemInOpCount)
    : itemInOpCount_(itemInOpCount)
{ }

//! Функция вставки нового этапа в позицию phaseId
std::size_t Program::addPhase(std::size_t phaseId, Phase* phase)
{
    phaseId = phaseId ? (phaseId - 1) : phases_.size();
    phases_.insert(phases_.begin() + phaseId, phase);
    return phaseId;
}

bool Program::isFirstOp(std::size_t phaseId) const
{
    return opId(phaseId) == 0;
}

void Program::initOpState(std::size_t phaseId)
{
    std::size_t fId = opId(phaseId) * itemInOpCount_;
    for (std::size_t i = 0; i < itemInOpCount_; ++i)
        items_.insert(items_.begin() + fId + i, items_[fId - itemInOpCount_ + i]->copy());
    applyPosRelations(phaseId, fId);
}

void Program::applyPosRelations(std::size_t phaseId, std::size_t fId)
{
    Op const* op = dynamic_cast<Op const*>(phases_[phaseId]);
    assert (op != nullptr);

    for (std::size_t i = 0; i < itemInOpCount_; ++i)
        applyPosRelations(items_[fId + i], op->absolute);
}

void Program::applyPosRelations(OpItem* item, bool absolute)
{
    OpPos* opPos = dynamic_cast<OpPos*>(item);
    if (opPos != nullptr)
        opPos->absolute = absolute;
}

void Program::removePhase(std::size_t phaseId)
{
    if (dynamic_cast<Op*>(phases_[phaseId]))
    {
        std::size_t id = opId(phaseId) * itemInOpCount_;

        auto first = items_.begin() + id;
        auto last = first + itemInOpCount_;

        for (auto it = first; it != last; ++it)
            delete *it;

        items_.erase(first, last);
    }

    auto it = phases_.begin() + phaseId;
    delete *it;
    phases_.erase(it);
}

OpItem const* Program::getOpItem(std::size_t phaseId, std::size_t itemId) const noexcept
{
    std::size_t id = opId(phaseId) * itemInOpCount_ + itemId;
    return items_[id];
}

OpItem* Program::getOpItem(std::size_t phaseId, std::size_t itemId) noexcept
{
    std::size_t id = opId(phaseId) * itemInOpCount_ + itemId;
    return items_[id];
}

bool Program::isDiffer(std::size_t phaseId, std::size_t itemId) const noexcept
{
    std::size_t shift = opId(phaseId) * itemInOpCount_;

    OpSpeed* speed = dynamic_cast<OpSpeed*>(items_[shift + itemId]);
    if (speed == nullptr)
        return _isDiffer(phaseId, itemId);

    //! Для скорости данная проверка имеет другой смысл
    //! Скорость для нас актуальна только в том случае,
    //! если есть изменение в координатах
    bool differ = false;
    for (std::size_t i = 0; i < itemInOpCount_; ++i)
    {
        if (dynamic_cast<OpPos*>(items_[shift + i]) == nullptr)
            continue;
        differ |= _isDiffer(phaseId, i);
    }
    return differ;
}

bool Program::_isDiffer(std::size_t phaseId, std::size_t itemId) const noexcept
{
    std::size_t id = opId(phaseId) * itemInOpCount_ + itemId;

    OpItem* actual = items_[id];
    OpItem* prev = (id < itemInOpCount_) ?
            nullptr : items_[id - itemInOpCount_];

    return (prev == nullptr) ?
            !actual->isDefault() : actual->isDiffer(prev);
}


std::size_t Program::opId(std::size_t phaseId) const
{
    std::size_t res = 0;
    for (std::size_t i = 0; i < phaseId; ++i)
    {
        if (dynamic_cast<Op*>(phases_[i]))
            ++res;
    }
    return res;
}
