#include "video_render_util.h"
#include <cassert>
#include "video_render/renderer.h"
#include <thread>
#include "json_parser/json_parser.h"
#include "rendering_server_client/rendering_server_client.h"
#include "video_receiver/video_track_receiver.h"
#include <Windows.h>

namespace detail {

	class video_receiver : public grt::video_frame_callback {
	private:
		std::shared_ptr<grt::renderer> renderer_;
		HWND hwnd_;
		const std::string id_;
		std::shared_ptr<grt::sender> sender_;

	public:
		video_receiver(HWND hwnd, std::unique_ptr< grt::renderer>&& render, std::string id, std::shared_ptr<grt::sender> sender)
			:renderer_{ std::move(render) }, hwnd_{ hwnd }, id_{ id }, sender_{ sender }{
			auto r = SetWindowPos(hwnd_, HWND_TOPMOST,
				0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
			assert(r != 0);
			assert(sender_);
			assert(!id_.empty());
		}

		~video_receiver() override {

			const auto m = grt::make_render_wnd_close_req(id_);
		
			std::promise<void> trigger;
			auto future = trigger.get_future();
			sender_->send_to_renderer(id_, m, [&trigger](auto type, auto msg) mutable {
				trigger.set_value();
			});

			const auto status = future.wait_for(std::chrono::seconds(5));//wait for message to deliever.
			assert(status == std::future_status::ready);//todo: log status.
			sender_->done(id_);
		}

		void on_frame(grt::yuv_frame frame) override {
			auto frame_info = grt::make_frame_info(
				frame.y_, frame.u_, frame.v_, frame.stride_y_,
				frame.stride_u_, frame.stride_v_, frame.w_, frame.h_);
			renderer_->render_frame(hwnd_, frame_info);
			grt::clean(frame_info);
		}
	};


	std::unique_ptr< grt::video_frame_callback>
		get_frame_receiver(HWND hwnd, std::unique_ptr< grt::renderer>&& render, std::string id, std::shared_ptr<grt::sender> sender) {
		return std::make_unique< video_receiver>(hwnd, std::move(render), id, sender);
	}

	void _show_rendering_wnd(std::shared_ptr<grt::sender> sender, bool to_show) {
		assert(sender);
		const auto m = grt::make_show_hide_msg(to_show);
		sender->send_to_renderer("show_hide", m, [](auto, auto) {});
		sender->done("show_hide");
		std::this_thread::sleep_for(std::chrono::seconds(2));//wait for message for sent. todo: fix this dependency in sender itself.
	}
}

namespace util {

	HWND find_child_window(const wchar_t* className, const wchar_t* parent_wnd_name,
		const wchar_t* child_wnd_name) {
		auto parentWnd = FindWindow(className, parent_wnd_name);
		assert(parentWnd);
		if (parentWnd == nullptr) return nullptr;
		auto wnd = FindWindowEx(parentWnd, nullptr, className, child_wnd_name);
		assert(wnd);
		return wnd;
	}

	std::wstring to_wstring(std::string v) {
		return std::wstring(v.begin(), v.end());
	}

	HWND find_child_window(std::string className, std::string parent_wnd, std::string child_name) {
		return find_child_window(to_wstring(className).c_str(),
			to_wstring(parent_wnd).c_str(), to_wstring(child_name).c_str());
	}

	bool set_video_renderer(grt::video_track_receiver* receiver, std::string class_name, 
		std::string parent_name, std::string  renderer_id, std::shared_ptr<grt::sender> sender, std::string id) {
		//todo: create connection with display manager and ask for creating a window.
		assert(receiver);

		auto hwnd = find_child_window(class_name, parent_name, renderer_id);
		
		assert(hwnd);//rendering application with window should be running
		if (hwnd == nullptr) return false;
		auto frame_receiver = detail::get_frame_receiver(hwnd, grt::get_renderer(), id, sender);
		receiver->register_callback(std::move(frame_receiver));
		return true;
	}

	void async_set_video_renderer(grt::video_track_receiver* recevier, std::shared_ptr<grt::sender> sender, std::string const& id) {
		const auto m = grt::make_render_wnd_req(id);
		sender->send_to_renderer(id, m, [recevier, sender, id](grt::message_type type, absl::any msg) {
			assert(type == grt::message_type::window_create_res);
			auto wndInfo = absl::any_cast<grt::wnd_create_res>(msg);
			assert(wndInfo.status_);
			const auto className = wndInfo.class_name_;
			const auto wndName = wndInfo.parent_wnd_name_;
			const auto wndId = wndInfo.child_wnd_id_;
		
			set_video_renderer(recevier, className, wndName, wndId, sender, id);
			sender->done(id);
		});
	}

	void async_reset_video_renderer(std::shared_ptr<grt::sender> sender, std::string const& id) {
		const auto m = grt::make_render_wnd_close_req(id);
		sender->send_to_renderer(id, m, [id, sender](auto type, auto msg) {
			assert(type == grt::message_type::wnd_close_req_res);
			sender->done(id); 
		});
	}

	void show_rendering_window(std::shared_ptr<grt::sender> sender) {
		detail::_show_rendering_wnd(sender, true);
	}
	void hide_rendering_window(std::shared_ptr<grt::sender> sender) {
		detail::_show_rendering_wnd(sender, false);
	}

	
}//namespace util