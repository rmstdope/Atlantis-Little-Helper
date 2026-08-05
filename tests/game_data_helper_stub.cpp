#include "data.h"

void CGameDataHelper::ReportError(const char *, int, bool)
{
}

long CGameDataHelper::GetStudyCost(const char *)
{
    return 0;
}

long CGameDataHelper::GetStructAttr(const char *, long & MaxLoad, long & MinSailingPower)
{
    MaxLoad = 0;
    MinSailingPower = 0;
    return 0;
}

const char * CGameDataHelper::GetConfString(const char *, const char *)
{
    return "";
}

bool CGameDataHelper::GetOrderId(const char *, long & id)
{
    id = 0;
    return false;
}

bool CGameDataHelper::IsTradeItem(const char *)
{
    return false;
}

bool CGameDataHelper::IsMan(const char *)
{
    return false;
}

const char * CGameDataHelper::GetWeatherLine(bool, bool, int)
{
    return "";
}

const char * CGameDataHelper::ResolveAlias(const char * alias)
{
    return alias ? alias : "";
}

bool CGameDataHelper::GetItemWeights(const char *, int *&, const char **&, int & movecount)
{
    movecount = 0;
    return false;
}

void CGameDataHelper::GetMoveNames(const char **& movenames)
{
    movenames = nullptr;
}

bool CGameDataHelper::GetTropicZone(const char *, long & y_min, long & y_max)
{
    y_min = 0;
    y_max = 0;
    return false;
}

void CGameDataHelper::SetTropicZone(const char *, long, long)
{
}

void CGameDataHelper::GetProdDetails(const char *, TProdDetails & details)
{
    details = TProdDetails();
}

long CGameDataHelper::MaxSkillLevel(const char *, const char *, const char *, bool)
{
    return 0;
}

bool CGameDataHelper::ImmediateProdCheck()
{
    return false;
}

bool CGameDataHelper::CanSeeAdvResources(const char *, const char *, std::vector<long> &, std::vector<std::string> &)
{
    return false;
}

bool CGameDataHelper::ShowMoveWarnings()
{
    return false;
}

bool CGameDataHelper::IsRawMagicSkill(const char *)
{
    return false;
}

int CGameDataHelper::GetAttitudeForFaction(int)
{
    return 0;
}

void CGameDataHelper::SetAttitudeForFaction(int, int)
{
}

void CGameDataHelper::SetPlayingFaction(long)
{
}

bool CGameDataHelper::IsWagon(const char *)
{
    return false;
}

bool CGameDataHelper::IsWagonPuller(const char *)
{
    return false;
}

int CGameDataHelper::WagonCapacity()
{
    return 0;
}
