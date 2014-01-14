@echo ===========================
@echo === Copying SDL library ===
@echo ===========================
xcopy /Y ..\..\SDKs\SDL\lib\armeabi\*.so libs\armeabi\

@echo ==============================
@echo === Copying Tiny2D library ===
@echo ==============================
xcopy /Y ..\..\Lib\armeabi\*.so libs\armeabi\

@echo ====================
@echo === Building App ===
@echo ====================
call ndk-build.cmd NDK_APPLICATION_MK=../../Project/Android/jni/Application.mk

@echo =======================================================
@echo === Deleting old APK (to make sure it gets rebuilt) ===
@echo =======================================================
del bin\Tiny2DActivity.apk

@pause