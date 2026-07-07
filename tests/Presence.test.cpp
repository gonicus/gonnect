#include "Presence.test.h"
#include "PresenceStateAggregator.h"
#include "IPresenceStateProvider.h"
#include "PresenceState.h"

#include <QTest>
#include <QSignalSpy>

using State = PresenceState::State;

namespace {

/*! Minimal IPresenceStateProvider that exposes the protected setters so tests can drive the
 *  aggregated state directly. */
class MockPresenceStateProvider : public IPresenceStateProvider
{
public:
    using IPresenceStateProvider::IPresenceStateProvider;

    void setState(State state) { setPresenceState(state); }
    void setText(const QString &text) { setStateText(text); }
};

/*! Split the aggregated state text into its parts, ignoring ordering (providers live in a QSet,
 *  so the join order is not deterministic). */
QStringList textParts(const QString &joined)
{
    if (joined.isEmpty()) {
        return {};
    }
    auto parts = joined.split("; ");
    parts.sort();
    return parts;
}

} // namespace

PresenceTest::PresenceTest(QObject *parent) : QObject{ parent } { }

void PresenceTest::testEmptyAggregatorIsUnknown()
{
    PresenceStateAggregator aggregator;
    QCOMPARE(aggregator.presenceState(), State::Unknown);
    QCOMPARE(aggregator.stateText(), QString(""));
}

void PresenceTest::testSingleProviderState()
{
    PresenceStateAggregator aggregator;
    MockPresenceStateProvider provider;
    provider.setState(State::Available);

    aggregator.registerStateProvider(&provider);

    QCOMPARE(aggregator.presenceState(), State::Available);
}

void PresenceTest::testPriorityAggregation()
{
    PresenceStateAggregator aggregator;

    MockPresenceStateProvider available;
    available.setState(State::Available);
    MockPresenceStateProvider busy;
    busy.setState(State::Busy);

    aggregator.registerStateProvider(&available);
    aggregator.registerStateProvider(&busy);

    // Busy outranks Available
    QCOMPARE(aggregator.presenceState(), State::Busy);

    // Offline outranks Busy in the aggregator's priority map (independent of the enum order)
    MockPresenceStateProvider offline;
    offline.setState(State::Offline);
    aggregator.registerStateProvider(&offline);

    QCOMPARE(aggregator.presenceState(), State::Offline);
}

void PresenceTest::testRingingWinsOverEverything()
{
    PresenceStateAggregator aggregator;

    MockPresenceStateProvider offline;
    offline.setState(State::Offline);
    MockPresenceStateProvider ringing;
    ringing.setState(State::Ringing);

    aggregator.registerStateProvider(&offline);
    aggregator.registerStateProvider(&ringing);

    QCOMPARE(aggregator.presenceState(), State::Ringing);
}

void PresenceTest::testStateTextJoin()
{
    PresenceStateAggregator aggregator;

    MockPresenceStateProvider first;
    first.setText("In a meeting");
    MockPresenceStateProvider second;
    second.setText("Back at 3pm");

    aggregator.registerStateProvider(&first);
    aggregator.registerStateProvider(&second);

    QCOMPARE(textParts(aggregator.stateText()),
             (QStringList{ "Back at 3pm", "In a meeting" }));
}

void PresenceTest::testStateTextSkipsEmpty()
{
    PresenceStateAggregator aggregator;

    MockPresenceStateProvider withText;
    withText.setText("Away");
    MockPresenceStateProvider withoutText; // empty text must not contribute an empty segment

    aggregator.registerStateProvider(&withText);
    aggregator.registerStateProvider(&withoutText);

    QCOMPARE(aggregator.stateText(), QString("Away"));
}

void PresenceTest::testStateChangeRecomputes()
{
    PresenceStateAggregator aggregator;
    MockPresenceStateProvider provider;
    provider.setState(State::Available);
    aggregator.registerStateProvider(&provider);
    QCOMPARE(aggregator.presenceState(), State::Available);

    QSignalSpy spy(&aggregator, &PresenceStateAggregator::presenceStateChanged);

    // Changing a registered provider must propagate to the aggregate
    provider.setState(State::Busy);

    QCOMPARE(aggregator.presenceState(), State::Busy);
    QCOMPARE(spy.count(), 1);
}

void PresenceTest::testProviderDestroyedIsRemoved()
{
    PresenceStateAggregator aggregator;
    MockPresenceStateProvider stayingAvailable;
    stayingAvailable.setState(State::Available);
    aggregator.registerStateProvider(&stayingAvailable);

    {
        MockPresenceStateProvider temporaryBusy;
        temporaryBusy.setState(State::Busy);
        aggregator.registerStateProvider(&temporaryBusy);
        QCOMPARE(aggregator.presenceState(), State::Busy);
    }
    // temporaryBusy is destroyed here — the aggregator must drop it and recompute
    QCOMPARE(aggregator.presenceState(), State::Available);
}

void PresenceTest::testNullptrRegistrationIgnored()
{
    PresenceStateAggregator aggregator;
    aggregator.registerStateProvider(nullptr); // must not crash
    QCOMPARE(aggregator.presenceState(), State::Unknown);
}

void PresenceTest::testDuplicateRegistrationIgnored()
{
    PresenceStateAggregator aggregator;
    MockPresenceStateProvider provider;
    provider.setText("Once");

    aggregator.registerStateProvider(&provider);
    aggregator.registerStateProvider(&provider); // second registration is ignored

    // Text must appear only once, not "Once; Once"
    QCOMPARE(aggregator.stateText(), QString("Once"));
}

QTEST_GUILESS_MAIN(PresenceTest)
