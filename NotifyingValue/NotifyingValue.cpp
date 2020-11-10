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

    NotifyingValueBase(const string& key)
        : mKey(key)
    {}

public:
    virtual void SendUpdate() const = 0;

    virtual void UpdateValue(string value) = 0;

    const string& key() const
    {
        return mKey;
    }
};

// Basic listener that just prints changes to console; can be replaced with custom notification mechanism 
class NotificationDelegate
{
public:
    virtual void SendUpdate(const string& member, const string& value)
    {
        cout << member << " updated, new value: " << value << endl;
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

    void SendUpdate() const;

    void SendUpdate(const string& key, const string& value) const;

    bool Update(const string& key, const string& value);
};

// Notification class for scalar values
template<typename T>
class NotifyingValue : public NotifyingValueBase
{
private:
    T mValue;

public:
    NotifyingValue(const string& name, const T& value) 
        : NotifyingValueBase(name), mValue(value)
    {
        ValueNotificationManager::GetInstance().Add(*this);
    }

    NotifyingValue(string name)
        : NotifyingValue(name, { 0 }) {}
        

    T operator=(const T& other)
    {
        mValue = other;
        SendUpdate();
        return mValue;
    }

    operator T() const
    {
        return mValue;
    }

    void SendUpdate() const override
    {
        stringstream ss;
        ss << mValue;
        ValueNotificationManager::GetInstance().SendUpdate(mKey, ss.str());
    }

    void UpdateValue(string value) override
    {
        stringstream ss(value);
        ss >> mValue;
    }

};

// Notification class for elements of arrays
template<typename T>
class NotifyingArrayMember : public NotifyingValueBase
{
private:
    T mValue;
    NotifyingValueBase const* mParentArray;

public:
    NotifyingArrayMember()
        : NotifyingValueBase{ "el" }, mValue{ 0 }, mParentArray(nullptr)
    {}

    void SetParent(NotifyingValueBase& parentArray)
    {
        mParentArray = &parentArray;
    }

    T operator=(const T& other)
    {
        mValue = other;
        mParentArray->SendUpdate();
        return mValue;
    }

    operator T() const
    {
        return mValue;
    }

    void SendUpdate() const override
    {
        // nothing here, we send updates for the whole array, not each element individually 
    }

    void UpdateValue(string value) override
    {
        stringstream ss(value);
        ss >> mValue;
    }
};

// Notification class for arrays
template<typename T, size_t Size>
class NotifyingArray : public NotifyingValueBase
{
private:
    array<NotifyingArrayMember<T>, Size> mArray;

public:
    NotifyingArray(const string& name)
        : NotifyingValueBase(name), mArray { }
    {
        for (auto& v : mArray)
        {
            v.SetParent(*this);
        }
        ValueNotificationManager::GetInstance().Add(*this);
    }

    NotifyingArrayMember<T>& operator [](size_t i)
    {
        return mArray[i];
    }

    void SendUpdate() const override
    {
        stringstream ss;
        ss << "[";
        for (auto& v : mArray)
        {
            ss << (T)v << " ";
        }
        ss << "]";
        ValueNotificationManager::GetInstance().SendUpdate(mKey, ss.str());
    }

    void UpdateValue(string value) override
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

void ValueNotificationManager::SendUpdate() const
{
    for (const auto* v : mp_Values)
    {
        v->SendUpdate();
    }
}

void ValueNotificationManager::SendUpdate(const string& key, const string& value) const
{
    mCurrentNotificatinoDelegate.SendUpdate(key, value);
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

// example substructure that is used in the main example strucutre 
struct S1
{
    int i1;
    NOTIFICATION_VAR_DEFAULT_VAL(double, d1, 1.0);
    NOTIFICATION_ARRAY(float, 7, af1);
};

enum Colors : int { Red, Blue, White, Yellow, Black };

// create global structure of values that we monitor changes of
struct Values
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


int main()
{
    // Ask all values to send their value notifications: these will be default values
    ValueNotificationManager::GetInstance().SendUpdate();

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
    ValueNotificationManager::GetInstance().SendUpdate();

    // update values trough strings methods
    ValueNotificationManager::GetInstance().Update("i1", "45");
}
