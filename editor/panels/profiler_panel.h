#ifndef CH_PROFILER_PANEL_H
#define CH_PROFILER_PANEL_H

#include "panel.h"

namespace Chained
{
	class ProfilerPanel : public Panel
	{
	public:
		ProfilerPanel();
		virtual void OnImGuiRender(bool readOnly = false) override;

	private:
		void DrawProfileResult(const struct ProfileResult& result);
		void UpdateHistory();

	private:
		std::vector<float> m_FrameTimeHistory;
	};
} // namespace Chained

#endif // CH_PROFILER_PANEL_H
