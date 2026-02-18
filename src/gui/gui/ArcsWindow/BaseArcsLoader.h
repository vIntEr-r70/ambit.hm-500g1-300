#pragma once

#include <deque>
#include <string>

class BaseArcsLoader
{
public:

    BaseArcsLoader();

public:

    void updateFilter();

    void updateLastArcId();

    void loadArcData(std::size_t, std::size_t);

    void removeArc(std::size_t);

public:

    inline std::size_t lastArcId() const { return lastArcId_; }

protected:

    void updatePages();

    void makeArcAsBase(std::size_t, bool);

    void saveProgramCtrl(std::size_t, std::string const&);

   // void saveProgramVerifyFlag(std::size_t, bool);

private:

    virtual void nfConnected() = 0;
    
    virtual void nfArcsCount(std::size_t) = 0;

    virtual void nfLastArcId(std::size_t) = 0;

    virtual std::size_t nfGetFirstRecordId() = 0;

    virtual std::size_t nfGetLastRecordId() = 0;

    virtual std::size_t nfGetRowsInPage() = 0;

    virtual bool nfIsPageLoaded(std::size_t) = 0;

    // virtual void nfAppendNewArcData(std::size_t, nlohmann::json const &) = 0;
    //
    // virtual void nfAppendNewListData(std::size_t, nlohmann::json const &) = 0;

    virtual void nfArcWasRemoved(std::size_t) = 0;

private:

    void initLoadPage(std::size_t);

private:

    std::size_t arcsCount_{ 0 };
    std::size_t lastArcId_{ 0 };

    // aem::net::rpc_tcp_client arcsSrc_;

    std::size_t stmtId_{ 0 };
    std::deque<std::size_t> pagesQueue_;
};


