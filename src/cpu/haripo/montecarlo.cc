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

#include "PlayoutEvaluator.h"

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

        CoreField ff = CoreField(req.playerFrameRequest[0].field);
        KumipuyoSeq seq = req.playerFrameRequest[0].kumipuyoSeq.subsequence(0, 2);
        PlayoutResult playoutResult = playout.evaluate(ff, seq, 50); // frames

        // スコアの中央値の高いものを選択
        int maxResult = 0;
        for (int l = 0; l < (int)playoutResult.thoughts.size(); ++l) {
            if (playoutResult.chains[l].score > playoutResult.chains[maxResult].score) {
                maxResult = l;
            }
        }

        cout << playoutResult.chains[maxResult] << endl;

        // playout 結果を反映
        {
            ThoughtResult r = playoutResult.thoughts[maxResult];
            CoreField f(req.playerFrameRequest[0].field);
            cout << "playoutResult" << r.plan.decisions().size() << endl;
            f.dropKumipuyo(r.plan.decisions().front(), req.playerFrameRequest[0].kumipuyoSeq.front());
            RensaResult result = f.simulate();
            req.playerFrameRequest[0].field = f.toPlainField();
        }
        req.playerFrameRequest[0].kumipuyoSeq.dropFront();

    }

    return 0;
}
