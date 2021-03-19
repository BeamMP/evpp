#include <evpp/event_loop_thread.h>
#include <evpp/httpc/conn_pool.h>
#include <evpp/httpc/response.h>
#include <evpp/httpc/request.h>

#include "../../../examples/winmain-inl.h"

static bool responsed = false;
static void HandleHTTPResponse(const std::shared_ptr<evpp::httpc::Response>& response, evpp::httpc::Request* request) {
    std::cout << "http_code=" << response->http_code() << " [" << response->body().ToString() << "]\n";
    auto* ConnectionHeader = response->FindHeader("Connection");
    if(ConnectionHeader != nullptr){
        std::cout << "HTTP HEADER Connection=" << std::string(ConnectionHeader) << "\n";
    }
    responsed = true;
    assert(request == response->request());
    delete request; // The request MUST BE deleted in EventLoop thread.
}

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    google::SetStderrLogging(google::GLOG_ERROR);

    evpp::EventLoopThread t;
    t.Start(true);
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    std::shared_ptr<evpp::httpc::ConnPool> pool(new evpp::httpc::ConnPool("reqbin.com", 443,true, evpp::Duration(10.0)));
    evpp::httpc::SET_SSL_VERIFY_MODE(SSL_VERIFY_NONE);
#else
    std::shared_ptr<evpp::httpc::ConnPool> pool(new evpp::httpc::ConnPool("www.360.cn", 80, evpp::Duration(2.0)));
#endif
    auto* r = new evpp::httpc::Request(pool.get(), t.loop(), "/echo/get/json", "");
    std::cout << "Do http request\n";
    r->Execute([&r](auto && PH1) { return HandleHTTPResponse(std::forward<decltype(PH1)>(PH1), r); });

    while (!responsed) {
        usleep(1);
    }

    pool->Clear();
    pool.reset();
    t.Stop(true);
    std::cout << "EventLoopThread stopped.\n";
    return 0;
}
