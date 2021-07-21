#include <jni.h>
#include <string>
#include "AntiDebug.h"

AntiDebug *pDebug = NULL;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    AntiDebug::antiDebug(vm);
    return JNI_VERSION_1_4; //这里很重要，必须返回版本，否则加载会失败。
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_securemodule_AntiDebug_startCheckDebug(JNIEnv *env,
                                                        jclass type,
                                                        jobject jCallback) {

    if (pDebug != NULL) {
        pDebug->stopCheck();
    }

    jclass jclazz = env->GetObjectClass(jCallback);
    jobject g_callbackRef = env->NewGlobalRef(jCallback);
    jmethodID g_MethodCallback = env->GetMethodID(jclazz, "beInjectedDebug", "()V");
    pDebug = new AntiDebug(g_callbackRef, g_MethodCallback);
    pDebug->startCheck();

}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_securemodule_AntiDebug_stopCheck(JNIEnv *env, jclass clazz) {

    if (pDebug != NULL) {
        pDebug->stopCheck();
    }
}