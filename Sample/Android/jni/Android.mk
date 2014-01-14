LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := Tiny2DApp

SDL_PATH := ../../../SDKs/SDL
LIB_PATH := ../../..

# List your include directories here
LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH)/$(LIB_PATH)/Include

# List your source files here
LOCAL_SRC_FILES := $(LIB_PATH)/Sample/Tiny2D_Sample.cpp

LOCAL_CPPFLAGS := -DOPENGL_ES

LOCAL_SHARED_LIBRARIES :=

LOCAL_LDLIBS := -lGLESv2 -llog
LOCAL_LDLIBS += -L$(LOCAL_PATH)/$(LIB_PATH)/Lib/$(APP_ABI) -lTiny2D

include $(BUILD_SHARED_LIBRARY)
