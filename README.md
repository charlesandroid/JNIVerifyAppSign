# JNIVerifyAppSign
so签名校验
前言
Android应用当中的native方法不能混淆,如果拿到so文件,找到声明native方法的类,就可以直接调用so中的方法,为了避免应用so被第三方调用,可以在native层加入签名校验,防止应用二次签名和so文件安全
思路
1获取正式签名文件的签名
2把签名保存在c/c++文件中
3应用启动时在c/c++中获取当前应用的签名比对
步骤

在grade里配置好签名文件
signingConfigs {
    releaseConfig {
        storePassword "123456"
        keyAlias "key0"
        keyPassword "123456"
        storeFile file("keystore.key")
    }
}
buildTypes {
    release {
        minifyEnabled false
        signingConfig signingConfigs.releaseConfig
        proguardFiles getDefaultProguardFile('proguard-android.txt'), 'proguard-rules.pro'
    }
    debug {
        minifyEnabled false
        signingConfig signingConfigs.releaseConfig
        proguardFiles getDefaultProguardFile('proguard-android.txt'), 'proguard-rules.pro'
    }
}
获取签名,签名过长,可以使用MD5加密,
通过Application获取PackageManager对象和应用包名
通过PackageManager获取PackageInfo
再找到成员变量signatures(Signature数组类型)
拿到第一个成员调用toCharsString方法获取签名
try {
    PackageManager packageManager = getPackageManager();
    PackageInfo packageInfo = packageManager.getPackageInfo(getPackageName(), PackageManager.GET_SIGNATURES);
    Signature[] signatures = packageInfo.signatures;
    Signature signature = signatures[0];
    String sign = signature.toCharsString();
    String signMD5 = MD5Util.MD5(sign);
    Log.e("signature", signMD5);
} catch (PackageManager.NameNotFoundException e) {
    e.printStackTrace();
}
保存签名到c/c++中
const char *APPSIGN = "BD133DCD8B418DC9B6F58FBA42934343";
签名校验
应用在加载so文件的时候
static {
    System.loadLibrary("native-lib");
}
首先调用JNI_OnLoad方法
int JNI_OnLoad(JavaVM *vm, void *reserved) 
在c/c++文件中重写JNI_Onload方法
获取到JNIEnv *env
JNIEnv *env = NULL;
if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
    return JNI_ERR;
}
声明校验方法,传入env
if (verifySign(env) == JNI_OK) {
    return JNI_VERSION_1_4;
}else{
    return JNI_ERR;
}
接下来就是校验签名,获取到当前应用签名,与常量APPSIGN做比对,如果相同返回JNI_VERSION_1_4,否则返回JNI_ERR (-1),系统自动抛出异常
按照Android中获取应用签名的步骤,用jni获取
获取Application对象
方法一:
利用ActivityThread静态方法currentApplication
static jobject getApplication(JNIEnv *env) {
    jobject application = NULL;
    jclass activity_thread_clz = env->FindClass("android/app/ActivityThread");
    if (activity_thread_clz != NULL) {
        jmethodID currentApplication = env->GetStaticMethodID(
                activity_thread_clz, "currentApplication", "()Landroid/app/Application;");
        if (getApplication != NULL) {
            application = env->CallStaticObjectMethod(activity_thread_clz, currentApplication);
        }
        env->DeleteLocalRef(activity_thread_clz);
    }
    return application;
}
这种方法在华为部分机型上不使用,所以改用应用内部提供方法获取application
方法二:
static jobject getApplication(JNIEnv *env) {
    jobject application = NULL;
    jclass application_clz = env->FindClass("com/charles/jniverifyappsign/JNIApplication");
    jmethodID getApplication = env->GetStaticMethodID(
            application_clz, "getApplication", "()Landroid/app/Application;");
    application = env->CallStaticObjectMethod(application_clz, getApplication);
    env->DeleteLocalRef(application_clz);
    return application;
}
这里定义了一个JNIApplication,声明了getApplication方法返回本类对象
public class JNIApplication extends Application {

    private static Application application;

    @Override
    public void onCreate() {
        super.onCreate();
        application = this;
    }

    public static Application getApplication() {
        return application;
    }
}
接下来通过application获取应用签名并MD5加密
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
    const char *sign_MD5 = getMD5(env, signature_str);
获取方法和java层获取方式一样
通过调用java层的MD5Util加密
static const char *getMD5(JNIEnv *env, jstring jstr) {
    jclass md5_clz = env->FindClass("com/charles/jniverifyappsign/MD5Util");
    jmethodID jmethodID = env->GetStaticMethodID(md5_clz, "MD5",
                                                 "(Ljava/lang/String;)Ljava/lang/String;");
    jstring md5 = (jstring) env->CallStaticObjectMethod(md5_clz, jmethodID, jstr);
    return env->GetStringUTFChars(md5, NULL);

}
最后比较两个字符串是否相等
int result = strcmp(sign_MD5, APP_SIGN);
if (result == 0) {
    return JNI_OK;
}
return JNI_ERR;

至此,所有工作搞定,下面换一个签名文件,再生成一个keystore2.key,替换gradle中签名配置

signingConfigs {
    releaseConfig {
        storePassword "123456"
        keyAlias "key0"
        keyPassword "123456"
        storeFile file("keystore2.key")
    }
}
再次运行

应用签名不匹配,JNI_OnLoad方法返回JNI_ERR,系统抛出UnsatisfiedLinkError异常,应用无法正常启动
