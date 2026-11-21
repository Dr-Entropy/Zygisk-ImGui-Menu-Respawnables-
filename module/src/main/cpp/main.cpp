#include <cstring>
#include <jni.h>
#include <pthread.h>
#include "hook.h"
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;
class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        api_ = api;
        env_ = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        enable_hack = false;
        if (args && args->nice_name && args->app_data_dir)
            enable_hack = isGame(env_, args->app_data_dir);
        // Any other app (browser, etc.): unload this library so it stays out of those processes entirely
        if (!enable_hack) api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (enable_hack) {
            int ret;
            pthread_t ntid;
            if ((ret = pthread_create(&ntid, nullptr, hack_thread, nullptr))) {
                LOGE("can't create thread: %s\n", strerror(ret));
            }
        }
    }

private:
    Api *api_{};
    JNIEnv *env_{};
};

REGISTER_ZYGISK_MODULE(MyModule)
