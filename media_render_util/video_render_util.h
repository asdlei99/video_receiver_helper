#ifndef _VIDEO_RENDER_UTIL_H__
#define _VIDEO_RENDER_UTIL_H__
#include <string>

namespace grt {
	class sender;
	class video_track_receiver;
}
namespace util {
	void async_set_video_renderer(grt::video_track_receiver*, std::shared_ptr<grt::sender> sender, std::string const& id);
	void async_reset_video_renderer(std::shared_ptr<grt::sender> sender, std::string const& id);
	void show_rendering_window(std::shared_ptr<grt::sender> sender);
	void hide_rendering_window(std::shared_ptr<grt::sender> sender);
}



#endif//_VIDEO_RENDER_UTIL_H__