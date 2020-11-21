#include <iostream>
#include <sstream> 
#include <vector>
#include <array>
using namespace std;

// This code is a proof of concept for data structures that can notify their data changes to arbitrary listeners.
// This is useful, for example, for observing global C-style structures in legacy code when we need to monitor when
// values are changed by some code.

// Abstract base class that holds an arbitrary piece of data and send notification when updated
class NotifyingValueBase
{
protected:
    string mKey;
    const NotifyingValueBase* mpParent;

    NotifyingValueBase(const NotifyingValueBase* parent, const string& key)
        : mpParent(parent), mKey(key)
    {}

    NotifyingValueBase(const string& key)
        : NotifyingValueBase(nullptr, key)
    {}

    NotifyingValueBase()
        : NotifyingValueBase(nullptr, "")
    {}

public:
    virtual void SendUpdate() const = 0;

    virtual void UpdateValue(const string& value) = 0;

    const string& key() const
    {
        return mKey;
    }

    const NotifyingValueBase* parent() const
    {
        return mpParent;
    }
};

// Basic listener that just prints changes to console; can be replaced with custom notification mechanism 
class NotificationDelegate
{
public:
    virtual void SendUpdate(const string& key, const string& value)
    {  
        cout << key << " updated, new value: " << value << endl;
    }
};


// Singleton to manage all values that send updates
class ValueNotificationManager
{
private:
    static ValueNotificationManager* mInstance;

    // vector that stores all values that send notification
    vector<NotifyingValueBase*> mp_Values;

    NotificationDelegate mDefaultNotificationDelegate;

    NotificationDelegate& mCurrentNotificatinoDelegate;

public:
    static ValueNotificationManager& GetInstance();

    ValueNotificationManager()
        : mCurrentNotificatinoDelegate(mDefaultNotificationDelegate)
    {
    }

    void Add(NotifyingValueBase& notifyingValue);

    void NotifyAll() const;

    void SendUpdate(const NotifyingValueBase* item, const string& value) const;

    bool Update(const string& key, const string& value);
};

// Notification class for scalar values
template<typename T>
class NotifyingValue : public NotifyingValueBase
{
private:
    T mValue;

public:
    // Constructors for variables inside of a structure
    NotifyingValue(const NotifyingValueBase* const parent, const string& name, const T& value)
        : NotifyingValueBase(parent, name), mValue(value)
    {
        ValueNotificationManager::GetInstance().Add(*this);
    }

    NotifyingValue(const NotifyingValueBase* const parent, const string& name)
        : NotifyingValue(parent, name, { 0 })
    {}

    // Constructors for variables outside of a structure
    NotifyingValue(string name, const T& value)
        : NotifyingValue(nullptr, name, value) 
    {}

    NotifyingValue(string name)
        : NotifyingValue(nullptr, name, { 0 }) 
    {}

    NotifyingValue()
        : mValue { 0 }
    {}

    void Init(const NotifyingValueBase* const parent, const string& name)
    {
        mpParent = parent;
        mKey = name;
        ValueNotificationManager::GetInstance().Add(*this);
    }
        
    NotifyingValue<T>& operator=(const T& other)
    {
        mValue = other;
        SendUpdate();
        return *this;
    }

    operator T() const
    {
        return mValue;
    }

    void SendUpdate() const override
    {
        stringstream ss;
        ss << mValue;
        ValueNotificationManager::GetInstance().SendUpdate(this, ss.str());
    }

    virtual void UpdateValue(const string& value) override
    {
        // TODO: implement deserialization from string
        //stringstream ss(value);
        //ss >> mValue;
    }

};

// Notification class for arrays
template<typename T, size_t Size>
class NotifyingArray : public NotifyingValueBase
{
private:
    array<NotifyingValue<T>, Size> mArray;

public:
    NotifyingArray(const NotifyingValueBase* const parent, const string& name)
        : NotifyingValueBase(parent, name), mArray{ }
    {
        for (size_t i = 0; i < mArray.size(); i++)
        {
            stringstream ss;
            ss << i;
            mArray[i].Init(this, ss.str());
        }
        ValueNotificationManager::GetInstance().Add(*this);
    }

    NotifyingArray(const string& name)
        : NotifyingArray(nullptr, name)
    {}

    NotifyingValue<T>& operator [](size_t i)
    {
        return mArray[i];
    }

    void SendUpdate() const override
    {
        // Each member sends its own update
    }

    void UpdateValue(const string& value) override
    {
        stringstream ss(value);
        string item;
        size_t idx = 0;
        while (getline(ss, item, ','))
        {
            if (idx < mArray.size())
            {
                mArray[idx].UpdateValue(item);
            }
        }
    }
};

class NotifyingStruct : public NotifyingValueBase
{
public:
    NotifyingStruct(const string& key)
        : NotifyingValueBase(key) { }

    NotifyingStruct(const NotifyingStruct* parent, const string& key)
        : NotifyingValueBase(parent, key) { }

    void SendUpdate() const override
    {
        // Each member sends its own update
    }

    void UpdateValue(const string& value) override
    {
        // TODO: ability to load a structure from a string (e.g. JSON)
    }

    friend ostream& operator<<(ostream& out, const NotifyingStruct& ns)
    {
        out << "<Struct " << ns.mKey << " updated>";
        return out;
    }
};

// Implementation of ValueNotificationManager
ValueNotificationManager* ValueNotificationManager::mInstance = nullptr;

ValueNotificationManager& ValueNotificationManager::GetInstance()
{
    if (mInstance == nullptr)
    {
        mInstance = new ValueNotificationManager();
    }
    return *mInstance;
}

void ValueNotificationManager::Add(NotifyingValueBase& notifyingValue)
{
    mp_Values.push_back(&notifyingValue);
}

void ValueNotificationManager::NotifyAll() const
{
    for (const auto* v : mp_Values)
    {
        v->SendUpdate();
    }
}

void ValueNotificationManager::SendUpdate(const NotifyingValueBase* item, const string& value) const
{
    vector<string> keys = { item->key() };

    auto parent = item->parent();
    while (parent != nullptr)
    {
        keys.push_back(parent->key());
        parent = parent->parent();
    }

    // assemble the key string
    stringstream ss;
    for (size_t idx = keys.size() - 1; idx > 0; idx--)
    {
        ss << keys[idx] << ".";
    }
    ss << keys[0];
    
    mCurrentNotificatinoDelegate.SendUpdate(ss.str(), value);
}

bool ValueNotificationManager::Update(const string& key, const string& value)
{

    for (NotifyingValueBase* v : mp_Values)
    {
        if (v->key() == key)
        {
            v->UpdateValue(value);
            return true;
        }
    }
    return false;
}

// Macro helpers to pass the variable name straight into the object constructor as the key
// This help us not having to repeat the variable name twice ("i1" in this example: "NotifyingValue<int> i1("i1")")
#define NOTIFICATION_VARIABLE(TYPE, NAME) NotifyingValue<TYPE> NAME = { #NAME }
#define NOTIFICATION_VAR_DEFAULT_VAL(TYPE, NAME, DEFAULT_VALUE) NotifyingValue<TYPE> NAME = { #NAME, DEFAULT_VALUE }
#define NOTIFICATION_ARRAY(TYPE, SIZE, NAME) NotifyingArray<TYPE, SIZE> NAME = { #NAME }

// example substructure that is used in the main example structure 
struct S1
{
    int i1;
    NOTIFICATION_VAR_DEFAULT_VAL(double, d1, 1.0);
    NOTIFICATION_ARRAY(float, 7, af1);
};

enum Colors : int { Red, Blue, White, Yellow, Black };

// create global structure of values that we monitor changes of
struct BasicValues
{
    // Scalars
    NOTIFICATION_VARIABLE(uint16_t, i1);
    int other0;
    NOTIFICATION_VARIABLE(float, f1);
    NOTIFICATION_VARIABLE(int32_t, i2);
    NOTIFICATION_VARIABLE(double, d1);

    // Enums
    Colors e1;
    NOTIFICATION_VAR_DEFAULT_VAL(int/*Colors*/, e2, Red);

    // Arrays
    uint32_t a1[4];
    NOTIFICATION_ARRAY(uint32_t, 4, a2);

    // Other structs
    S1 s1;
} my_values;

// Working with structs
#define NOTIFICATION_STRUCT_VARIABLE(TYPE, NAME) NotifyingValue<TYPE> NAME = { this, #NAME }
#define NOTIFICATION_STRUCT_VAR_DEFAULT_VAL(TYPE, NAME, DEFAULT_VALUE) NotifyingValue<TYPE> NAME = { this, #NAME, DEFAULT_VALUE }
#define NOTIFICATION_STRUCT(NAME) NAME() : NotifyingStruct(#NAME) {} \
    NAME(const NotifyingStruct* parent) : NotifyingStruct(parent, #NAME) {}
    // TOOD: delete default constructor and assignment operator
#define NOTIFICATION_SUBSTRUCT(TYPE, NAME) TYPE NAME = { this };
#define NOTIFICATION_STRUCT_ARRAY(TYPE, SIZE, NAME) NotifyingArray<TYPE, SIZE> NAME = { this, #NAME }

struct S1L1 : public NotifyingStruct
{
    // boilerplate stuff
    //S1L1() : NotifyingStruct("S1L1") {}
    //S1L1(const NotifyingStruct* parent) : NotifyingStruct(parent, "S1L1") {}
    NOTIFICATION_STRUCT(S1L1)

    //NotifyingValue<uint32_t> i1 = { this, "i1", 42 };
    NOTIFICATION_STRUCT_VAR_DEFAULT_VAL(uint32_t, i1, 42);

    NOTIFICATION_STRUCT_VARIABLE(double, d1);

    NOTIFICATION_STRUCT_ARRAY(int, 6, a1);

};

struct S1L2 : public NotifyingStruct
{
public:
    NOTIFICATION_STRUCT(S1L2)

    NOTIFICATION_STRUCT_VAR_DEFAULT_VAL(uint32_t, i1, 42);

    //S1L1 s1l1 = { this };
    NOTIFICATION_SUBSTRUCT(S1L1, s1l1);

    //NotifyingArray<S1L1, 2> sa1 = { this, "sa1" };
    NOTIFICATION_STRUCT_ARRAY(S1L1, 2, sa1);

};

int main()
{
    // Ask all values to send their value notifications: these will be default values
    ValueNotificationManager::GetInstance().NotifyAll();

    // We will access the struct members as if it were a basic C-style structure 
    cout << "Update some values\n";
    my_values.i1 = 42;
    my_values.d1 = 6.555;
    my_values.f1 = 0.33f;
    my_values.i2 = -56;
    my_values.e1 = Black;
    my_values.e2 = Blue;

    // change array values
    my_values.a1[0] = 5;
    my_values.a2[1] = 6;

    // sub structs
    my_values.s1.i1 = 5;
    my_values.s1.d1 = 5.5;
    my_values.s1.af1[0] = 3.3f;

    // read back some values (not notification should be sent)
    uint16_t i = my_values.i1;
    double d = my_values.s1.d1;
    float f = my_values.s1.af1[0];

    // comparisons
    if (my_values.i1 == 42)
        cout << "Comparison works\n";

    cout << "Again send all updates\n";
    ValueNotificationManager::GetInstance().NotifyAll();

    cout << "Update i1 \n";
    ValueNotificationManager::GetInstance().Update("i1", "45");

    ValueNotificationManager::GetInstance().NotifyAll();

    // Structure with name
    S1L1 s1l1;
    s1l1.i1 = 65;
    s1l1.a1[3] = 55;

    // Complex structure (members are other structures)
    S1L2 s1l2;
    s1l2.i1 = 72;
    s1l2.s1l1.i1 = 11;
    s1l2.s1l1.a1[1] = 66;

    // update array of structures
    //auto& a = s1l2.sa1[0];
    auto& t = s1l2.sa1[0]; // .i1 = 99;
    //t.i1 = 99;

    ValueNotificationManager::GetInstance().NotifyAll();
}
