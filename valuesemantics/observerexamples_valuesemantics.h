#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <set>
#include <memory>
#include <functional>

namespace ValueSemantics
{
    template<typename T>
    concept IsScopedEnum = std::is_scoped_enum_v<T>;

    template<typename SubjectT, IsScopedEnum TagT>
    class Observer
    {
    public:
        using OnNotificationFunc = std::function<void(const SubjectT&, TagT)>;

        Observer(OnNotificationFunc&& func)
            : m_OnNotification{std::move(func)}
        {
        }

        void OnNotification(const SubjectT& subject, const TagT tag)
        {
            m_OnNotification(subject, tag);
        }
    private:
        OnNotificationFunc m_OnNotification{};
    };

    template<typename SubjectT, IsScopedEnum TagT>
    class Subject
    {
    public:
        using Observer = Observer<SubjectT, TagT>;
        using Tag = TagT;

        void AttachObserver(Observer* const observer)
        {
            m_Observers.insert(observer);
        }

        void DetachObserver(Observer* const observer)
        {
            m_Observers.erase(observer);
        }
    protected:
        void SendNotification(const Tag tag) const
        {
            for(Observer* const observer : m_Observers)
            {
                observer->OnNotification(static_cast<const SubjectT&>(*this), tag);
            }
        }
    private:
        std::set<Observer*> m_Observers{};
    };

    enum class SubjectSystemTag
    {
        ValueA,
        ValueB,
    };

    class SubjectSystem final : public Subject<SubjectSystem, SubjectSystemTag>
    {
    public:
        void SetValueA(const int32_t value)
        {
            m_ValueA = value;
            SendNotification(SubjectSystemTag::ValueA);
        }

        void SetValueB(const int32_t value)
        {
            m_ValueB = value;
            SendNotification(SubjectSystemTag::ValueB);
        }

        int32_t GetValueA() const{ return m_ValueA; }
        int32_t GetValueB() const { return m_ValueB; }
    private:
        int32_t m_ValueA{0};
        int32_t m_ValueB{0};
    };

    class SubjectObserverA final : public SubjectSystem::Observer
    {
    public:
        SubjectObserverA()
            : SubjectSystem::Observer{
                [&value{m_Value}](const SubjectSystem& subject, const SubjectSystem::Tag tag)
                {
                    if(tag == SubjectSystem::Tag::ValueA)
                    {
                        value = subject.GetValueA();
                    }
                }}
        {
        }

        int32_t GetValue() const { return m_Value; }
    private:
        int32_t m_Value{0};
    };

    namespace
    {
        constexpr uint32_t creationCount{250'000};
        int32_t freeFuncValueB{0};
        void OnNotification(const SubjectSystem& subject, const SubjectSystem::Tag tag)
        {
            if(tag == SubjectSystem::Tag::ValueB)
            {
                freeFuncValueB = subject.GetValueB();
            }
        }
    }

    TEST_CASE("Observer - Value Semantics - Unit Tests")
    {
        SubjectSystem subject{};
        SubjectObserverA observerA{};

        int32_t lambdaValueB{0};
        SubjectSystem::Observer observerB{
            [&value{lambdaValueB}](const SubjectSystem& subject, const SubjectSystem::Tag tag)
            {
                if(tag == SubjectSystem::Tag::ValueB)
                {
                    value = subject.GetValueB();
                }
            }};

        freeFuncValueB = 0;
        SubjectSystem::Observer observerBB{OnNotification};

        REQUIRE(subject.GetValueA() == 0);
        REQUIRE(subject.GetValueB() == 0);
        REQUIRE(observerA.GetValue() == 0);
        REQUIRE(lambdaValueB == 0);
        REQUIRE(freeFuncValueB == 0);

        subject.AttachObserver(&observerA);
        subject.AttachObserver(&observerB);
        subject.AttachObserver(&observerBB);

        subject.SetValueA(1);

        REQUIRE(subject.GetValueA() == 1);
        REQUIRE(subject.GetValueB() == 0);
        REQUIRE(observerA.GetValue() == 1);
        REQUIRE(lambdaValueB == 0);
        REQUIRE(freeFuncValueB == 0);

        subject.SetValueB(2);

        REQUIRE(subject.GetValueA() == 1);
        REQUIRE(subject.GetValueB() == 2);
        REQUIRE(observerA.GetValue() == 1);
        REQUIRE(lambdaValueB == 2);
        REQUIRE(freeFuncValueB == 2);

        subject.DetachObserver(&observerBB);
        subject.SetValueB(3);

        REQUIRE(subject.GetValueA() == 1);
        REQUIRE(subject.GetValueB() == 3);
        REQUIRE(observerA.GetValue() == 1);
        REQUIRE(lambdaValueB == 3);
        REQUIRE(freeFuncValueB == 2);
    }

    TEST_CASE("Observer - Value Semantics - Benchmarks Object")
    {
        SubjectSystem subject{};
        std::vector<std::shared_ptr<SubjectSystem::Observer>> observers{};
        observers.reserve(creationCount);
        for(uint32_t i{0}; i != creationCount; ++i)
        {
            std::shared_ptr<SubjectSystem::Observer> observer{std::make_unique<SubjectObserverA>()};
            observers.push_back(observer);
            subject.AttachObserver(observer.get());
        }

        BENCHMARK("Benchmark Attach")
        {
            for(std::shared_ptr<SubjectSystem::Observer>& observer : observers)
            {
                subject.AttachObserver(observer.get());
            }
        };

        BENCHMARK("Benchmark Notification")
        {
            subject.SetValueA(0);
        };
    }

    TEST_CASE("Observer - Value Semantics - Benchmarks Lambda")
    {
        int32_t value{0};
        SubjectSystem::Observer observerLambda{
            [&value](const SubjectSystem& subject, const SubjectSystem::Tag tag)
            {
                if(tag == SubjectSystem::Tag::ValueB)
                {
                    value = subject.GetValueB();
                }
            }};

        SubjectSystem subject{};
        std::vector<std::shared_ptr<SubjectSystem::Observer>> observers{};
        observers.reserve(creationCount);
        for(uint32_t i{0}; i != creationCount; ++i)
        {
            std::shared_ptr<SubjectSystem::Observer> observer{std::make_unique<SubjectSystem::Observer>(observerLambda)};
            observers.push_back(observer);
            subject.AttachObserver(observer.get());
        }

        BENCHMARK("Benchmark Attach")
        {
            for(std::shared_ptr<SubjectSystem::Observer>& observer : observers)
            {
                subject.AttachObserver(observer.get());
            }
        };

        BENCHMARK("Benchmark Notification")
        {
            subject.SetValueA(0);
        };
    }

    TEST_CASE("Observer - Value Semantics - Benchmarks Free Function")
    {
        SubjectSystem::Observer observerFreeFunction{OnNotification};
        SubjectSystem subject{};
        std::vector<std::shared_ptr<SubjectSystem::Observer>> observers{};
        observers.reserve(creationCount);
        for(uint32_t i{0}; i != creationCount; ++i)
        {
            std::shared_ptr<SubjectSystem::Observer> observer{std::make_unique<SubjectSystem::Observer>(observerFreeFunction)};
            observers.push_back(observer);
            subject.AttachObserver(observer.get());
        }

        BENCHMARK("Benchmark Attach")
        {
            for(std::shared_ptr<SubjectSystem::Observer>& observer : observers)
            {
                subject.AttachObserver(observer.get());
            }
        };

        BENCHMARK("Benchmark Notification")
        {
            subject.SetValueA(0);
        };
    }
}
