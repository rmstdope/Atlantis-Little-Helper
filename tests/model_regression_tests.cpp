#include "catch.hpp"

#include "data.h"

#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <string>

namespace
{
const void * AsPtr(long value)
{
    return reinterpret_cast<const void *>(static_cast<intptr_t>(value));
}

long AsLong(const void * value)
{
    return static_cast<long>(reinterpret_cast<intptr_t>(value));
}

// Saves/restores the process-global gpDataHelper around a test. Declaring this
// before any CLand/CUnit fixtures ensures it is destroyed *after* them, so
// CLand's destructor (which can indirectly recompute unit weights) never reads
// a stale gpDataHelper left dangling by a previously-run test in this binary.
class ScopedDataHelper
{
public:
    explicit ScopedDataHelper(CGameDataHelper * helper) : previous(gpDataHelper)
    {
        gpDataHelper = helper;
    }
    ~ScopedDataHelper()
    {
        gpDataHelper = previous;
    }

private:
    CGameDataHelper * previous;
};

class TestModelDataHelper : public CGameDataHelper
{
public:
    TestModelDataHelper()
    {
        moveNameLabels = {"walk", "ride", "wagon", "fly", "swim"};
        for (int i = 0; i < MOVE_MODE_MAX; i++)
            moveNamePtrs[i] = moveNameLabels[i].c_str();
    }

    void SetItemWeight(const std::string & item, std::array<int, MOVE_MODE_MAX> weights)
    {
        itemWeights[item] = weights;
    }

    bool GetItemWeights(const char * item, int *& weights, const char **& movenames, int & movecount) override
    {
        auto it = itemWeights.find(item ? item : "");
        if (it == itemWeights.end())
            return false;
        weights = it->second.data();
        movenames = moveNamePtrs.data();
        movecount = MOVE_MODE_MAX;
        return true;
    }

    void GetMoveNames(const char **& movenames) override
    {
        movenames = moveNamePtrs.data();
    }

    bool IsWagon(const char * item) override
    {
        return item && wagons.count(item) > 0;
    }

    bool IsWagonPuller(const char * item) override
    {
        return item && wagonPullers.count(item) > 0;
    }

    int WagonCapacity() override
    {
        return wagonCapacity;
    }

    std::set<std::string> wagons;
    std::set<std::string> wagonPullers;
    int wagonCapacity = 0;

private:
    std::map<std::string, std::array<int, MOVE_MODE_MAX>> itemWeights;
    std::array<std::string, MOVE_MODE_MAX> moveNameLabels;
    std::array<const char *, MOVE_MODE_MAX> moveNamePtrs;
};
}

//=============================================================
// CBaseObject

TEST_CASE("CBaseObject SetName preserves the original name across multiple edits", "[model]")
{
    CBaseObject obj;
    obj.Name = "Original";

    obj.SetName("First");
    obj.SetName("Second");

    EValueType type;
    const void * value;
    REQUIRE(obj.GetProperty(PRP_ORG_NAME, type, value, eNormal));
    CHECK(std::string((const char *)value) == "Original");
    CHECK(obj.Name == "Second");
}

TEST_CASE("CBaseObject ResetName restores the original name", "[model]")
{
    CBaseObject obj;
    obj.Name = "Original";
    obj.SetName("Changed");

    obj.ResetName();

    CHECK(obj.Name == "Original");
}

TEST_CASE("CBaseObject SetDescription preserves the original across multiple edits; ResetDescription restores it", "[model]")
{
    CBaseObject obj;
    obj.Description = "Original description";

    obj.SetDescription("First");
    obj.SetDescription("Second");

    EValueType type;
    const void * value;
    REQUIRE(obj.GetProperty(PRP_ORG_DESCR, type, value, eNormal));
    CHECK(std::string((const char *)value) == "Original description");

    obj.ResetDescription();
    CHECK(obj.Description == "Original description");
}

TEST_CASE("CBaseObject GetProperty falls back to Id, Name and Description when not stored as properties", "[model]")
{
    CBaseObject obj;
    obj.Id = 42;
    obj.Name = "Bob";
    obj.Description = "A hero";

    EValueType type;
    const void * value;
    REQUIRE(obj.GetProperty(PRP_ID, type, value));
    CHECK(type == eLong);
    CHECK(AsLong(value) == 42);

    REQUIRE(obj.GetProperty(PRP_NAME, type, value));
    CHECK(type == eCharPtr);
    CHECK(std::string((const char *)value) == "Bob");

    REQUIRE(obj.GetProperty(PRP_FULL_TEXT, type, value));
    CHECK(std::string((const char *)value) == "A hero");
}

TEST_CASE("CBaseObject Clear resets Id, Name and Description but leaves stored properties intact", "[model]")
{
    CBaseObject obj;
    obj.Id = 1;
    obj.Name = "X";
    obj.Description = "Y";
    obj.SetProperty("custom", eLong, AsPtr(9));

    obj.Clear();

    CHECK(obj.Id == 0);
    CHECK(obj.Name.empty());
    CHECK(obj.Description.empty());

    EValueType type;
    const void * value;
    REQUIRE(obj.GetProperty("custom", type, value));
    CHECK(AsLong(value) == 9);
}

//=============================================================
// CUnit weight calculation

TEST_CASE("CUnit CalcWeightsAndMovement sums per-item weights and picks the best movement mode", "[model]")
{
    TestModelDataHelper helper;
    ScopedDataHelper guard(&helper);

    CUnit unit;
    unit.SetProperty("men", eLong, AsPtr(3));
    unit.SetProperty("horse", eLong, AsPtr(2));
    helper.SetItemWeight("men", {2, 0, 0, 0, 0});
    helper.SetItemWeight("horse", {1, 5, 0, 0, 0});

    unit.CalcWeightsAndMovement();

    CHECK(unit.Weight[0] == 8);
    CHECK(unit.Weight[1] == 10);

    EValueType type;
    const void * value;
    REQUIRE(unit.GetProperty(PRP_MOVEMENT, type, value));
    CHECK(std::string((const char *)value) == "ride");
}

TEST_CASE("CUnit CalcWeightsAndMovement boosts ride capacity via a wagon+puller combination when otherwise immobile", "[model]")
{
    TestModelDataHelper helper;
    ScopedDataHelper guard(&helper);

    CUnit unit;
    unit.SetProperty("men", eLong, AsPtr(2));
    unit.SetProperty("horse", eLong, AsPtr(2));
    unit.SetProperty("wagon", eLong, AsPtr(1));
    helper.SetItemWeight("men", {5, 0, 0, 0, 0});
    helper.wagonPullers.insert("horse");
    helper.wagons.insert("wagon");
    helper.wagonCapacity = 10;

    unit.CalcWeightsAndMovement();

    CHECK(unit.Weight[0] == 10);
    CHECK(unit.Weight[1] == 10);

    EValueType type;
    const void * value;
    REQUIRE(unit.GetProperty(PRP_MOVEMENT, type, value));
    CHECK(std::string((const char *)value) == "ride");
}

TEST_CASE("CUnit CalcWeightsAndMovement appends the swim mode when swim capacity covers the load", "[model]")
{
    TestModelDataHelper helper;
    ScopedDataHelper guard(&helper);

    CUnit unit;
    unit.SetProperty("men", eLong, AsPtr(3));
    unit.SetProperty("fins", eLong, AsPtr(1));
    helper.SetItemWeight("men", {2, 0, 0, 0, 0});
    helper.SetItemWeight("fins", {0, 0, 0, 0, 10});

    unit.CalcWeightsAndMovement();

    CHECK(unit.Weight[0] == 6);
    CHECK(unit.Weight[4] == 10);

    EValueType type;
    const void * value;
    REQUIRE(unit.GetProperty(PRP_MOVEMENT, type, value));
    CHECK(std::string((const char *)value) == "walk,swim");
}

TEST_CASE("CUnit CalcWeightsAndMovement leaves weights zero and movement empty when no item weight table matches", "[model]")
{
    TestModelDataHelper helper;
    ScopedDataHelper guard(&helper);

    CUnit unit;
    unit.SetProperty("mystery", eLong, AsPtr(5));

    unit.CalcWeightsAndMovement();

    CHECK(unit.Weight[0] == 0);

    EValueType type;
    const void * value;
    REQUIRE(unit.GetProperty(PRP_MOVEMENT, type, value));
    CHECK(std::string((const char *)value).empty());
}

TEST_CASE("CUnit ResetNormalProperties recomputes weights from current properties", "[model]")
{
    TestModelDataHelper helper;
    ScopedDataHelper guard(&helper);

    CUnit unit;
    unit.SetProperty("men", eLong, AsPtr(3));
    helper.SetItemWeight("men", {2, 0, 0, 0, 0});
    unit.Weight[0] = 999; // stale value from before the reset

    unit.ResetNormalProperties();

    CHECK(unit.Weight[0] == 6);
}

TEST_CASE("CUnit CheckWeight reports no error when nothing is overloaded", "[model]")
{
    TestModelDataHelper helper;
    ScopedDataHelper guard(&helper);

    CUnit unit;
    unit.Weight[0] = 5;
    unit.Weight[1] = 10;

    std::string err;
    unit.CheckWeight(err);

    CHECK(err.empty());
}

TEST_CASE("CUnit CheckWeight flags overloaded when a lower move mode is exceeded and no higher mode compensates", "[model]")
{
    TestModelDataHelper helper;
    ScopedDataHelper guard(&helper);

    CUnit unit;
    unit.Weight[0] = 20;
    unit.Weight[1] = 10;

    std::string err;
    unit.CheckWeight(err);

    CHECK(err == " - Could ride but is overloaded.");
}

TEST_CASE("CUnit CheckWeight clears the overload when a higher move mode still works", "[model]")
{
    TestModelDataHelper helper;
    ScopedDataHelper guard(&helper);

    CUnit unit;
    unit.Weight[0] = 20;
    unit.Weight[1] = 10;
    unit.Weight[3] = 25;

    std::string err;
    unit.CheckWeight(err);

    CHECK(err.empty());
}

//=============================================================
// CLand::AddUnit / RemoveUnit

TEST_CASE("CLand AddUnit inserts into Units and UnitsSeq, sets LandId, LAND_UNITS flag, and PRP_SEQUENCE", "[model]")
{
    ScopedDataHelper guard(nullptr);
    // CUnit locals are declared before CLand: land holds raw (non-owning) pointers
    // to them, so land must be destroyed first - reverse-declaration-order
    // destruction only gives that if land is declared *after* the units it will
    // point at (destroying it earlier via CLand-first ordering reads through
    // already-destroyed CUnit stack storage in ~CLand).
    CUnit unit1, unit2;
    unit1.Id = 1;
    unit2.Id = 2;
    CLand land;
    land.Id = 100;

    REQUIRE(land.AddUnit(&unit1));
    REQUIRE(land.AddUnit(&unit2));

    CHECK(land.Units.Count() == 2);
    CHECK(land.UnitsSeq.Count() == 2);
    CHECK(unit1.LandId == 100);
    CHECK((land.Flags & LAND_UNITS) != 0);

    EValueType type;
    const void * value;
    REQUIRE(unit1.GetProperty(PRP_SEQUENCE, type, value));
    CHECK(AsLong(value) == 1);
    REQUIRE(unit2.GetProperty(PRP_SEQUENCE, type, value));
    CHECK(AsLong(value) == 2);
}

TEST_CASE("CLand AddUnit rejects a duplicate unit Id", "[model]")
{
    ScopedDataHelper guard(nullptr);
    CUnit unit1, unit2;
    unit1.Id = 5;
    unit2.Id = 5;
    CLand land;

    REQUIRE(land.AddUnit(&unit1));
    CHECK_FALSE(land.AddUnit(&unit2));
    CHECK(land.Units.Count() == 1);
    CHECK(land.UnitsSeq.Count() == 1);
}

TEST_CASE("CLand RemoveUnit unlinks from Units and UnitsSeq but does not free the unit", "[model]")
{
    ScopedDataHelper guard(nullptr);
    CUnit unit1;
    unit1.Id = 7;
    CLand land;
    REQUIRE(land.AddUnit(&unit1));

    land.RemoveUnit(&unit1);

    CHECK(land.Units.Count() == 0);
    CHECK(land.UnitsSeq.Count() == 0);
    CHECK(unit1.Id == 7); // still a valid object - ownership returned to the caller
}

TEST_CASE("CLand RemoveUnit on a unit that was never added is a no-op", "[model]")
{
    ScopedDataHelper guard(nullptr);
    CUnit unit1, unit2;
    unit1.Id = 1;
    unit2.Id = 2;
    CLand land;
    REQUIRE(land.AddUnit(&unit1));

    land.RemoveUnit(&unit2);

    CHECK(land.Units.Count() == 1);
    CHECK(land.UnitsSeq.Count() == 1);
}

TEST_CASE("CLand LAND_UNITS flag stays set after the last unit is removed", "[model]")
{
    ScopedDataHelper guard(nullptr);
    CUnit unit1;
    unit1.Id = 1;
    CLand land;
    REQUIRE(land.AddUnit(&unit1));

    land.RemoveUnit(&unit1);

    CHECK(land.Units.Count() == 0);
    CHECK((land.Flags & LAND_UNITS) != 0);
}

//=============================================================
// CLand::CalcStructsLoad

TEST_CASE("CLand CalcStructsLoad sums occupant Weight[0] via PRP_STRUCT_ID", "[model]")
{
    ScopedDataHelper guard(nullptr);
    CUnit unit1, unit2;
    unit1.Id = 1;
    unit2.Id = 2;
    CLand land;
    CStruct * pStruct = new CStruct();
    pStruct->Id = 10;
    land.AddNewStruct(pStruct);

    unit1.Weight[0] = 30;
    unit2.Weight[0] = 20;
    unit1.SetProperty(PRP_STRUCT_ID, eLong, AsPtr(10));
    unit2.SetProperty(PRP_STRUCT_ID, eLong, AsPtr(10));
    REQUIRE(land.AddUnit(&unit1));
    REQUIRE(land.AddUnit(&unit2));

    land.CalcStructsLoad();

    CHECK(pStruct->Load == 50);
}

TEST_CASE("CLand CalcStructsLoad is idempotent across repeated calls", "[model]")
{
    ScopedDataHelper guard(nullptr);
    CUnit unit1;
    unit1.Id = 1;
    CLand land;
    CStruct * pStruct = new CStruct();
    pStruct->Id = 10;
    land.AddNewStruct(pStruct);

    unit1.Weight[0] = 40;
    unit1.SetProperty(PRP_STRUCT_ID, eLong, AsPtr(10));
    REQUIRE(land.AddUnit(&unit1));

    land.CalcStructsLoad();
    CHECK(pStruct->Load == 40);

    land.CalcStructsLoad();
    CHECK(pStruct->Load == 40); // would be 80 without the idempotency fix
}

//=============================================================
// CLand edge structures

TEST_CASE("CLand EdgeStructs allows multiple zero-Id structs to coexist at different directions", "[model]")
{
    CLand land;
    land.AddNewEdgeStruct("Road", 0);
    land.AddNewEdgeStruct("Gate", 3);

    CHECK(land.EdgeStructs.Count() == 2);
}

TEST_CASE("CLand RemoveEdgeStructs removes only structs at the matching direction", "[model]")
{
    CLand land;
    land.AddNewEdgeStruct("Road", 0);
    land.AddNewEdgeStruct("Gate", 3);

    land.RemoveEdgeStructs(0);

    REQUIRE(land.EdgeStructs.Count() == 1);
    CHECK(((CStruct *)land.EdgeStructs.At(0))->Kind == "Gate");
}

TEST_CASE("CLand AddNewEdgeStruct normalizes a negative direction on insert", "[model]")
{
    CLand land;
    land.AddNewEdgeStruct("Road", -1); // should normalize to Location 5

    REQUIRE(land.EdgeStructs.Count() == 1);
    CHECK(((CStruct *)land.EdgeStructs.At(0))->Location == 5);
}

TEST_CASE("CLand RemoveEdgeStructs normalizes a negative direction", "[model]")
{
    CLand land;
    land.AddNewEdgeStruct("Road", 5);

    land.RemoveEdgeStructs(-1); // -1 should normalize to 5, matching the struct above

    CHECK(land.EdgeStructs.Count() == 0);
}

//=============================================================
// CLand::AddNewStruct

TEST_CASE("CLand AddNewStruct merges a duplicate Id into the existing struct", "[model]")
{
    CLand land;
    CStruct * first = new CStruct();
    first->Id = 1;
    first->Name = "Tower";
    first->Attr = SA_GATE;
    land.AddNewStruct(first);

    CStruct * second = new CStruct();
    second->Id = 1;
    second->Name = "Tower Renamed";
    second->OwnerUnitId = 42;

    CStruct * result = land.AddNewStruct(second); // second is merged into first and freed

    CHECK(result == first);
    CHECK(land.Structs.Count() == 1);
    CHECK(first->Name == "Tower Renamed");
    CHECK(first->OwnerUnitId == 42);
}

TEST_CASE("CLand AddNewStruct folds new Attr bits into CLand::Flags on first insert", "[model]")
{
    CLand land;
    CStruct * s = new CStruct();
    s->Id = 1;
    s->Attr = SA_GATE;
    land.AddNewStruct(s);

    CHECK((land.Flags & LAND_STR_GATE) != 0);
}

//=============================================================
// CStruct

TEST_CASE("CStruct ResetNormalProperties zeroes Load and SailingPower", "[model]")
{
    CStruct s;
    s.Load = 40;
    s.SailingPower = 5;

    s.ResetNormalProperties();

    CHECK(s.Load == 0);
    CHECK(s.SailingPower == 0);
}

//=============================================================
// CPlane

TEST_CASE("CPlane default construction sets the documented sentinel values", "[model]")
{
    CPlane plane;

    CHECK(plane.EastEdge == 0);
    CHECK(plane.WestEdge == 0);
    CHECK(plane.Width == 0);
    CHECK(plane.TropicZoneMin == TROPIC_ZONE_MAX);
    CHECK(plane.TropicZoneMax == -TROPIC_ZONE_MAX);
    CHECK(plane.Lands.Count() == 0);
}

TEST_CASE("CPlane Lands behaves like a standard id-sorted collection", "[model]")
{
    CPlane plane;
    CLand * land1 = new CLand();
    land1->Id = 1;
    CLand * land2 = new CLand();
    land2->Id = 1;

    REQUIRE(plane.Lands.Insert(land1));
    CHECK_FALSE(plane.Lands.Insert(land2));
    delete land2;

    CHECK(plane.Lands.Count() == 1);
}

// Regression test for the Faction Overview crash: GetPropertyName returns ""
// (not nullptr) past the last property, so iterating via std::string assignment
// and !propname.empty() terminates safely without a null-pointer dereference.
TEST_CASE("TPropertyHolder GetPropertyName returns empty string past the last property", "[model]")
{
    CBaseObject obj;

    // With no properties, index 0 should already be past the end.
    CHECK(std::string(obj.GetPropertyName(0)) == "");

    // Add one property and verify iteration: index 0 returns the name, index 1
    // returns "" (terminates the loop).
    obj.SetProperty("speed", eLong, reinterpret_cast<const void *>(static_cast<intptr_t>(42L)), eNormal);
    CHECK(std::string(obj.GetPropertyName(0)) == "speed");
    CHECK(std::string(obj.GetPropertyName(1)) == "");
}
