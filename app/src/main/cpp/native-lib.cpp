#include <jni.h>
#include <string>

const char *APP_SIGN = "BD133DCD8B418DC9B6F58FBA42934343";
extern "C"
JNIEXPORT jstring JNICALL
Java_com_charles_jniverifyappsign_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello World";
    return env->NewStringUTF(hello.c_str());
}

static const char *getMD5(JNIEnv *env, jstring jstr) {
    jclass md5_clz = env->FindClass("com/charles/jniverifyappsign/MD5Util");
    jmethodID jmethodID = env->GetStaticMethodID(md5_clz, "MD5",
                                                 "(Ljava/lang/String;)Ljava/lang/String;");
    jstring md5 = (jstring) env->CallStaticObjectMethod(md5_clz, jmethodID, jstr);
    return env->GetStringUTFChars(md5, NULL);

}

//static jobject getApplication(JNIEnv *env) {
//    jobject application = NULL;
//    jclass activity_thread_clz = env->FindClass("android/app/ActivityThread");
//    if (activity_thread_clz != NULL) {
//        jmethodID currentApplication = env->GetStaticMethodID(
//                activity_thread_clz, "currentApplication", "()Landroid/app/Application;");
//        if (getApplication != NULL) {
//            application = env->CallStaticObjectMethod(activity_thread_clz, currentApplication);
//        }
//        env->DeleteLocalRef(activity_thread_clz);
//    }
//    return application;
//}
static jobject getApplication(JNIEnv *env) {
    jobject application = NULL;
    jclass application_clz = env->FindClass("com/charles/jniverifyappsign/JNIApplication");
    jmethodID getApplication = env->GetStaticMethodID(
            application_clz, "getApplication", "()Landroid/app/Application;");
    application = env->CallStaticObjectMethod(application_clz, getApplication);
    env->DeleteLocalRef(application_clz);
    return application;
}

static int verifySign(JNIEnv *env) {
    jobject application = getApplication(env);
    if (application == NULL) {
        return JNI_ERR;
    }
    jclass context_clz = env->GetObjectClass(application);
    jmethodID getPackageManager = env->GetMethodID(context_clz, "getPackageManager",
                                                   "()Landroid/content/pm/PackageManager;");
    jobject package_manager = env->CallObjectMethod(application, getPackageManager);
    jclass package_manager_clz = env->GetObjectClass(package_manager);
    jmethodID getPackageInfo = env->GetMethodID(package_manager_clz, "getPackageInfo",
                                                "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
    jmethodID getPackageName = env->GetMethodID(context_clz, "getPackageName",
                                                "()Ljava/lang/String;");
    jstring package_name = (jstring) (env->CallObjectMethod(application, getPackageName));
    jobject package_info = env->CallObjectMethod(package_manager, getPackageInfo, package_name, 64);
    jclass package_info_clz = env->GetObjectClass(package_info);
    jfieldID signatures_field = env->GetFieldID(package_info_clz, "signatures",
                                                "[Landroid/content/pm/Signature;");
    jobject signatures = env->GetObjectField(package_info, signatures_field);
    jobjectArray signatures_array = (jobjectArray) signatures;
    jobject signature0 = env->GetObjectArrayElement(signatures_array, 0);
    jclass signature_clz = env->GetObjectClass(signature0);

    jmethodID toCharsString = env->GetMethodID(signature_clz, "toCharsString",
                                               "()Ljava/lang/String;");
    jstring signature_str = (jstring) (env->CallObjectMethod(signature0, toCharsString));
    env->DeleteLocalRef(application);
    env->DeleteLocalRef(context_clz);
    env->DeleteLocalRef(package_manager);
    env->DeleteLocalRef(package_manager_clz);
    env->DeleteLocalRef(package_info);
    env->DeleteLocalRef(package_info_clz);
    env->DeleteLocalRef(signatures);
    env->DeleteLocalRef(signature0);
    env->DeleteLocalRef(signature_clz);
    env->DeleteLocalRef(package_name);
    const char *temp = getMD5(env, signature_str);
    int result = strcmp(temp, APP_SIGN);
    env->ReleaseStringUTFChars(signature_str, temp);
    env->DeleteLocalRef(signature_str);
    if (result == 0) {
        return JNI_OK;
    }
    return JNI_ERR;
}

int JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }
    if (verifySign(env) == JNI_OK) {
        return JNI_VERSION_1_4;
    }
    return JNI_ERR;
}