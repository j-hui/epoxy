LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall

LOCAL_C_INCLUDES:= system/core/base/include

LOCAL_SRC_FILES:= \
	epoxyd.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils libutils libbinder liblog

LOCAL_MODULE:= epoxyd

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
