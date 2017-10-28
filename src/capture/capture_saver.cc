#include "capture/capture_saver.h"

#include "capture/source.h"
#include "gui/screen.h"
#include "gui/SDL_prims.h"

using namespace std;

CaptureSaver::CaptureSaver(Source* source, Analyzer* analyzer, int target) :
  Capture(source, analyzer)
{
    this->target_player = target;
}

void CaptureSaver::runLoop()
{
    int frameId = 0;
    UniqueSDLSurface prevSurface(emptyUniqueSDLSurface());
    UniqueSDLSurface prev2Surface(emptyUniqueSDLSurface());
    UniqueSDLSurface prev3Surface(emptyUniqueSDLSurface());

    while (!shouldStop_) {
        UniqueSDLSurface surface(source_->nextFrame());
        if (!surface.get())
            continue;

        // We set frameId to surface's userdata. This will be useful for saving screen shot.
        surface->userdata = reinterpret_cast<void*>(static_cast<uintptr_t>(++frameId));

        lock_guard<mutex> lock(mu_);
        unique_ptr<AnalyzerResult> r =
            analyzer_->analyze(surface.get(), prevSurface.get(), prev2Surface.get(), prev3Surface.get(), results_);

        const PlayerAnalyzerResult* pr = r->playerResult(target_player);
        if (pr) {
          cout << pr->toLine() << endl;
        }

        prev3Surface = move(prev2Surface);
        prev2Surface = move(prevSurface);
        prevSurface = move(surface_);
        surface_ = move(surface);
        results_.push_front(move(r));
        while (results_.size() > 10)
            results_.pop_back();
    }
}
