#include "catch.hpp"

#include "data.h"

#include <cstdint>
#include <map>
#include <string>

namespace
{
class TestGroupedHolder : public TPropertyHolder
{
public:
    std::multimap<std::string, std::string> * GetPropertyGroups() override
    {
        return &groups;
    }

    std::multimap<std::string, std::string> groups;
};

const void * AsPtr(long value)
{
    return reinterpret_cast<const void *>(static_cast<intptr_t>(value));
}

long AsLong(const void * value)
{
    return static_cast<long>(reinterpret_cast<intptr_t>(value));
}
}

//=============================================================
// TPropertyHolder

TEST_CASE("TPropertyHolder round-trips an eLong property", "[property]")
{
    TPropertyHolder holder;
    REQUIRE(PE_OK == holder.SetProperty("count", eLong, AsPtr(42)));

    EValueType type;
    const void * value;
    REQUIRE(holder.GetProperty("count", type, value));
    CHECK(type == eLong);
    CHECK(AsLong(value) == 42);
}

TEST_CASE("TPropertyHolder round-trips an eCharPtr property", "[property]")
{
    TPropertyHolder holder;
    REQUIRE(PE_OK == holder.SetProperty("name", eCharPtr, "hello"));

    EValueType type;
    const void * value;
    REQUIRE(holder.GetProperty("name", type, value));
    CHECK(type == eCharPtr);
    CHECK(std::string((const char *)value) == "hello");
}

TEST_CASE("TPropertyHolder SetProperty rejects a value-type change on an existing property", "[property]")
{
    TPropertyHolder holder;
    REQUIRE(PE_OK == holder.SetProperty("x", eLong, AsPtr(1)));

    CHECK(PE_INV_VALUE_TYPE == holder.SetProperty("x", eCharPtr, "oops"));

    EValueType type;
    const void * value;
    REQUIRE(holder.GetProperty("x", type, value));
    CHECK(type == eLong);
    CHECK(AsLong(value) == 1);
}

TEST_CASE("TPropertyHolder SetProperty seeds normal and original identically on first creation", "[property]")
{
    TPropertyHolder holder;
    // proptype is ignored the first time a property is created - both slots get the same value.
    REQUIRE(PE_OK == holder.SetProperty("x", eLong, AsPtr(7), eOriginal));

    EValueType type;
    const void * value;
    REQUIRE(holder.GetProperty("x", type, value, eNormal));
    CHECK(AsLong(value) == 7);
    REQUIRE(holder.GetProperty("x", type, value, eOriginal));
    CHECK(AsLong(value) == 7);
}

TEST_CASE("TPropertyHolder ResetNormalProperties restores the value from before a normal-only edit", "[property]")
{
    TPropertyHolder holder;
    REQUIRE(PE_OK == holder.SetProperty("x", eLong, AsPtr(1), eBoth));
    REQUIRE(PE_OK == holder.SetProperty("x", eLong, AsPtr(2), eNormal));

    EValueType type;
    const void * value;
    REQUIRE(holder.GetProperty("x", type, value, eNormal));
    CHECK(AsLong(value) == 2);
    REQUIRE(holder.GetProperty("x", type, value, eOriginal));
    CHECK(AsLong(value) == 1);

    holder.ResetNormalProperties();

    REQUIRE(holder.GetProperty("x", type, value, eNormal));
    CHECK(AsLong(value) == 1);
}

TEST_CASE("TPropertyHolder GetProperty on an unknown name fails without touching the out-params", "[property]")
{
    TPropertyHolder holder;

    EValueType type = eCharPtr;
    const void * value = (const void *)0x1234;
    CHECK_FALSE(holder.GetProperty("doesnotexist", type, value));
    CHECK(type == eCharPtr);
    CHECK(value == (const void *)0x1234);
}

TEST_CASE("TPropertyHolder DelProperty removes the property", "[property]")
{
    TPropertyHolder holder;
    REQUIRE(PE_OK == holder.SetProperty("x", eLong, AsPtr(1)));
    holder.DelProperty("x");

    EValueType type;
    const void * value;
    CHECK_FALSE(holder.GetProperty("x", type, value));
}

TEST_CASE("TPropertyHolder GetProperty sums eLong values across a property group", "[property]")
{
    TestGroupedHolder holder;
    REQUIRE(PE_OK == holder.SetProperty("sword", eLong, AsPtr(3)));
    REQUIRE(PE_OK == holder.SetProperty("axe", eLong, AsPtr(5)));
    holder.groups.insert({"weapons", "sword"});
    holder.groups.insert({"weapons", "axe"});

    EValueType type;
    const void * value;
    REQUIRE(holder.GetProperty("weapons", type, value));
    CHECK(type == eLong);
    CHECK(AsLong(value) == 8);
}

//=============================================================
// TPropertyColl

TEST_CASE("TPropertyColl insert on an existing key keeps a single entry with the latest value", "[property]")
{
    TPropertyColl coll;
    coll.insert(new TProperty("x", eLong, AsPtr(1)));
    coll.insert(new TProperty("x", eLong, AsPtr(2)));

    CHECK(coll.count() == 1);
    TProperty * p = coll.find("x");
    REQUIRE(p);
    CHECK(AsLong(p->m_value) == 2);

    coll.freeAll();
}

//=============================================================
// TPropertyHolderColl

TEST_CASE("TPropertyHolderColl SetSortMode orders items ascending by a single eLong key", "[property]")
{
    TPropertyHolder h1, h2, h3;
    h1.SetProperty("sortkey", eLong, AsPtr(30));
    h2.SetProperty("sortkey", eLong, AsPtr(10));
    h3.SetProperty("sortkey", eLong, AsPtr(20));

    TPropertyHolderColl coll;
    coll.AtInsert(0, &h1);
    coll.AtInsert(1, &h2);
    coll.AtInsert(2, &h3);

    const char * keys[] = {"sortkey"};
    coll.SetSortMode(keys, 1);

    REQUIRE(coll.Count() == 3);
    CHECK(coll.At(0) == &h2);
    CHECK(coll.At(1) == &h3);
    CHECK(coll.At(2) == &h1);
}

TEST_CASE("TPropertyHolderColl SetSortMode falls through to a second key on ties", "[property]")
{
    TPropertyHolder h1, h2;
    h1.SetProperty("primary", eLong, AsPtr(1));
    h1.SetProperty("secondary", eLong, AsPtr(2));
    h2.SetProperty("primary", eLong, AsPtr(1));
    h2.SetProperty("secondary", eLong, AsPtr(1));

    TPropertyHolderColl coll;
    coll.AtInsert(0, &h1);
    coll.AtInsert(1, &h2);

    const char * keys[] = {"primary", "secondary"};
    coll.SetSortMode(keys, 2);

    REQUIRE(coll.Count() == 2);
    CHECK(coll.At(0) == &h2);
    CHECK(coll.At(1) == &h1);
}

TEST_CASE("TPropertyHolderColl SetSortMode is stable when all items tie", "[property]")
{
    TPropertyHolder h1, h2, h3;
    h1.SetProperty("sortkey", eLong, AsPtr(5));
    h2.SetProperty("sortkey", eLong, AsPtr(5));
    h3.SetProperty("sortkey", eLong, AsPtr(5));

    TPropertyHolderColl coll;
    coll.AtInsert(0, &h1);
    coll.AtInsert(1, &h2);
    coll.AtInsert(2, &h3);

    const char * keys[] = {"sortkey"};
    coll.SetSortMode(keys, 1);

    REQUIRE(coll.Count() == 3);
    CHECK(coll.At(0) == &h1);
    CHECK(coll.At(1) == &h2);
    CHECK(coll.At(2) == &h3);
}

TEST_CASE("TPropertyHolderColl AtInsert bypasses the established sort order", "[property]")
{
    TPropertyHolder h1, h2, hLast;
    h1.SetProperty("sortkey", eLong, AsPtr(20));
    h2.SetProperty("sortkey", eLong, AsPtr(10));
    hLast.SetProperty("sortkey", eLong, AsPtr(1)); // would sort first, but is appended after sorting

    TPropertyHolderColl coll;
    coll.AtInsert(0, &h1);
    coll.AtInsert(1, &h2);

    const char * keys[] = {"sortkey"};
    coll.SetSortMode(keys, 1);
    coll.AtInsert((int)coll.Count(), &hLast);

    REQUIRE(coll.Count() == 3);
    CHECK(coll.At(0) == &h2);
    CHECK(coll.At(1) == &h1);
    CHECK(coll.At(2) == &hLast);
}

//=============================================================
// CBaseCollById

TEST_CASE("CBaseCollById Insert rejects a duplicate Id and preserves the original entry", "[property]")
{
    CBaseCollById coll;
    CBaseObject * a = new CBaseObject();
    a->Id = 5;
    a->Name = "A";
    REQUIRE(coll.Insert(a));

    CBaseObject * b = new CBaseObject();
    b->Id = 5;
    b->Name = "B";
    CHECK_FALSE(coll.Insert(b));
    delete b;

    CHECK(coll.Count() == 1);
    CHECK(((CBaseObject *)coll.At(0))->Name == "A");
}

TEST_CASE("CBaseCollById maintains ascending Id order regardless of insertion order", "[property]")
{
    CBaseCollById coll;
    for (long id : {30, 10, 20})
    {
        CBaseObject * item = new CBaseObject();
        item->Id = id;
        REQUIRE(coll.Insert(item));
    }

    REQUIRE(coll.Count() == 3);
    CHECK(((CBaseObject *)coll.At(0))->Id == 10);
    CHECK(((CBaseObject *)coll.At(1))->Id == 20);
    CHECK(((CBaseObject *)coll.At(2))->Id == 30);
}

TEST_CASE("CBaseCollById Search finds an item via a dummy key object", "[property]")
{
    CBaseCollById coll;
    for (long id : {30, 10, 20})
    {
        CBaseObject * item = new CBaseObject();
        item->Id = id;
        REQUIRE(coll.Insert(item));
    }

    CBaseObject dummy;
    dummy.Id = 20;
    int index;
    REQUIRE(coll.Search(&dummy, index));
    CHECK(((CBaseObject *)coll.At(index))->Id == 20);

    dummy.Id = 99;
    CHECK_FALSE(coll.Search(&dummy, index));
}

TEST_CASE("CBaseCollById AtInsert bypasses duplicate-Id rejection", "[property]")
{
    CBaseCollById coll;
    CBaseObject * a = new CBaseObject();
    a->Id = 5;
    CBaseObject * b = new CBaseObject();
    b->Id = 5;

    coll.AtInsert(0, a);
    coll.AtInsert(1, b);

    CHECK(coll.Count() == 2);
}

//=============================================================
// CBaseCollByName

TEST_CASE("CBaseCollByName rejects duplicates case-insensitively", "[property]")
{
    CBaseCollByName coll;
    CBaseObject * a = new CBaseObject();
    a->Name = "Bob";
    REQUIRE(coll.Insert(a));

    CBaseObject * b = new CBaseObject();
    b->Name = "BOB";
    CHECK_FALSE(coll.Insert(b));
    delete b;

    CHECK(coll.Count() == 1);
}

TEST_CASE("CBaseCollByName orders items by name", "[property]")
{
    CBaseCollByName coll;
    for (const char * name : {"Charlie", "Alice", "Bob"})
    {
        CBaseObject * item = new CBaseObject();
        item->Name = name;
        REQUIRE(coll.Insert(item));
    }

    REQUIRE(coll.Count() == 3);
    CHECK(((CBaseObject *)coll.At(0))->Name == "Alice");
    CHECK(((CBaseObject *)coll.At(1))->Name == "Bob");
    CHECK(((CBaseObject *)coll.At(2))->Name == "Charlie");
}
