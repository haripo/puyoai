#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <tuple>
#include <memory>

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


struct MonteCarloTreeNode {
    std::vector<std::shared_ptr<MonteCarloTreeNode>> children;
    int score;
    int value;
    int count;
    CoreField field;

    MonteCarloTreeNode() {
        count = 0;
        score = 0;
        value = 0;
    }
};


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

    std::shared_ptr<MonteCarloTreeNode> tree(new MonteCarloTreeNode());

    for (int i = 0; i < 50; ++i) {
        cout << "FINAL" << endl;
        FieldPrettyPrinter::print(
          req.playerFrameRequest[0].field,
          req.playerFrameRequest[0].kumipuyoSeq);

        cout << "*********************" << endl;

        std::vector<Plan> plans;
        //std::vector<PlayoutResult> playoutResults;
        std::vector<int> playoutResults;

        std::shared_ptr<MonteCarloTreeNode> currentNode = tree;

        auto callback = [&](const RefPlan& plan) {
            KumipuyoSeq seq = req.playerFrameRequest[0].kumipuyoSeq.subsequence(1, 3);
            //PlayoutResult playoutResult = playout.evaluate(plan.field(), seq, 50 - i);

            MidEvalResult midEvalResult;
            GazeResult gazeResult;
            PlayerState playerState;

            EvalResult evalResult = playout.ai->pattern_thinker_->eval(
                plan,
                seq,
                3 + i, //int currentFrameId,
                4, // int maxIteration,
                playerState, // const PlayerState& me, お邪魔量、現在のスコアなど
                playerState, // const PlayerState& enemy,
                midEvalResult, // const MidEvalResult& midEvalResult,
                false, //bool fast,
                false, //bool usesRensaHandTree,
                gazeResult //const GazeResult& gazeResult
            );

            plans.push_back(plan.toPlan());

            if (plan.score() > evalResult.maxVirtualScore() && plan.score() > 70000) {
                playoutResults.push_back(evalResult.score() * 100);
            } else {
                playoutResults.push_back(evalResult.score());
            }

            //playoutResults.push_back(plan.score());
            //playoutResults.push_back(playoutResult);

            std::shared_ptr<MonteCarloTreeNode> newNode(new MonteCarloTreeNode());
            newNode->value = evalResult.score();
            newNode->score = plan.score();
            newNode->count += 1;
            currentNode->children.push_back(newNode);

            //cout << "TRIAL score: " << evalResult.score() << endl;
            cout << "TRIAL score: " << plan.score() << ", virtual: " << evalResult.maxVirtualScore() << endl;
            //FieldPrettyPrinter::print(plan.field().toPlainField(), seq);
        };

        CoreField field = CoreField(req.playerFrameRequest[0].field);
        KumipuyoSeq seq = req.playerFrameRequest[0].kumipuyoSeq.subsequence(0, 2);
        Plan::iterateAvailablePlans(field, seq, 1, callback);

        // スコアの中央値の高いものを選択
        int maxResult = 0;
        for (int j = 0; j < (int)playoutResults.size(); ++j) {
            if (playoutResults[j] > playoutResults[maxResult]) {
                maxResult = j;
            }
        }

        currentNode = currentNode->children.at(maxResult);

        // playout 結果を反映
        {
            Plan plan = plans[maxResult];
            CoreField f(req.playerFrameRequest[0].field);
            f.dropKumipuyo(plan.decisions().front(), req.playerFrameRequest[0].kumipuyoSeq.front());
            RensaResult result = f.simulate();
            cout << result << ", " << plan.score() << endl;
            req.playerFrameRequest[0].field = f.toPlainField();
        }
        req.playerFrameRequest[0].kumipuyoSeq.dropFront();

    }

    return 0;
}
