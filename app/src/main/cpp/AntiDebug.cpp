#include "AntiDebug.h"
#include "Log.h"
#include <sys/ptrace.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <pthread.h>


using namespace std;


MACRO_HIDE_SYMBOL JavaVM *g_jvm = NULL;

MACRO_HIDE_SYMBOL AntiDebug *AntiDebug::s_instance = NULL;

MACRO_HIDE_SYMBOL JNIEnv *GetEnv() {
    if (g_jvm == NULL)
        return NULL;

    int status;
    JNIEnv *env = NULL;
    status = g_jvm->GetEnv((void **) &env, JNI_VERSION_1_4);
    if (status < 0) {
        status = g_jvm->AttachCurrentThread(&env, NULL);
        if (status < 0) {
            return NULL;
        }
    }

    return env;
}


//检测进程状态
MACRO_HIDE_SYMBOL bool AntiDebug::readStatus() {
    const int bufSize = 1024;
    char fileName[bufSize];
    char contentLine[bufSize];
    int ppid = 0;
    int pid = getpid();
    sprintf(fileName, "/proc/%d/status", pid);
    FILE *fd = fopen(fileName, "r");
    if (fd != NULL) {
        while (fgets(contentLine, bufSize, fd)) {
            if (strncmp(contentLine, "PPid", 4) == 0) {
                ppid = atoi(&contentLine[5]);
            }

            if (strncmp(contentLine, "TracerPid", 9) == 0) {
                int statue = atoi(&contentLine[10]);
                if (statue != 0 && ppid != statue) {
                    LOG_PRINT_E("app be debug by ida or lldb.");
                    fclose(fd);
                    return true;
                }
                break;
            }
        }
        fclose(fd);
    } else {
        LOG_PRINT_E("status file open %s fail...", fileName);
    }

    return false;
}

//Check if it is injected by Xposed
MACRO_HIDE_SYMBOL bool AntiDebug::IsHookByXPosed() {
    char buf[1024] = {0};
    FILE *fp;
    int pid = getpid();
    sprintf(buf, "/proc/%d/maps", pid);
    fp = fopen(buf, "r");
    if (fp == NULL) {
        LOG_PRINT_E("Error open maps file in progress %d", pid);
        return false;
    }

    if (mXPosedGlobalRef != 0) {
        LOG_PRINT_E("app be injected by xposed or substrate.");
        return true;
    }

    while (fgets(buf, sizeof(buf), fp)) {
        if (strstr(buf, "com.saurik.substrate") || strstr(buf, "io.va.exposed") ||
            strstr(buf, "de.robv.android.xposed")) {
            LOG_PRINT_E("app be injected by xposed or substrate.");
            fclose(fp);
            return true;
        }
    }
    fclose(fp);

    return false;
}

jclass jDebugClazz = NULL;
jmethodID mthIsDebuggerConn = NULL;
//检测调试器状态
MACRO_HIDE_SYMBOL bool AntiDebug::isBeDebug() {

    JNIEnv *env = GetEnv();
    if (env == NULL)
        return false;

    if (jDebugClazz == NULL) {
        jDebugClazz = env->FindClass("android/os/Debug");
    }
    if (mthIsDebuggerConn == NULL) {
        mthIsDebuggerConn = env->GetStaticMethodID(jDebugClazz, "isDebuggerConnected", "()Z");
    }


    jboolean jIsDebuggerConnected = env->CallStaticBooleanMethod(jDebugClazz, mthIsDebuggerConn);

    //DetachCurrent();
    if (jIsDebuggerConnected) {
        LOG_PRINT_E("jIsDebuggerConnected = %d", jIsDebuggerConnected);
        return true;
    }

    return false;
}

//反调试检测
MACRO_HIDE_SYMBOL void *AntiDebug::antiDebugCallback(void *arg) {
    if (arg == NULL)
        return NULL;

    AntiDebug *pAntiDebug = (AntiDebug *) arg;

    while (!pAntiDebug->stopCheckFlag) {
        try {
            //https://github.com/weikaizhi/AntiDebug/blob/master/app/src/main/cpp/AntiDebug.h
            bool bRet1 = pAntiDebug->readStatus();
            bool bRet2 = pAntiDebug->IsHookByXPosed();
            bool bRet3 = pAntiDebug->isBeDebug();
            LOG_PRINT_E("检测 ptrace 结果 %d , xposed 检测 %d , jdwp 检测 %d ", bRet1, bRet2, bRet3);
//            https://github.com/weikaizhi/AntiDebug/blob/master/app/src/main/cpp/AntiDebug.h
            if (bRet1 || bRet2 || bRet3) {
                if (pAntiDebug->g_callbackRef != 0 && pAntiDebug->g_MethodCallback != 0) {
                    JNIEnv *env = GetEnv();
                    if (env != NULL) {
                        env->CallVoidMethod(pAntiDebug->g_callbackRef,
                                            pAntiDebug->g_MethodCallback);
                    }
                }
                pAntiDebug->stopCheckFlag = true;
            }
        } catch (...) {

        }


        sleep(1);
    }


    pAntiDebug->recycle();
    (g_jvm)->DetachCurrentThread();
    return NULL;
}

MACRO_HIDE_SYMBOL void AntiDebug::getGlobalRef() {
    mXPosedGlobalRef = 0;
    int status;
    JNIEnv *env = NULL;
    status = g_jvm->GetEnv((void **) &env, JNI_VERSION_1_4);

    if (env == NULL)
        return;

    try {
        char szClazzName[256] = {0};
        //
        sprintf(szClazzName, "de/robv/android/xposed/XposedBridge");
        jclass jXPosedClazz = env->FindClass(szClazzName);
        //如果找不到类那么会抛出异常
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
        if (jXPosedClazz != 0) {
            mXPosedGlobalRef = (jclass) env->NewGlobalRef(jXPosedClazz);
        }
    }
    catch (...) {

    }
}


MACRO_HIDE_SYMBOL void AntiDebug::antiDebug(JavaVM *jvm) {
    g_jvm = jvm;
//    if (s_instance == NULL) {
//        s_instance = new AntiDebug();
//        s_instance->stopCheck = false;
//        s_instance->antiDebugInner();
//    }
}

void AntiDebug::recycle() {

    JNIEnv *env = NULL;
    g_jvm->GetEnv((void **) &env, JNI_VERSION_1_4);
    if (env == NULL)
        return;
    try {
        //删除xposed引用
        env->DeleteGlobalRef((jobject) (mXPosedGlobalRef));
        //删除回调引用
        env->DeleteGlobalRef((jobject) (g_callbackRef));
        //删除本地引用
        env->DeleteLocalRef(jDebugClazz);
    } catch (...) {
        LOG_PRINT_E("回收资源错误");
    }
}

AntiDebug::AntiDebug(jobject g_callbackRef2, jmethodID g_MethodCallback2) :
        g_callbackRef(g_callbackRef2), g_MethodCallback(g_MethodCallback2) {

}

void AntiDebug::startCheck() {
    getGlobalRef();
    ptrace(PTRACE_TRACEME, 0, 0, 0);
    stopCheckFlag = false;
    pthread_create(&mThread, NULL, AntiDebug::antiDebugCallback, this);
}

/**
 * 停止
 */
void AntiDebug::stopCheck() {
    stopCheckFlag = true;
    if (mThread != NULL) {
        pthread_join(mThread, NULL);
        mThread = NULL;
    }
}

