# Observer Pattern

This pattern was inspired by [Game Programming Patterns](https://gameprogrammingpatterns.com/observer.html) and [
C++ Software Design - Guideline 25](https://www.oreilly.com/library/view/c-software-design/9781098113155/).

## When To Use

This pattern decouples operations that need to run synchronously/immediately based on state changes they do not own.

General:
* This pattern is best for a one-to-many relationship between Subject (State owner) and Observers (State observers).
* Allows the Subject to be loosely coupled by an interface to the Observers that depend on state changes owned by the Subject.

### Implementation Considerations:

Lifetime of Subject/Observers needs to be considered based on the systems requirements:
* Who is responsible for Attaching/Detaching Observers?
* Can Observers be destroyed while still Attached to a Subject?
* Can a Observer Attach to a Subject more than once?
* Can the Subject be destroyed with Attached Observers?

The interface for notifying Observers can have different styles.

Single function:
* Pass the Subject and a state change Tag
* Observers react to the tags they respond to and pull state required from the Subject
* Observer code can be templated to reuse with other Subject/Observer relationships
* Additional tags do not require immediate updates to Observers
* A std::function can be used instead of a virtual function

Function per change:
* Function per state change, pushing the new state, no need for a Tag
* Observers do not need to pull state from the Subject
* Observer interface cannot be reused, due to unique function names per state change
* Observers must implement each function even if they do not respond to the state change
* Extending the interface with new state changes require all Observers to be updated

## Features

Create a Subject with an Observer:
```cpp
enum class SubjectSystemTag
{
    Value
};

class SubjectSystem final : public Subject<SubjectSystem, SubjectSystemTag>
{
public:
    void SetValue(const int32_t value)
    {
        m_Value = value;
        SendNotification(SubjectSystemTag::Value);
    }

    int32_t GetValue() const{ return m_Value; }
private:
    int32_t m_Value{0};
};

class SubjectObserver final : public SubjectSystem::Observer
{
public:
    void OnNotification(const SubjectSystem& subject, const SubjectSystemTag tag) override
    {
        if(tag == SubjectSystemTag::Value)
        {
            subject.GetValue();
            ...
        }
    }
};
```

Attach and Trigger a Notification:
```cpp
{
    SubjectSystem subject{};
    SubjectObserver observer{};
    
    subject.AttachObserver(&observer);
    
    subject.SetValue(1); // Will trigger SubjectObserver::OnNotification
}
```

## Setup

This repository uses the .sln/.proj files created by Visual Studio 2022 Community Edition.
Using MSVC compiler, Preview version(C++23 Preview). 

### Catch2
The examples for how to use the pattern are written as Unit Tests.

Launching the program in Debug or Release will run the Unit Tests.

Alternative:
Installing the Test Adapter for Catch2 Visual Studio extension enables running the Unit Tests via the Test Explorer Window. Setup the Test Explorer to use the project's .runsettings file.

### vcpkg
This repository uses vcpkg in manifest mode for it's dependencies. To interact with vcpkg, open a Developer PowerShell (View -> Terminal).

To setup vcpkg, install it via the Visual Studio installer. To enable/disable it run these commands from the Developer PowerShell:
```
vcpkg integrate install
vcpkg integrate remove
```

To add additional dependencies run:
```
vcpkg add port [dependency name]
```

To update the version of a dependency modify the overrides section of vcpkg.json. 

To create a clean vcpkg.json and vcpkg-configuration.json file run:
```
vcpkg new --application
```

### TODO
- [x] Reference Semantics Implementation
- [x] Reference Semantics Implementation Unit Tests
- [x] Value Semantics Implementation Example
- [x] Value Semantics Implementation Example Unit Tests
- [x] Benchmarking
