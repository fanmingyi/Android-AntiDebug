#ifndef _ANTI_DEBUG_H
#define _ANTI_DEBUG_H

#include <jni.h>
#include <cstdio>

#define MACRO_HIDE_SYMBOL __attribute__ ((visibility ("hidden")))

class AntiDebug {
public:
    static void antiDebug(JavaVM *jvm);

private:
    /**
     * 存在某些引用才会执行某些检测
     */
    jclass mXPosedGlobalRef;

    static AntiDebug *s_instance;
    /**
     * 检测是否需要停止检测
     * true停止
     */
    bool stopCheckFlag;

    /**
     * 回调使用
     */
    jobject g_callbackRef;
    /**
     * 回调使用
     */
    jmethodID g_MethodCallback;

    void recycle();

public:
    AntiDebug(jobject g_callbackRef, jmethodID g_MethodCallback);

    /**
     * 开始检测
     */
    void startCheck();

    /**
     * 停止检测
     */
    void stopCheck();

private:
    pthread_t mThread = NULL;

    static void *antiDebugCallback(void *arg);

    void getGlobalRef();

    bool readStatus();

    bool isBeDebug();

    bool IsHookByXPosed();


};

#endif