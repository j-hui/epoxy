LOCAL_PATH := $(call my-dir)
 
include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
	liblog

LOCAL_MODULE    := EchoService
LOCAL_SRC_FILES := \
    IEchoService.cpp \
    EchoService.cpp
   
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
  

include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
	liblog
LOCAL_MODULE    := EchoClient
LOCAL_SRC_FILES := \
    IEchoService.cpp \
    EchoClient.cpp

LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
