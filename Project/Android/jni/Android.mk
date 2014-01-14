LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := Tiny2D

SDL_PATH := ../../../SDKs/SDL
LIB_PATH := ../../..

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH)/$(LIB_PATH)/SDKs/OGGVorbis \
	$(LOCAL_PATH)/$(LIB_PATH)/Include \
	$(LOCAL_PATH)/$(LIB_PATH)/Src \
	$(LOCAL_PATH)/$(LIB_PATH)/Src/SDL \
	$(LOCAL_PATH)/$(LIB_PATH)/Src/OpenGL

LOCAL_SRC_FILES := $(SDL_PATH)/fullsrc/SDL2/src/main/android/SDL_android_main.c \
	$(LIB_PATH)/SDKs/OGGVorbis/stb_vorbis.cpp \
	$(LIB_PATH)/Src/Tiny2D_Common.cpp \
	$(LIB_PATH)/Src/Tiny2D_Particles.cpp \
	$(LIB_PATH)/Src/Tiny2D_Sprite.cpp \
	$(LIB_PATH)/Src/Tiny2D_Postprocessing.cpp \
	$(LIB_PATH)/Src/Tiny2D_RapidXML.cpp \
	$(LIB_PATH)/Src/Tiny2D_RectPacker.cpp \
	$(LIB_PATH)/Src/Tiny2D_Unicode.cpp \
	$(LIB_PATH)/Src/Tiny2D_CPPWrappers.cpp \
	$(LIB_PATH)/Src/Tiny2D_Shape.cpp \
	$(LIB_PATH)/Src/Tiny2D_Localization.cpp \
	$(LIB_PATH)/Src/SDL/Tiny2D_SDL.cpp \
	$(LIB_PATH)/Src/OpenGL/Tiny2D_OpenGL.cpp \
	$(LIB_PATH)/Src/OpenGL/Tiny2D_OpenGLES.cpp \
	$(LIB_PATH)/Src/OpenGL/Tiny2D_OpenGLMaterial.cpp

LOCAL_CPPFLAGS := -DOPENGL_ES

LOCAL_SHARED_LIBRARIES :=

LOCAL_LDLIBS := -lGLESv2 -llog
LOCAL_LDLIBS += -L$(LOCAL_PATH)/$(SDL_PATH)/lib/$(APP_ABI) -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

include $(BUILD_SHARED_LIBRARY)
