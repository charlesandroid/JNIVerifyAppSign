package com.charles.jniverifyappsign;

import android.app.Application;

/**
 * com.charles.jniverifyappsign.JNIApplication
 *
 * @author Just.T
 * @since 17/5/7
 */
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
