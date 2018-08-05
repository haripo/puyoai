#include "mayah_ai.h"
#include "yukina_ai.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <future>
#include <sstream>
#include <random>
#include <mutex>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "base/executor.h"
#include "core/kumipuyo_seq_generator.h"
#include "core/probability/puyo_set_probability.h"
#include "solver/endless.h"
#include "solver/puyop.h"

#include "evaluation_parameter.h"

DECLARE_string(feature);
DECLARE_string(seq);
DECLARE_int32(seed);

DEFINE_bool(show_field, false, "show field after each hand.");
DEFINE_int32(size, 10000000, "the number of case size.");
DEFINE_int32(offset, 0, "offset for random seed");

using namespace std;

struct Result {
    EndlessResult result;
    string msg;
};

inline const char* toString(EndlessResult::Type v)
{
    switch (v)
    {
        case EndlessResult::Type::DEAD:   return "dead";
        case EndlessResult::Type::MAIN_CHAIN:   return "mainchain";
        case EndlessResult::Type::ZENKESHI: return "allclear";
        case EndlessResult::Type::PUYOSEQ_RUNOUT: return "runout";
    }
}


mutex mtx;

void run(Executor* executor, const EvaluationParameterMap& paramMap)
{
    const int N = FLAGS_size;
    vector<promise<Result>> ps(N);

    for (int i = 0; i < N; ++i) {
        auto f = [i, &paramMap, &ps]() {
            LOG(ERROR) << "STARTED: " << i;

            auto ai = new DebuggableMayahAI;

            //ai->setUsesRensaHandTree(false);
            ai->setEvaluationParameterMap(paramMap);

            std::unique_ptr<AI> ai_ptr(ai);
            Endless endless(std::move(ai_ptr));

            stringstream ss;
            KumipuyoSeq seq = KumipuyoSeqGenerator::generateACPuyo2SequenceWithSeed(i + FLAGS_offset);
            EndlessResult result = endless.run(seq);
            string history = "";

            for (auto it = result.decisions.begin(); it != result.decisions.end(); ++it) {
                history += std::to_string(it->axisX())　+ std::to_string(it->rot());
            }

            ss << "case, "
                << setw(2) << i << ", "
                << setw(6) << result.score << ", "
                << setw(2) << result.maxRensa << ", "
                << setw(8) << toString(result.type) << ", "
                << seq.toString() << ", "
                << history << ", "
                << makePuyopURL(seq, result.decisions) << endl;

            int j = 0;
            CoreField ff;
            for (auto it = result.decisions.begin(); it != result.decisions.end(); ++it) {
                ff.dropKumipuyo(*it, seq.get(j));
                ff.simulate();
                ss << "field, "
                    << setw(2) << i << ", "
                    << setw(2) << j++ << ", "
                    << it->axisX() << ", "
                    << it->rot() << ", "
                    << ff.toPlainField().toString('_') << endl;
            }

            {
                lock_guard<mutex> lock(mtx);
                cout << ss.str() << endl;
            }

            LOG(ERROR) << "FINISHED: " << i;
        };

        executor->submit(f);
    }
}

int main(int argc, char* argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
#if !defined(_MSC_VER)
    google::InstallFailureSignalHandler();
#endif

    unique_ptr<Executor> executor = Executor::makeDefaultExecutor();

    EvaluationParameterMap paramMap;
    if (!paramMap.load(FLAGS_feature)) {
        std::string filename = string(SRC_DIR) + "/cpu/mayah/" + FLAGS_feature;
        if (!paramMap.load(filename))
            CHECK(false) << "parameter cannot be loaded correctly.";
    }
    paramMap.removeNontokopuyoParameter();

    run(executor.get(), paramMap);

    executor->stop();
    return 0;
}
