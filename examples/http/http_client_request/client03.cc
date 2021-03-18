#include <evpp/event_loop_thread.h>
#include <evpp/httpc/conn_pool.h>
#include <evpp/httpc/response.h>
#include <evpp/httpc/request.h>

#include "../../../examples/winmain-inl.h"

static bool responsed = false;
static void HandleHTTPResponse(const std::shared_ptr<evpp::httpc::Response>& response, evpp::httpc::Request* request) {
    LOG_INFO << "http_code=" << response->http_code() << " [" << response->body().ToString() << "]";
    auto* ConnectionHeader = response->FindHeader("Connection");
    if(ConnectionHeader != nullptr){
        LOG_INFO << "HTTP HEADER Connection=" << std::string(ConnectionHeader);
    }
    responsed = true;
    assert(request == response->request());
    delete request; // The request MUST BE deleted in EventLoop thread.
}

int main() {
    evpp::EventLoopThread t;
    t.Start(true);
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    std::shared_ptr<evpp::httpc::ConnPool> pool(new evpp::httpc::ConnPool("www.360.cn", 443,true, evpp::Duration(10.0)));
    evpp::httpc::SET_SSL_VERIFY_MODE(SSL_VERIFY_NONE);
#else
    std::shared_ptr<evpp::httpc::ConnPool> pool(new evpp::httpc::ConnPool("www.360.cn", 80, evpp::Duration(2.0)));
#endif
    auto* r = new evpp::httpc::Request(pool.get(), t.loop(), "/robots.txt", "");
    LOG_INFO << "Do http request";
    r->Execute([&r](auto && PH1) { return HandleHTTPResponse(std::forward<decltype(PH1)>(PH1), r); });

    while (!responsed) {
        usleep(1);
    }

    pool->Clear();
    pool.reset();
    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
    return 0;
}
