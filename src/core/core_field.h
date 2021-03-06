#ifndef CORE_CORE_FIELD_H_
#define CORE_CORE_FIELD_H_

#include <glog/logging.h>

#include <algorithm>
#include <initializer_list>
#include <ostream>
#include <string>
#include <vector>

#include "base/base.h"
#include "core/bit_field.h"
#include "core/column_puyo_list.h"
#include "core/decision.h"
#include "core/field_checker.h"
#include "core/field_constant.h"
#include "core/frame.h"
#include "core/kumipuyo_pos.h"
#include "core/puyo_color.h"
#include "core/plain_field.h"
#include "core/rensa_result.h"
#include "core/score.h"

class ColumnPuyoList;
class Kumipuyo;
struct Position;

// CoreField represents a field. Without strong reason, this class should be used for
// field implementation.
class CoreField : public FieldConstant {
public:
    CoreField() : heights_{} {}
    explicit CoreField(const std::string& url);
    explicit CoreField(const PlainField&);
    explicit CoreField(const BitField&);
    CoreField(const CoreField&) = default;

    static CoreField fromPlainFieldWithDrop(const PlainField& pf) {
        PlainField copied(pf);
        copied.drop();
        return CoreField(copied);
    }

    // Gets a color of puyo at a specified position.
    PuyoColor color(int x, int y) const { return field_.color(x, y); }

    // Returns true if puyo on (x, y) is |c|.
    bool isColor(int x, int y, PuyoColor c) const { return field_.isColor(x, y, c); }
    // Returns true if puyo on (x, y) is empty.
    bool isEmpty(int x, int y) const { return field_.isEmpty(x, y); }
    // Returns true if puyo on (x, y) is a normal color.
    bool isNormalColor(int x, int y) const { return field_.isNormalColor(x, y); }

    // Returns the height of the specified column.
    int height(int x) const { return heights_[x]; }

    const BitField& bitField() const { return field_; }

    PlainField toPlainField() const;

    // ----------------------------------------------------------------------
    // field utilities

    // Returns true if the field does not have any puyo. Valid only all puyos are dropped.
    bool isZenkeshi() const;

    // Counts the number of color puyos.
    int countColorPuyos() const;
    // Counts the all puyos (including ojama).
    int countPuyos() const;
    // Counts the specified color.
    int countColor(PuyoColor) const;

    // Returns the number of puyos connected to (x, y).
    // Actually you can use this if color(x, y) is EMPTY or OJAMA.
    int countConnectedPuyos(int x, int y) const { return field_.countConnectedPuyos(x, y); }
    // Same as countConnectedPuyos(x, y), but with checking using |checked|.
    int countConnectedPuyos(int x, int y, FieldBits* checked) const { return field_.countConnectedPuyos(x, y, checked); }
    // Same as countConnectedPuyos(x, y).
    // If # of connected puyos is >= 4, the result is any value >= 4.
    // For example, if the actual number of connected is 6, result is 4, 5, or 6.
    // This is faster than countConnectedPuyos, so this will be useful when checking
    // puyo is vanished or not.
    int countConnectedPuyosMax4(int x, int y) const { return field_.countConnectedPuyosMax4(x, y); }
    int countConnectedPuyosMax4(int x, int y, PuyoColor c) const { return field_.countConnectedPuyosMax4(x, y, c); }
    // Returns true if color(x, y) is connected in some direction.
    bool isConnectedPuyo(int x, int y) const { return field_.isConnectedPuyo(x, y, color(x, y)); }
    bool isConnectedPuyo(int x, int y, PuyoColor c) const { return field_.isConnectedPuyo(x, y, c); }
    // Returns true if there is an empty neighbor of (x, y).
    bool hasEmptyNeighbor(int x, int y) const { return field_.hasEmptyNeighbor(x, y); }
    // Returns the number of empty unreachable spaces.
    int countUnreachableSpaces() const;
    // Returns the number of reachable spaces.
    int countReachableSpaces() const;

    void countConnection(int* count2, int* count3) const { return field_.countConnection(count2, count3); }
    // Returns the ridge height of column |x|.
    int ridgeHeight(int x) const;
    // Returns the vallye depth of column |x|.
    int valleyDepth(int x) const;

    // ----------------------------------------------------------------------
    // field manipulation

    // Drop kumipuyo with decision.
    bool dropKumipuyo(const Decision&, const Kumipuyo&);

    // Returns #frame to drop the next KumiPuyo with decision. This function does not drop the puyo.
    int framesToDropNext(const Decision&) const;

    // Returns true if |decision| will cause chigiri.
    bool isChigiriDecision(const Decision&) const;

    // Fall ojama puyos |lines| lines.
    // Returns the frame to fall ojama.
    int fallOjama(int lines);

    // Places a puyo on the top of column |x|.
    // Returns true if succeeded. False if failed. When false is returned, field will not change.
    bool dropPuyoOn(int x, PuyoColor pc) { return dropPuyoOnWithMaxHeight(x, pc, 13); }
    bool dropPuyoOnWithMaxHeight(int x, PuyoColor, int maxHeight);

    // Drop all puyos in |cpl|. If failed, false will be returned. In that case, the CoreField
    // might be corrupted, so you cannot use this CoreField.
    bool dropPuyoList(const ColumnPuyoList& cpl) { return dropPuyoListWithMaxHeight(cpl, 13); }
    bool dropPuyoListWithMaxHeight(const ColumnPuyoList&, int maxHeight);

    // Removes the puyo from top of column |x|.
    // If there is no puyo on column |x|, behavior is undefined.
    void removePuyoFrom(int x);

    // ----------------------------------------------------------------------
    // simulation

    // SimulationContext can be used when we continue simulation from the intermediate points.
    typedef BitField::SimulationContext SimulationContext;

    // Inserts positions whose puyo color is the same as |c|, and connected to (x, y).
    // The checked cells will be marked in |checked|.
    // PositionQueueHead should have enough capacity.
    Position* fillSameColorPosition(int x, int y, PuyoColor c, Position* positionQueueHead, FieldBits* checked) const
    {
        return field_.fillSameColorPosition(x, y, c, positionQueueHead, checked);
    }

    // Fills the positions where puyo is vanished in the 1-rensa.
    // Returns the length of the filled positions. The max length should be 72.
    // So, |Position*| must have 72 Position spaces.
    int fillErasingPuyoPositions(Position*) const;
    // TODO(mayah): Remove this.
    std::vector<Position> erasingPuyoPositions() const;
    FieldBits ignitionPuyoBits() const;

    // TODO(mayah): Remove this.
    bool rensaWillOccurWhenLastDecisionIs(const Decision&) const;
    bool rensaWillOccur() const { return field_.rensaWillOccur(); }

    // Simulates chains. Returns RensaResult.
    RensaResult simulate(int initialChain = 1);
    // Simulates chains with SimulationContext.
    RensaResult simulate(SimulationContext*);
    // Simulates chains with Tracker. Tracker can track various rensa information.
    // Several trackers are defined in core/rensa_trackers.h. You can define your own Tracker.
    template<typename Tracker> RensaResult simulate(Tracker*);
    // Simualtes chains with SimulationContext and Tracker.
    template<typename Tracker> RensaResult simulate(SimulationContext*, Tracker*);

    int simulateFast();
    template<typename Tracker> int simulateFast(Tracker*);

    // Vanishes the connected puyos, and drop the puyos in the air. Score will be returned.
    RensaStepResult vanishDrop();
    RensaStepResult vanishDrop(SimulationContext*);
    // Vanishes the connected puyos with Tracker.
    template<typename Tracker> RensaStepResult vanishDrop(Tracker*);
    template<typename Tracker> RensaStepResult vanishDrop(SimulationContext*, Tracker*);

    // Vanishes the connected puyos, and drop the puyos in the air.
    // Returns true if something is vanished.
    bool vanishDropFast();
    bool vanishDropFast(SimulationContext*);
    template<typename Tracker> bool vanishDropFast(Tracker*);
    template<typename Tracker> bool vanishDropFast(SimulationContext*, Tracker*);

    // ----------------------------------------------------------------------
    // utility methods

    size_t hash() const { return field_.hash(); }

    std::string toDebugString() const;

    friend bool operator==(const CoreField&, const CoreField&);
    friend bool operator!=(const CoreField&, const CoreField&);
    friend std::ostream& operator<<(std::ostream& os, const CoreField& cf) { return (os << cf.toDebugString()); }

public:
    // --- These methods should be carefully used.

    // TODO(mayah): Remove this.
    void setPuyoAndHeight(int x, int y, PuyoColor c)
    {
        unsafeSet(x, y, c);

        // Recalculate height.
        heights_[x] = 0;
        for (int y = 1; y <= 13; ++y) {
            if (color(x, y) != PuyoColor::EMPTY)
                heights_[x] = y;
        }
    }

private:
    void unsafeSet(int x, int y, PuyoColor c) { field_.setColor(x, y, c); }

    BitField field_;
    alignas(16) int heights_[MAP_WIDTH];
};

inline
CoreField::CoreField(const BitField& f) :
    field_(f)
{
    f.calculateHeight(heights_);
}

inline
RensaResult CoreField::simulate(int initialChain)
{
    SimulationContext context(initialChain);
    RensaNonTracker tracker;
    return simulate(&context, &tracker);
}

inline
RensaResult CoreField::simulate(SimulationContext* context)
{
    RensaNonTracker tracker;
    return simulate(context, &tracker);
}

template<typename Tracker>
RensaResult CoreField::simulate(Tracker* tracker)
{
    SimulationContext context;
    return simulate(&context, tracker);
}

template<typename Tracker>
RensaResult CoreField::simulate(SimulationContext* context, Tracker* tracker)
{
#if defined(__AVX2__) && defined(__BMI2__)
    RensaResult result = field_.simulateAVX2(context, tracker);
#else
    RensaResult result = field_.simulate(context, tracker);
#endif

    field_.calculateHeight(heights_);
    return result;
}

inline
int CoreField::simulateFast()
{
    RensaNonTracker tracker;
    return simulateFast(&tracker);
}

template<typename Tracker>
int CoreField::simulateFast(Tracker* tracker)
{
#if defined(__AVX2__) && defined(__BMI2__)
    int result = field_.simulateFastAVX2(tracker);
#else
    int result = field_.simulateFast(tracker);
#endif

    field_.calculateHeight(heights_);
    return result;
}

inline
RensaStepResult CoreField::vanishDrop()
{
    RensaNonTracker tracker;
    return vanishDrop(&tracker);
}

inline
RensaStepResult CoreField::vanishDrop(SimulationContext* context)
{
    RensaNonTracker tracker;
    return vanishDrop(context, &tracker);
}

template<typename Tracker>
RensaStepResult CoreField::vanishDrop(Tracker* tracker)
{
    SimulationContext context;
    return vanishDrop(&context, tracker);
}

template<typename Tracker>
RensaStepResult CoreField::vanishDrop(SimulationContext* context, Tracker* tracker)
{
#if defined(__AVX2__) && defined(__BMI2__)
    RensaStepResult result = field_.vanishDropAVX2(context, tracker);
#else
    RensaStepResult result = field_.vanishDrop(context, tracker);
#endif

    field_.calculateHeight(heights_);
    return result;
}

inline
bool CoreField::vanishDropFast()
{
    RensaNonTracker tracker;
    return vanishDropFast(&tracker);
}

inline
bool CoreField::vanishDropFast(CoreField::SimulationContext* context)
{
    RensaNonTracker tracker;
    return vanishDropFast(context, &tracker);
}

template<typename Tracker>
bool CoreField::vanishDropFast(Tracker* tracker)
{
    CoreField::SimulationContext context;
    return vanishDropFast(&context, tracker);
}

template<typename Tracker>
bool CoreField::vanishDropFast(SimulationContext* context, Tracker* tracker)
{
#if defined(__AVX2__) && defined(__BMI2__)
    bool result = field_.vanishDropFastAVX2(context, tracker);
#else
    bool result = field_.vanishDropFast(context, tracker);
#endif

    field_.calculateHeight(heights_);
    return result;
}

inline
void CoreField::removePuyoFrom(int x)
{
    DCHECK_GE(height(x), 1);
    unsafeSet(x, heights_[x]--, PuyoColor::EMPTY);
}

namespace std {

template<>
struct hash<CoreField>
{
    size_t operator()(const CoreField& cf) const
    {
        return cf.hash();
    }
};

}

#endif
