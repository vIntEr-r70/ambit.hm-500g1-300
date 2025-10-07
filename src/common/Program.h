#pragma once

#include <vector>
#include <string>

struct OpItem;
struct Phase;

class Program
{
    friend class ProgramOpFactory;

private:

    Program(std::size_t);

public:

    std::size_t addPhase(std::size_t, Phase*);

    bool isFirstOp(std::size_t) const;

    void initOpState(std::size_t);

    void removePhase(std::size_t);

    Phase* phase(std::size_t idx) { return phases_[idx]; }

    Phase const* phase(std::size_t idx) const { return phases_[idx]; }

    inline std::size_t phaseCount() const noexcept { return phases_.size(); }

    inline std::size_t opsCount() const noexcept { return items_.size() / itemInOpCount_; }

    inline std::size_t itemInOpCount() const noexcept { return itemInOpCount_; }

    OpItem const* getOpItem(std::size_t, std::size_t) const noexcept;

    OpItem* getOpItem(std::size_t, std::size_t) noexcept;

    bool isDiffer(std::size_t, std::size_t) const noexcept;

    std::size_t opId(std::size_t) const;

    void applyPosRelations(std::size_t, std::size_t);

    void applyPosRelations(OpItem*, bool);

private:

    bool _isDiffer(std::size_t, std::size_t) const noexcept;

private:

    std::size_t itemInOpCount_ = 0;

    std::vector<Phase*> phases_;
    std::vector<OpItem*> items_;
};


