#include "playout_evaluator.h"

PlayoutEvaluator::PlayoutEvaluator() {
    // initialize ai
    ai = new InteractiveAI(0, NULL);
    FrameRequest req;
    ai->removeNontokopuyoParameter();
    ai->setUsesRensaHandTree(false);
    req.frameId = 1;
    ai->gameWillBegin(req);
    req.frameId = 2;
}

PlayoutEvaluator::~PlayoutEvaluator() {
    delete ai;
}

std::tuple<RensaResult, PlainField, ThoughtResult> PlayoutEvaluator::try_once(
        CoreField field,
        KumipuyoSeq seq,
        int hands) {

    ThoughtResult thoughtResult;
    RensaResult rensaResult;
    PlainField fieldResult;

    // ランダムにツモを作る
    seq.append(KumipuyoSeqGenerator::generateACPuyo2Sequence());

    // search in random queues
    for (int j = 0; j < hands; ++j) {
        int frameId =  2 + j;

        ThoughtResult tmpThoughtResult = ai->thinkPlan(
            frameId,
            field,
            seq.subsequence(0, 2),
            ai->myPlayerState(),
            ai->enemyPlayerState(),
            PatternThinker::DEFAULT_DEPTH,
            PatternThinker::DEFAULT_NUM_ITERATION);

        if (tmpThoughtResult.plan.decisions().size() == 0) {
            continue; // 連鎖構築に失敗？
        }

        // いずれいらなくなる
        if (j == 0) {
            thoughtResult = tmpThoughtResult;
        }

        field.dropKumipuyo(tmpThoughtResult.plan.decisions().front(), seq.front());
        PlainField tmpField = field.toPlainField();
        RensaResult tmpRensaResult = field.simulate();
        if (j == 0 || tmpRensaResult.score > rensaResult.score) {
            rensaResult = tmpRensaResult;
            fieldResult = tmpField;
        }
        seq.dropFront();
    }

    return std::make_tuple(rensaResult, fieldResult, thoughtResult);
}

PlayoutResult PlayoutEvaluator::evaluate(
        CoreField field,
        KumipuyoSeq seq,
        int hands) {
    std::vector<int> scores;
    // for debug
    std::vector<ThoughtResult> thoughts;
    std::vector<RensaResult> chains;

    for (int k = 0; k < 30; ++k) {
        std::tuple<RensaResult, PlainField, ThoughtResult> result = try_once(field, seq, hands);
        RensaResult rensaResult = std::get<0>(result);
        PlainField fieldResult = std::get<1>(result);
        ThoughtResult thoughtdResult = std::get<2>(result);

        std::cout << "trial " << k << " : " << rensaResult << std::endl;

        thoughts.push_back(thoughtdResult);
        chains.push_back(rensaResult);
        scores.push_back(rensaResult.score);
    }

    // compute median
    sort(scores.begin(), scores.end());
    int score = 0;
    if (scores.size() == 0) {
        std::cerr << "Can't produce any chains" << std::endl;
    } else {
        score = scores[(scores.size() - 1) / 2];
    }

    return PlayoutResult { score, thoughts, chains };
}
