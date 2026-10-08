#pragma once

namespace Starline::GUI {

void InitializeExternalEsp();
void RenderExternalEsp();
void RenderPlutoWatermark(float fps, float cpu_usage, float working_set_mib);

} // namespace Starline::GUI
