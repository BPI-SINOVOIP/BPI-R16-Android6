# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libctssecurity_jni

# Don't include this package in any configuration by default.
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
		CtsSecurityJniOnLoad.cpp \
		android_security_cts_CharDeviceTest.cpp \
		android_security_cts_KernelSettingsTest.cpp \
		android_security_cts_LinuxRngTest.cpp \
		android_security_cts_LoadEffectLibraryTest.cpp \
		android_security_cts_NativeCodeTest.cpp \
		android_security_cts_SELinuxTest.cpp \
		android_security_cts_MMapExecutableTest.cpp \
		android_security_cts_AudioPolicyBinderTest.cpp \
		android_security_cts_EncryptionTest.cpp \
		android_security_cts_MediaCryptoTest.cpp \
		android_security_cts_MediaPlayerInfoLeakTest.cpp \
		android_security_cts_AudioFlingerBinderTest.cpp \
		android_security_cts_AudioEffectBinderTest.cpp

LOCAL_C_INCLUDES := \
	$(JNI_H_INCLUDE) \
	system/core/include \
	frameworks/base/media/jni

LOCAL_SHARED_LIBRARIES := \
	libnativehelper \
	liblog \
	libbinder \
	libutils \
	libmedia \
	libselinux \
	libdl \
	libcutils \
	libcrypto \
	libstagefright_foundation 

LOCAL_C_INCLUDES += ndk/sources/cpufeatures
LOCAL_STATIC_LIBRARIES := cpufeatures

include $(BUILD_SHARED_LIBRARY)
