#ifndef CPU_MAYAH_YUKINA_AI_H_
#define CPU_MAYAH_YUKINA_AI_H_

#include <memory>
#include <set>
#include <vector>

#include "mayah_base_ai.h"

class YukinaAI : public MayahBaseAI {
public:
    YukinaAI(int argc, char* argv[]);

    DropDecision think(int frameId, const CoreField&, const KumipuyoSeq&,
                       const PlayerState& me, const PlayerState& enemy, bool fast) const override;

    DropDecision thinkByThinker(int frameId, const CoreField&, const KumipuyoSeq&,
                                const PlayerState& me, const PlayerState& enemy, bool fast) const;

private:
    std::string gazeMessage(int frame_id, const PlayerState& me, const PlayerState& enemy, const GazeResult& gazeResult) const;

    mutable std::mutex mu_; // for cout
};


class DebuggableYukinaAI : public YukinaAI {
public:
    DebuggableYukinaAI() : YukinaAI(0, nullptr) {}
    virtual ~DebuggableYukinaAI() {}

    using YukinaAI::saveEvaluationParameter;
    using YukinaAI::loadEvaluationParameter;

    using YukinaAI::gameWillBegin;
    using YukinaAI::gameHasEnded;
    using YukinaAI::next2AppearedForEnemy;
    using YukinaAI::decisionRequestedForEnemy;
    using YukinaAI::groundedForEnemy;

    using YukinaAI::myPlayerState;
    using YukinaAI::enemyPlayerState;

    using YukinaAI::mutableMyPlayerState;
    using YukinaAI::mutableEnemyPlayerState;

//    void setUsesRensaHandTree(bool flag) { usesRensaHandTree_ = flag; }

    void removeNontokopuyoParameter() { evaluationParameterMap_.removeNontokopuyoParameter(); }

    const EvaluationParameterMap& evaluationParameterMap() const { return evaluationParameterMap_; }
    void setEvaluationParameterMap(const EvaluationParameterMap& map) { evaluationParameterMap_.loadValue(map.toTomlValue()); };
};


#endif // CPU_MAYAH_YUKINA_AI_H_
