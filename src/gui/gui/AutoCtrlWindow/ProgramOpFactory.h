#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>

#include <rapidxml/rapidxml.hpp>

class Program;
class OpItem;
class Phase;

// namespace Ex
// {
//     class Buffer;
// }

class ProgramOpFactory
{
    typedef std::function<void(std::string const &)> EnumTitleCallback;

    struct ItemRecord
    {
        OpItem* op;
        std::string title;
        std::string name;
    };

    typedef std::vector<ItemRecord> ItemsList;

public:

    ProgramOpFactory();

public:

    void addAxis(char, std::string const&, bool);

public:

    bool check_for_accordance(Program const&) const noexcept;

    void initOpState(Program&, std::size_t) const;

    void makeCode(Program const&, std::string&) const;

    Program* create() const;

    Program* loadFromFile(char const*, std::string&) const;

    Program* loadFromCode(char const*) const;

    // void makeByteCode(Program const&, Ex::Buffer&) const;

    void enumTitle(EnumTitleCallback const&) const;

    ItemsList::const_iterator begin() const { return items_.begin(); }
    ItemsList::const_iterator end() const { return items_.end(); }

    std::string const& itemName(std::size_t cId) const { return items_[cId].title; }

private:

    bool loadProgram(Program*, rapidxml::xml_node<>*) const;

private:

    OpItem* pos(rapidxml::xml_node<> const&) const;
    OpItem* spin(rapidxml::xml_node<> const&) const;
    OpItem* speed(rapidxml::xml_node<> const&) const;
    OpItem* power(rapidxml::xml_node<> const&) const;
    OpItem* current(rapidxml::xml_node<> const&) const;
    OpItem* sprayer(rapidxml::xml_node<> const&) const;

private:

    Phase* pOp(rapidxml::xml_node<> const&) const;
    Phase* pPause(rapidxml::xml_node<> const&) const;
    Phase* pGoTo(rapidxml::xml_node<> const&) const;
    Phase* pTimedFc(rapidxml::xml_node<> const&) const;

private:

    typedef OpItem* (ProgramOpFactory::*OpGetCall)(rapidxml::xml_node<> const&) const;
    typedef Phase* (ProgramOpFactory::*PhaseGetCall)(rapidxml::xml_node<> const&) const;

    ItemsList items_;
    std::vector<std::size_t> priority_;

    std::map<std::string, OpGetCall> ops_;
    std::map<std::string, PhaseGetCall> phases_;
};

