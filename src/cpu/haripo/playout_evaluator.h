#ifndef HARIPO_PLAYOUT_EVALUATOR_H_
#define HARIPO_PLAYOUT_EVALUATOR_H_

#include <iostream>
#include <utility>

#include "core/core_field.h"
#include "core/field_pretty_printer.h"
#include "core/frame_request.h"
#include "core/kumipuyo.h"
#include "core/kumipuyo_seq_generator.h"

#include "cpu/mayah/evaluator.h"
#include "cpu/mayah/pattern_thinker.h"
#include "cpu/mayah/mayah_ai.h"

class InteractiveAI : public DebuggableMayahAI {
public:
    InteractiveAI(int argc, char* argv[]) : DebuggableMayahAI(argc, argv) {}
};

struct PlayoutResult {
    int score;
    std::vector<ThoughtResult> thoughts;
    std::vector<RensaResult> chains;
};

class PlayoutEvaluator {
public:
    PlayoutEvaluator();
    ~PlayoutEvaluator();

    std::tuple<RensaResult, PlainField, ThoughtResult> try_once(CoreField field, KumipuyoSeq seq, int hands);
    PlayoutResult evaluate(CoreField field, KumipuyoSeq seq, int hands);

private:
    InteractiveAI *ai;
};

#endif
