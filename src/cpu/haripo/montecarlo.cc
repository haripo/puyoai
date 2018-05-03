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

DEFINE_string(problem, "", "use problem");
DEFINE_bool(tokopuyo, false, "Use tokopuyo parameter");

using namespace std;

class InteractiveAI : public DebuggableMayahAI {
public:
    InteractiveAI(int argc, char* argv[]) : DebuggableMayahAI(argc, argv) {}
};

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

    InteractiveAI ai(argc, argv);
    ai.removeNontokopuyoParameter();
    ai.setUsesRensaHandTree(false);

    Problem problem = makeProblem();

    FrameRequest req;
    req.frameId = 1;
    ai.gameWillBegin(req);

    req.frameId = 2;
    req.playerFrameRequest[0].field = problem.myState.field.toPlainField();
    req.playerFrameRequest[0].kumipuyoSeq = problem.myState.seq;

    ai.mutableMyPlayerState()->field = problem.myState.field;
    ai.mutableMyPlayerState()->seq = problem.enemyState.seq;

    for (int i = 0; i < 50; ++i) {
        FieldPrettyPrinter::print(
          req.playerFrameRequest[0].field,
          req.playerFrameRequest[0].kumipuyoSeq);

        // playout
        ThoughtResult playoutResult;
        for (int k = 0; k < 1; ++k) {
            ThoughtResult tmpResult;
            CoreField ff = CoreField(req.playerFrameRequest[0].field);
            KumipuyoSeq seq = req.playerFrameRequest[0].kumipuyoSeq;
            RensaResult tmpRensaResult;

            // search in random queues
            for (int j = 0; j < 50; ++j) {
                cout << "*";
                fflush(stdout);

                const int frameId = 2 + i + j;
                req.frameId = frameId;

                ThoughtResult thoughtResult = ai.thinkPlan(
                  frameId,
                  ff,
                  seq.subsequence(0, 2),
                  ai.myPlayerState(),
                  ai.enemyPlayerState(),
                  PatternThinker::FAST_DEPTH,
                  PatternThinker::FAST_NUM_ITERATION);

                if (j == 0) {
                    tmpResult = thoughtResult;
                }

                {
                    ff.dropKumipuyo(thoughtResult.plan.decisions().front(), seq.front());
                    tmpRensaResult = ff.simulate();
                    //req.playerFrameRequest[0].field = ff.toPlainField();
                }
                seq.dropFront();
            }

            if (true) { // compare tmpRensaResult
                playoutResult = tmpResult;
            }
        }
        cout << endl;

        // playout 結果を反映
        {
            CoreField f(req.playerFrameRequest[0].field);
            f.dropKumipuyo(playoutResult.plan.decisions().front(), req.playerFrameRequest[0].kumipuyoSeq.front());
            RensaResult result = f.simulate();
            req.playerFrameRequest[0].field = f.toPlainField();
        }
        req.playerFrameRequest[0].kumipuyoSeq.dropFront();

    }

    return 0;
}
