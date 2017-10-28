#ifndef CAPTURE_CAPTURE_H_
#define CAPTURE_CAPTURE_H_

#include <memory>
#include <mutex>
#include <thread>
#include <iostream>

#include "base/base.h"
#include "capture/analyzer.h"
#include "capture/analyzer_result_drawer.h"
#include "gui/drawer.h"
#include "gui/unique_sdl_surface.h"

class Analyzer;
class Source;
class Screen;

class Capture : public Drawer, public AnalyzerResultRetriever {
public:
    // Does not take the ownership of |source| and |analyzer|.
    // They should be alive during Capture is alive.
    explicit Capture(Source* source, Analyzer* analyzer);
    virtual ~Capture() {}

    bool start();
    void stop();

    virtual void draw(Screen*) override;

    virtual std::unique_ptr<AnalyzerResult> analyzerResult() const override;

protected:
    virtual void runLoop();

    Source* source_;
    Analyzer* analyzer_;

    std::thread th_;
    volatile bool shouldStop_;

    mutable std::mutex mu_;
    UniqueSDLSurface surface_;
    std::deque<std::unique_ptr<AnalyzerResult>> results_;
};

#endif  // CAPTURE_CAPTURE_H_
