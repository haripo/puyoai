#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <tuple>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "base/strings.h"
#include "base/time.h"
#include "core/plan/plan.h"
#include "core/rensa/rensa_detector.h"
#include "core/core_field.h"
#include "core/field_pretty_printer.h"
#include "core/frame_request.h"
#include "core/kumipuyo.h"
#include "core/kumipuyo_seq_generator.h"
#include "core/probability/puyo_set_probability.h"
#include "solver/problem.h"

#include "cpu/mayah/evaluator.h"
#include "cpu/mayah/pattern_thinker.h"
#include "cpu/mayah/mayah_ai.h"

#include "playout_evaluator.h"

DEFINE_string(problem, "", "use problem");
DEFINE_bool(tokopuyo, false, "Use tokopuyo parameter");

using namespace std;

Problem makeProblem()
{
    KumipuyoSeq generated = KumipuyoSeqGenerator::generateACPuyo2Sequence();
    LOG(INFO) << "seq=" << generated.toString();

    Problem problem;
    if (!FLAGS_problem.empty()) {
        problem = Problem::readProblem(FLAGS_problem);

        // Add generated sequence after problem.
        problem.myState.seq.append(generated);
        problem.enemyState.seq.append(generated);
    } else {
        problem.myState.seq = generated;
        problem.enemyState.field = CoreField(
            "5...65"
            "4...66"
            "545645"
            "456455"
            "545646"
            "545646"
            "564564"
            "456456"
            "456456"
            "456456");
        problem.enemyState.seq = KumipuyoSeq("666666");
        problem.enemyState.seq.append(generated);
    }

    return problem;
}

int main(int argc, char* argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
#if !defined(_MSC_VER)
    google::InstallFailureSignalHandler();
#endif

    Problem problem = makeProblem();
    PlayoutEvaluator playout;

    FrameRequest req;
    req.playerFrameRequest[0].field = problem.myState.field.toPlainField();
    req.playerFrameRequest[0].kumipuyoSeq = problem.myState.seq;

    for (int i = 0; i < 50; ++i) {
        FieldPrettyPrinter::print(
          req.playerFrameRequest[0].field,
          req.playerFrameRequest[0].kumipuyoSeq);

        cout << "*********************" << endl;

        std::vector<Plan> plans;
        std::vector<PlayoutResult> playoutResults;

        auto callback = [&](const RefPlan& plan) {
            KumipuyoSeq seq = req.playerFrameRequest[0].kumipuyoSeq.subsequence(1, 3);
            PlayoutResult playoutResult = playout.evaluate(plan.field(), seq, 50 - i);

            // TODO: 50-i のように固定値を指定するのではなく、本線発火タイミングを検知する

            plans.push_back(plan.toPlan());
            playoutResults.push_back(playoutResult);

            cout << "score: " << playoutResult.score << endl;
            FieldPrettyPrinter::print(plan.field().toPlainField(), seq);
        };

        CoreField field = CoreField(req.playerFrameRequest[0].field);
        KumipuyoSeq seq = req.playerFrameRequest[0].kumipuyoSeq.subsequence(0, 2);
        Plan::iterateAvailablePlans(field, seq, 1, callback);

        // スコアの中央値の高いものを選択
        int maxResult = 0;
        for (int j = 0; j < (int)playoutResults.size(); ++j) {
            if (playoutResults[j].score > playoutResults[maxResult].score) {
                maxResult = j;
            }
        }

        // playout 結果を反映
        {
            Plan plan = plans[maxResult];
            CoreField f(req.playerFrameRequest[0].field);
            f.dropKumipuyo(plan.decisions().front(), req.playerFrameRequest[0].kumipuyoSeq.front());
            RensaResult result = f.simulate();
            req.playerFrameRequest[0].field = f.toPlainField();
        }
        req.playerFrameRequest[0].kumipuyoSeq.dropFront();

    }

    return 0;
}
