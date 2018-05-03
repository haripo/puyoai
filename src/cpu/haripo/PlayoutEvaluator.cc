#include "PlayoutEvaluator.h"

using namespace std;

PlayoutResult PlayoutEvaluator::evaluate(
        CoreField field,
        KumipuyoSeq seq,
        int hands) {
    vector<ThoughtResult> playoutResult;
    vector<RensaResult> playoutRensaResult;

    InteractiveAI ai(0, NULL);
    ai.removeNontokopuyoParameter();
    ai.setUsesRensaHandTree(false);

    for (int k = 0; k < 10; ++k) {
        // initialize ai
        FrameRequest req;
        req.frameId = 1;
        ai.gameWillBegin(req);
        req.frameId = 2;

        //ai.mutableMyPlayerState()->field = problem.myState.field;
        //ai.mutableMyPlayerState()->seq = problem.enemyState.seq;

        ThoughtResult tmpResult;
        CoreField ff = field;

        // 見えてる 2 手 + ランダム
        seq.append(KumipuyoSeqGenerator::generateACPuyo2Sequence());

        RensaResult tmpRensaResult;
        PlainField lastFieldState;

        // search in random queues
        for (int j = 0; j < hands; ++j) {
            int frameId =  2 + j;

            ThoughtResult thoughtResult = ai.thinkPlan(
              frameId,
              ff,
              seq.subsequence(0, 2),
              ai.myPlayerState(),
              ai.enemyPlayerState(),
              PatternThinker::DEFAULT_DEPTH,
              PatternThinker::DEFAULT_NUM_ITERATION);

            if (j == 0) {
                tmpResult = thoughtResult;
            }

            {
                if (thoughtResult.plan.decisions().size() == 0) {
                    continue; // 連鎖構築に失敗？
                }
                ff.dropKumipuyo(thoughtResult.plan.decisions().front(), seq.front());
                PlainField _lastFieldState = ff.toPlainField();
                RensaResult lastRensaResult = ff.simulate();
                if (j == 0
                    || lastRensaResult.chains > tmpRensaResult.chains
                    || (lastRensaResult.chains == tmpRensaResult.chains
                        && lastRensaResult.score > tmpRensaResult.score)) {
                    tmpRensaResult = lastRensaResult;
                    lastFieldState = _lastFieldState;
                }
                //req.playerFrameRequest[0].field = ff.toPlainField();
            }
            seq.dropFront();
        }

        cout << "trial: " << k << endl;
        FieldPrettyPrinter::print(lastFieldState, seq);
        cout << tmpRensaResult << endl;

        playoutResult.push_back(tmpResult);
        playoutRensaResult.push_back(tmpRensaResult);
    }

    return PlayoutResult { playoutResult, playoutRensaResult };
}
