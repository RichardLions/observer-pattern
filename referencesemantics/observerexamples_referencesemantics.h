#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <set>
#include <memory>

namespace ReferenceSemantics
{
    template<typename T>
    concept IsScopedEnum = std::is_scoped_enum_v<T>;

    template<typename SubjectT, IsScopedEnum TagT>
    class Observer
    {
    public:
        virtual ~Observer() = default;
        virtual void OnNotification(const SubjectT& subject, const TagT tag) = 0;
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
        void OnNotification(const SubjectSystem& subject, const SubjectSystem::Tag tag) override
        {
            if(tag == SubjectSystem::Tag::ValueA)
            {
                m_Value = subject.GetValueA();
            }
        }

        int32_t GetValue() const { return m_Value; }
    private:
        int32_t m_Value{0};
    };

    class SubjectObserverB final : public SubjectSystem::Observer
    {
    public:
        void OnNotification(const SubjectSystem& subject, const SubjectSystem::Tag tag) override
        {
            if(tag == SubjectSystem::Tag::ValueB)
            {
                m_Value = subject.GetValueB();
            }
        }

        int32_t GetValue() const { return m_Value; }
    private:
        int32_t m_Value{0};
    };

    TEST_CASE("Observer - Reference Semantics - Unit Tests")
    {
        SubjectSystem subject{};
        SubjectObserverA observerA{};
        SubjectObserverB observerB{};
        SubjectObserverB observerBB{};
        REQUIRE(subject.GetValueA() == 0);
        REQUIRE(subject.GetValueB() == 0);
        REQUIRE(observerA.GetValue() == 0);
        REQUIRE(observerB.GetValue() == 0);
        REQUIRE(observerBB.GetValue() == 0);

        subject.AttachObserver(&observerA);
        subject.AttachObserver(&observerB);
        subject.AttachObserver(&observerBB);

        subject.SetValueA(1);

        REQUIRE(subject.GetValueA() == 1);
        REQUIRE(subject.GetValueB() == 0);
        REQUIRE(observerA.GetValue() == 1);
        REQUIRE(observerB.GetValue() == 0);
        REQUIRE(observerBB.GetValue() == 0);

        subject.SetValueB(2);

        REQUIRE(subject.GetValueA() == 1);
        REQUIRE(subject.GetValueB() == 2);
        REQUIRE(observerA.GetValue() == 1);
        REQUIRE(observerB.GetValue() == 2);
        REQUIRE(observerBB.GetValue() == 2);

        subject.DetachObserver(&observerBB);
        subject.SetValueB(3);

        REQUIRE(subject.GetValueA() == 1);
        REQUIRE(subject.GetValueB() == 3);
        REQUIRE(observerA.GetValue() == 1);
        REQUIRE(observerB.GetValue() == 3);
        REQUIRE(observerBB.GetValue() == 2);
    }

    TEST_CASE("Observer - Reference Semantics - Benchmarks")
    {
        constexpr uint32_t creationCount{250'000};
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
}
