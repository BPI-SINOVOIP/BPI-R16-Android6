/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.settings;

import com.android.internal.logging.MetricsLogger;
import com.android.internal.view.RotationPolicy;
import com.android.settings.DropDownPreference.Callback;
import com.android.settings.search.BaseSearchIndexProvider;
import com.android.settings.search.Indexable;

import static android.provider.Settings.Secure.CAMERA_DOUBLE_TAP_POWER_GESTURE_DISABLED;
import static android.provider.Settings.Secure.CAMERA_GESTURE_DISABLED;
import static android.provider.Settings.Secure.DOUBLE_TAP_TO_WAKE;
import static android.provider.Settings.Secure.DOZE_ENABLED;
import static android.provider.Settings.Secure.WAKE_GESTURE_ENABLED;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL;
import static android.provider.Settings.System.SCREEN_OFF_TIMEOUT;

import android.app.Activity;
import android.app.ActivityManagerNative;
import android.app.Dialog;
import android.app.UiModeManager;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceCategory;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.provider.SearchIndexableResource;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.text.TextUtils;
import android.util.Log;
import android.os.UserHandle;

import java.util.ArrayList;
import java.util.List;

public class DisplaySettings extends SettingsPreferenceFragment implements
        Preference.OnPreferenceChangeListener, OnPreferenceClickListener, Indexable {
    private static final String TAG = "DisplaySettings";

    /** If there is no setting in the provider, use this. */
    private static final int FALLBACK_SCREEN_TIMEOUT_VALUE = 30000;

    private static final String KEY_SCREEN_TIMEOUT = "screen_timeout";
    private static final String KEY_FONT_SIZE = "font_size";
    private static final String KEY_SCREEN_SAVER = "screensaver";
    private static final String KEY_LIFT_TO_WAKE = "lift_to_wake";
    private static final String KEY_DOZE = "doze";
    private static final String KEY_TAP_TO_WAKE = "tap_to_wake";
    private static final String KEY_AUTO_BRIGHTNESS = "auto_brightness";
    private static final String KEY_AUTO_ROTATE = "auto_rotate";
    private static final String KEY_NIGHT_MODE = "night_mode";
    private static final String KEY_CAMERA_GESTURE = "camera_gesture";
    private static final String KEY_CAMERA_DOUBLE_TAP_POWER_GESTURE
            = "camera_double_tap_power_gesture";

	/* add by allwinner */
    private static final String KEY_BRIGHT_SYSTEM = "bright_system";
    private static final String KEY_BRIGHT_SYSTEM_DEMO = "bright_demo_mode";
    private static final String KEY_BRIGHTNESS_LIGHT = "brightness_light";
    private static final String KEY_BRIGHTNESS_LIGHT_DEMO = "backlight_demo_mode";
    private static final String KEY_HDMI_OUTPUT_MODE = "hdmi_output_mode";
    private static final String KEY_HDMI_OUTPUT_MODE_720P = "hdmi_output_mode_720p";
    private static final String KEY_HDMI_OUTPUT_MODE_CATE = "hdmi_output_mode_cate";
    private static final String KEY_HDMI_FULL_SCREEN = "hdmi_full_screen";
    private static final String KEY_SCREEN_RECORD = "screen_record";

    private static final int DLG_GLOBAL_CHANGE_WARNING = 1;

    private WarnedListPreference mFontSizePref;
    private Preference mScreenRecordPref;

    private final Configuration mCurConfig = new Configuration();

    private ListPreference mScreenTimeoutPreference;
    private ListPreference mNightModePreference;
    private Preference mScreenSaverPreference;
    private SwitchPreference mLiftToWakePreference;
    private SwitchPreference mDozePreference;
    private SwitchPreference mTapToWakePreference;
    private SwitchPreference mAutoBrightnessPreference;
    private SwitchPreference mCameraGesturePreference;
    private SwitchPreference mCameraDoubleTapPowerGesturePreference;

	/* add by allwinner */
    private CheckBoxPreference mBrightSystem,mBrightSystemDemo;
    private CheckBoxPreference mBrightnessLight,mBrightnessLightDemo;
    private ListPreference mHdmiOutputModePreference;
    private PreferenceCategory mHdmiOutputModeCategory;
    private CheckBoxPreference mHdmiFullScreen;

    @Override
    protected int getMetricsCategory() {
        return MetricsLogger.DISPLAY;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Activity activity = getActivity();
        final ContentResolver resolver = activity.getContentResolver();

        addPreferencesFromResource(R.xml.display_settings);

        mScreenSaverPreference = findPreference(KEY_SCREEN_SAVER);
        if (mScreenSaverPreference != null
                && getResources().getBoolean(
                        com.android.internal.R.bool.config_dreamsSupported) == false) {
            getPreferenceScreen().removePreference(mScreenSaverPreference);
        }

        mScreenTimeoutPreference = (ListPreference) findPreference(KEY_SCREEN_TIMEOUT);
        final long currentTimeout = Settings.System.getLong(resolver, SCREEN_OFF_TIMEOUT,
                FALLBACK_SCREEN_TIMEOUT_VALUE);
        mScreenTimeoutPreference.setValue(String.valueOf(currentTimeout));
        mScreenTimeoutPreference.setOnPreferenceChangeListener(this);
        disableUnusableTimeouts(mScreenTimeoutPreference);
        updateTimeoutPreferenceDescription(currentTimeout);

        mFontSizePref = (WarnedListPreference) findPreference(KEY_FONT_SIZE);
        mFontSizePref.setOnPreferenceChangeListener(this);
        mFontSizePref.setOnPreferenceClickListener(this);
        
        mScreenRecordPref = findPreference(KEY_SCREEN_RECORD);
        mScreenRecordPref.setOnPreferenceClickListener(this);

        if (isAutomaticBrightnessAvailable(getResources())) {
            mAutoBrightnessPreference = (SwitchPreference) findPreference(KEY_AUTO_BRIGHTNESS);
            mAutoBrightnessPreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_AUTO_BRIGHTNESS);
        }

        if (isLiftToWakeAvailable(activity)) {
            mLiftToWakePreference = (SwitchPreference) findPreference(KEY_LIFT_TO_WAKE);
            mLiftToWakePreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_LIFT_TO_WAKE);
        }

        if (isDozeAvailable(activity)) {
            mDozePreference = (SwitchPreference) findPreference(KEY_DOZE);
            mDozePreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_DOZE);
        }

        if (isTapToWakeAvailable(getResources())) {
            mTapToWakePreference = (SwitchPreference) findPreference(KEY_TAP_TO_WAKE);
            mTapToWakePreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_TAP_TO_WAKE);
        }

        if (isCameraGestureAvailable(getResources())) {
            mCameraGesturePreference = (SwitchPreference) findPreference(KEY_CAMERA_GESTURE);
            mCameraGesturePreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_CAMERA_GESTURE);
        }

        if (isCameraDoubleTapPowerGestureAvailable(getResources())) {
            mCameraDoubleTapPowerGesturePreference
                    = (SwitchPreference) findPreference(KEY_CAMERA_DOUBLE_TAP_POWER_GESTURE);
            mCameraDoubleTapPowerGesturePreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_CAMERA_DOUBLE_TAP_POWER_GESTURE);
        }

        if (RotationPolicy.isRotationLockToggleVisible(activity)) {
            DropDownPreference rotatePreference =
                    (DropDownPreference) findPreference(KEY_AUTO_ROTATE);
            rotatePreference.addItem(activity.getString(R.string.display_auto_rotate_rotate),
                    false);
            int rotateLockedResourceId;
            // The following block sets the string used when rotation is locked.
            // If the device locks specifically to portrait or landscape (rather than current
            // rotation), then we use a different string to include this information.
            if (allowAllRotations(activity)) {
                rotateLockedResourceId = R.string.display_auto_rotate_stay_in_current;
            } else {
                if (RotationPolicy.getRotationLockOrientation(activity)
                        == Configuration.ORIENTATION_PORTRAIT) {
                    rotateLockedResourceId =
                            R.string.display_auto_rotate_stay_in_portrait;
                } else {
                    rotateLockedResourceId =
                            R.string.display_auto_rotate_stay_in_landscape;
                }
            }
            rotatePreference.addItem(activity.getString(rotateLockedResourceId), true);
            rotatePreference.setSelectedItem(RotationPolicy.isRotationLocked(activity) ?
                    1 : 0);
            rotatePreference.setCallback(new Callback() {
                @Override
                public boolean onItemSelected(int pos, Object value) {
                    final boolean locked = (Boolean) value;
                    MetricsLogger.action(getActivity(), MetricsLogger.ACTION_ROTATION_LOCK,
                            locked);
                    RotationPolicy.setRotationLock(activity, locked);
                    return true;
                }
            });
        } else {
            removePreference(KEY_AUTO_ROTATE);
        }

        mNightModePreference = (ListPreference) findPreference(KEY_NIGHT_MODE);
        if (mNightModePreference != null) {
            final UiModeManager uiManager = (UiModeManager) getSystemService(
                    Context.UI_MODE_SERVICE);
            final int currentNightMode = uiManager.getNightMode();
            mNightModePreference.setValue(String.valueOf(currentNightMode));
            mNightModePreference.setOnPreferenceChangeListener(this);
        }

		/* add by allwinner */
		mBrightSystem = (CheckBoxPreference)findPreference(KEY_BRIGHT_SYSTEM);
		mBrightSystemDemo = (CheckBoxPreference)findPreference(KEY_BRIGHT_SYSTEM_DEMO);
		boolean demoEnabled;
		if(mBrightSystem != null) {
			try{
					demoEnabled = (Settings.System.getInt(resolver,
									Settings.System.BRIGHT_SYSTEM_MODE)&0x01) > 0;
					mBrightSystem.setChecked(demoEnabled);
					mBrightSystem.setOnPreferenceChangeListener(this);
					if (mBrightSystemDemo != null && demoEnabled) {
							try {
									mBrightSystemDemo.setChecked((Settings.System.getInt(resolver,
													Settings.System.BRIGHT_SYSTEM_MODE)&0x02) > 0);
									mBrightSystemDemo.setOnPreferenceChangeListener(this);
								}	catch (SettingNotFoundException snfe)	{
									Log.e(TAG,Settings.System.BRIGHT_SYSTEM_MODE + " not found");
								}
							} else if (mBrightSystemDemo == null) {
									getPreferenceScreen().removePreference(mBrightSystemDemo);
								}	else {
										mBrightSystemDemo.setEnabled(demoEnabled);
									}
								}catch (SettingNotFoundException snfe) {
									Log.e(TAG, Settings.System.BRIGHT_SYSTEM_MODE + " not found");
								}
					}else {
							getPreferenceScreen().removePreference(mBrightSystem);
						}

					mBrightnessLight = (CheckBoxPreference)findPreference(KEY_BRIGHTNESS_LIGHT);
					mBrightnessLightDemo = (CheckBoxPreference)findPreference(KEY_BRIGHTNESS_LIGHT_DEMO);
					if(mBrightnessLight != null) {
						try{
								demoEnabled = (Settings.System.getInt(resolver,
												Settings.System.BRIGHTNESS_LIGHT_MODE)&0x01) > 0;
								mBrightnessLight.setChecked(demoEnabled);
								mBrightnessLight.setOnPreferenceChangeListener(this);
								if (mBrightnessLightDemo != null && demoEnabled) {
										try {
												mBrightnessLightDemo.setChecked((Settings.System.getInt(resolver,
																Settings.System.BRIGHTNESS_LIGHT_MODE)&0x02) > 0);
												mBrightnessLightDemo.setOnPreferenceChangeListener(this);
											}	catch (SettingNotFoundException snfe)	{
												Log.e(TAG,Settings.System.BRIGHTNESS_LIGHT_MODE + " not found");
											}
										} else if (mBrightnessLightDemo == null) {
												getPreferenceScreen().removePreference(mBrightnessLightDemo);
											}	else {
													mBrightnessLightDemo.setEnabled(demoEnabled);
												}
											}catch (SettingNotFoundException snfe) {
												Log.e(TAG, Settings.System.BRIGHTNESS_LIGHT_MODE + " not found");
											}
						}else {
								getPreferenceScreen().removePreference(mBrightnessLight);
        }

        final int sethdmimode = getResources().getInteger(R.integer.config_hdmi_settings_mode);
        final boolean isShowHdmiMode = (sethdmimode & 0x03) > 0;
        final boolean isShow1080p = (sethdmimode & 0x02) > 0;
        final boolean isShowFullScreen = (sethdmimode & 0x04) > 0;
        mHdmiOutputModeCategory = (PreferenceCategory) findPreference(KEY_HDMI_OUTPUT_MODE_CATE);
        mHdmiFullScreen = (CheckBoxPreference)findPreference(KEY_HDMI_FULL_SCREEN);
        if (isShow1080p) {
            mHdmiOutputModePreference = (ListPreference) findPreference(KEY_HDMI_OUTPUT_MODE_720P);
            mHdmiOutputModeCategory.removePreference(mHdmiOutputModePreference);
            mHdmiOutputModePreference = (ListPreference) findPreference(KEY_HDMI_OUTPUT_MODE);
        } else {
            mHdmiOutputModePreference = (ListPreference) findPreference(KEY_HDMI_OUTPUT_MODE);
            mHdmiOutputModeCategory.removePreference(mHdmiOutputModePreference);
            mHdmiOutputModePreference = (ListPreference) findPreference(KEY_HDMI_OUTPUT_MODE_720P);
        }

        if (sethdmimode != 0) {
            if (isShowHdmiMode) {
                final int currentHdmiMode = Settings.System.getInt(resolver, Settings.System.HDMI_OUTPUT_MODE, 10);
                mHdmiOutputModePreference.setValue(String.valueOf(currentHdmiMode));
                mHdmiOutputModePreference.setOnPreferenceChangeListener(this);
            } else {
                mHdmiOutputModeCategory.removePreference(mHdmiOutputModePreference);
                mHdmiOutputModePreference = null;
            }

            if (isShowFullScreen) {
                final boolean isHdmiFullScreen = Settings.System.getInt(resolver,
                        Settings.System.HDMI_FULL_SCREEN, 0) > 0;
                mHdmiFullScreen.setChecked(isHdmiFullScreen);
                mHdmiFullScreen.setOnPreferenceChangeListener(this);
            } else {
                mHdmiOutputModeCategory.removePreference(mHdmiFullScreen);
                mHdmiFullScreen = null;
            }
        } else {
            getPreferenceScreen().removePreference(mHdmiOutputModeCategory);
            mHdmiOutputModePreference = null;
            mHdmiOutputModeCategory = null;
            mHdmiFullScreen = null;
        }
    }

    private static boolean allowAllRotations(Context context) {
        return Resources.getSystem().getBoolean(
                com.android.internal.R.bool.config_allowAllRotations);
    }

    private static boolean isLiftToWakeAvailable(Context context) {
        SensorManager sensors = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        return sensors != null && sensors.getDefaultSensor(Sensor.TYPE_WAKE_GESTURE) != null;
    }

    private static boolean isDozeAvailable(Context context) {
        String name = Build.IS_DEBUGGABLE ? SystemProperties.get("debug.doze.component") : null;
        if (TextUtils.isEmpty(name)) {
            name = context.getResources().getString(
                    com.android.internal.R.string.config_dozeComponent);
        }
        return !TextUtils.isEmpty(name);
    }

    private static boolean isTapToWakeAvailable(Resources res) {
        return res.getBoolean(com.android.internal.R.bool.config_supportDoubleTapWake);
    }

    private static boolean isAutomaticBrightnessAvailable(Resources res) {
        return res.getBoolean(com.android.internal.R.bool.config_automatic_brightness_available);
    }

    private static boolean isCameraGestureAvailable(Resources res) {
        boolean configSet = res.getInteger(
                com.android.internal.R.integer.config_cameraLaunchGestureSensorType) != -1;
        return configSet &&
                !SystemProperties.getBoolean("gesture.disable_camera_launch", false);
    }

    private static boolean isCameraDoubleTapPowerGestureAvailable(Resources res) {
        return res.getBoolean(
                com.android.internal.R.bool.config_cameraDoubleTapPowerGestureEnabled);
    }

    private void updateTimeoutPreferenceDescription(long currentTimeout) {
        ListPreference preference = mScreenTimeoutPreference;
        String summary;
        if (currentTimeout < 0) {
            // Unsupported value
            summary = "";
        } else {
            final CharSequence[] entries = preference.getEntries();
            final CharSequence[] values = preference.getEntryValues();
            if (entries == null || entries.length == 0) {
                summary = "";
            } else {
                int best = 0;
                for (int i = 0; i < values.length; i++) {
                    long timeout = Long.parseLong(values[i].toString());
                    if (currentTimeout >= timeout) {
                        best = i;
                    }
                }
                if (currentTimeout == Integer.MAX_VALUE)
                    summary = entries[best].toString();
                else
                    summary = preference.getContext().getString(R.string.screen_timeout_summary,
                            entries[best]);
            }
        }
        preference.setSummary(summary);
    }

    private void checkAddNeverTimeout(ListPreference screenTimeoutPreference) {
        boolean isShowNeverTimeout = screenTimeoutPreference.getContext().getResources().getBoolean(
                R.bool.config_show_screen_off_never_timeout);
        if (!isShowNeverTimeout) {
            return;
        }
        final CharSequence[] entries = screenTimeoutPreference.getEntries();
        final CharSequence[] values = screenTimeoutPreference.getEntryValues();
        ArrayList<CharSequence> newEntries = new ArrayList<CharSequence>();
        ArrayList<CharSequence> newValues = new ArrayList<CharSequence>();
        for (int i = 0; i < values.length; i++) {
            newEntries.add(entries[i]);
            newValues.add(values[i]);
        }
        newEntries.add(screenTimeoutPreference.getContext().getString(R.string.screen_off_never_sleep_summary));
        newValues.add(String.valueOf(Integer.MAX_VALUE));
        screenTimeoutPreference.setEntries(
                newEntries.toArray(new CharSequence[newEntries.size()]));
        screenTimeoutPreference.setEntryValues(
                newValues.toArray(new CharSequence[newValues.size()]));
    }

    private void disableUnusableTimeouts(ListPreference screenTimeoutPreference) {
        checkAddNeverTimeout(screenTimeoutPreference);
        final DevicePolicyManager dpm =
                (DevicePolicyManager) getActivity().getSystemService(
                Context.DEVICE_POLICY_SERVICE);
        final long maxTimeout = dpm != null ? dpm.getMaximumTimeToLock(null) : 0;
        if (maxTimeout == 0) {
            return; // policy not enforced
        }
        final CharSequence[] entries = screenTimeoutPreference.getEntries();
        final CharSequence[] values = screenTimeoutPreference.getEntryValues();
        ArrayList<CharSequence> revisedEntries = new ArrayList<CharSequence>();
        ArrayList<CharSequence> revisedValues = new ArrayList<CharSequence>();
        for (int i = 0; i < values.length; i++) {
            long timeout = Long.parseLong(values[i].toString());
            if (timeout <= maxTimeout) {
                revisedEntries.add(entries[i]);
                revisedValues.add(values[i]);
            }
        }
        if (revisedEntries.size() != entries.length || revisedValues.size() != values.length) {
            final int userPreference = Integer.parseInt(screenTimeoutPreference.getValue());
            screenTimeoutPreference.setEntries(
                    revisedEntries.toArray(new CharSequence[revisedEntries.size()]));
            screenTimeoutPreference.setEntryValues(
                    revisedValues.toArray(new CharSequence[revisedValues.size()]));
            if (userPreference <= maxTimeout) {
                screenTimeoutPreference.setValue(String.valueOf(userPreference));
            } else if (revisedValues.size() > 0
                    && Long.parseLong(revisedValues.get(revisedValues.size() - 1).toString())
                    == maxTimeout) {
                // If the last one happens to be the same as the max timeout, select that
                screenTimeoutPreference.setValue(String.valueOf(maxTimeout));
            } else {
                // There will be no highlighted selection since nothing in the list matches
                // maxTimeout. The user can still select anything less than maxTimeout.
                // TODO: maybe append maxTimeout to the list and mark selected.
            }
        }
        screenTimeoutPreference.setEnabled(revisedEntries.size() > 0);
    }

    int floatToIndex(float val) {
        String[] indices = getResources().getStringArray(R.array.entryvalues_font_size);
        float lastVal = Float.parseFloat(indices[0]);
        for (int i=1; i<indices.length; i++) {
            float thisVal = Float.parseFloat(indices[i]);
            if (val < (lastVal + (thisVal-lastVal)*.5f)) {
                return i-1;
            }
            lastVal = thisVal;
        }
        return indices.length-1;
    }

    public void readFontSizePreference(ListPreference pref) {
        try {
            mCurConfig.updateFrom(ActivityManagerNative.getDefault().getConfiguration());
        } catch (RemoteException e) {
            Log.w(TAG, "Unable to retrieve font size");
        }

        // mark the appropriate item in the preferences list
        int index = floatToIndex(mCurConfig.fontScale);
        pref.setValueIndex(index);

        // report the current size in the summary text
        final Resources res = getResources();
        String[] fontSizeNames = res.getStringArray(R.array.entries_font_size);
        pref.setSummary(String.format(res.getString(R.string.summary_font_size),
                fontSizeNames[index]));
    }

    @Override
    public void onResume() {
        super.onResume();
        updateState();
    }

    @Override
    public Dialog onCreateDialog(int dialogId) {
        if (dialogId == DLG_GLOBAL_CHANGE_WARNING) {
            return Utils.buildGlobalChangeWarningDialog(getActivity(),
                    R.string.global_font_change_title,
                    new Runnable() {
                        public void run() {
                            mFontSizePref.click();
                        }
                    });
        }
        return null;
    }

    private void updateState() {
        readFontSizePreference(mFontSizePref);
        updateScreenSaverSummary();

        // Update auto brightness if it is available.
        if (mAutoBrightnessPreference != null) {
            int brightnessMode = Settings.System.getInt(getContentResolver(),
                    SCREEN_BRIGHTNESS_MODE, SCREEN_BRIGHTNESS_MODE_MANUAL);
            mAutoBrightnessPreference.setChecked(brightnessMode != SCREEN_BRIGHTNESS_MODE_MANUAL);
        }

        // Update lift-to-wake if it is available.
        if (mLiftToWakePreference != null) {
            int value = Settings.Secure.getInt(getContentResolver(), WAKE_GESTURE_ENABLED, 0);
            mLiftToWakePreference.setChecked(value != 0);
        }

        // Update doze if it is available.
        if (mDozePreference != null) {
            int value = Settings.Secure.getInt(getContentResolver(), DOZE_ENABLED, 1);
            mDozePreference.setChecked(value != 0);
        }

        // Update tap to wake if it is available.
        if (mTapToWakePreference != null) {
            int value = Settings.Secure.getInt(getContentResolver(), DOUBLE_TAP_TO_WAKE, 0);
            mTapToWakePreference.setChecked(value != 0);
        }

        // Update camera gesture #1 if it is available.
        if (mCameraGesturePreference != null) {
            int value = Settings.Secure.getInt(getContentResolver(), CAMERA_GESTURE_DISABLED, 0);
            mCameraGesturePreference.setChecked(value == 0);
        }

        // Update camera gesture #2 if it is available.
        if (mCameraDoubleTapPowerGesturePreference != null) {
            int value = Settings.Secure.getInt(
                    getContentResolver(), CAMERA_DOUBLE_TAP_POWER_GESTURE_DISABLED, 0);
            mCameraDoubleTapPowerGesturePreference.setChecked(value == 0);
        }
    }

    private void updateScreenSaverSummary() {
        if (mScreenSaverPreference != null) {
            mScreenSaverPreference.setSummary(
                    DreamSettings.getSummaryTextWithDreamName(getActivity()));
        }
    }

    public void writeFontSizePreference(Object objValue) {
        try {
            mCurConfig.fontScale = Float.parseFloat(objValue.toString());
            ActivityManagerNative.getDefault().updatePersistentConfiguration(mCurConfig);
        } catch (RemoteException e) {
            Log.w(TAG, "Unable to save font size");
        }
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
		/* add by allwinner */
		boolean value;
		int value2;
		int value3;
		try {
			if (preference == mBrightSystem) {
				value = mBrightSystem.isChecked();
				value2 = Settings.System.getInt(getContentResolver(),
										Settings.System.BRIGHT_SYSTEM_MODE);
				Settings.System.putInt(getContentResolver(),Settings.System.BRIGHT_SYSTEM_MODE,
										value ? value2|0x01 : value2&0x02);
				mBrightSystemDemo.setEnabled(value);
			} else if (preference == mBrightSystemDemo) {
				value = mBrightSystemDemo.isChecked();
				value2 = Settings.System.getInt(getContentResolver(),
										Settings.System.BRIGHT_SYSTEM_MODE);
				Settings.System.putInt(getContentResolver(),Settings.System.BRIGHT_SYSTEM_MODE,
										value ? value2|0x02 : value2&0x01);
			} else if (preference == mBrightnessLight) {
				value = mBrightnessLight.isChecked();
				value2 = Settings.System.getInt(getContentResolver(),
										Settings.System.BRIGHTNESS_LIGHT_MODE);
				value3 = Settings.System.getInt(getContentResolver(),Settings.System.BRIGHTNESS_LIGHT_MODE,
										value ? value2|0x01 : value2&0x02);
				Settings.System.putInt(getContentResolver(),Settings.System.BRIGHTNESS_LIGHT_MODE,
										value ? value2|0x01 : value2&0x02);
				mBrightnessLightDemo.setEnabled(value);
			} else if (preference == mBrightnessLightDemo) {
				value = mBrightnessLightDemo.isChecked();
				value2 = Settings.System.getInt(getContentResolver(),
										Settings.System.BRIGHTNESS_LIGHT_MODE);
				value3 = Settings.System.getInt(getContentResolver(),Settings.System.BRIGHTNESS_LIGHT_MODE,
										value ? value2|0x02 : value2&0x01);
				Settings.System.putInt(getContentResolver(),Settings.System.BRIGHTNESS_LIGHT_MODE,
										value ? value2|0x02 : value2&0x01);
			} else if (preference == mHdmiFullScreen) {
				value = mHdmiFullScreen.isChecked();
	        	Settings.System.putInt(getContentResolver(),Settings.System.HDMI_FULL_SCREEN,
	                        value ? 0x01 : 0);
	        }
        } catch (SettingNotFoundException e) {
				Log.e(TAG, Settings.System.BRIGHTNESS_LIGHT_MODE + " or " +
							Settings.System.BRIGHT_SYSTEM_MODE + "not found");
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final String key = preference.getKey();
        if (KEY_SCREEN_TIMEOUT.equals(key)) {
            try {
                int value = Integer.parseInt((String) objValue);
                Settings.System.putInt(getContentResolver(), SCREEN_OFF_TIMEOUT, value);
                updateTimeoutPreferenceDescription(value);
            } catch (NumberFormatException e) {
                Log.e(TAG, "could not persist screen timeout setting", e);
            }
        }
        if (KEY_FONT_SIZE.equals(key)) {
            writeFontSizePreference(objValue);
        }
        if (preference == mAutoBrightnessPreference) {
            boolean auto = (Boolean) objValue;
            Settings.System.putInt(getContentResolver(), SCREEN_BRIGHTNESS_MODE,
                    auto ? SCREEN_BRIGHTNESS_MODE_AUTOMATIC : SCREEN_BRIGHTNESS_MODE_MANUAL);
        }
        if (preference == mLiftToWakePreference) {
            boolean value = (Boolean) objValue;
            Settings.Secure.putInt(getContentResolver(), WAKE_GESTURE_ENABLED, value ? 1 : 0);
        }
        if (preference == mDozePreference) {
            boolean value = (Boolean) objValue;
            Settings.Secure.putInt(getContentResolver(), DOZE_ENABLED, value ? 1 : 0);
        }
        if (preference == mTapToWakePreference) {
            boolean value = (Boolean) objValue;
            Settings.Secure.putInt(getContentResolver(), DOUBLE_TAP_TO_WAKE, value ? 1 : 0);
        }
        if (preference == mCameraGesturePreference) {
            boolean value = (Boolean) objValue;
            Settings.Secure.putInt(getContentResolver(), CAMERA_GESTURE_DISABLED,
                    value ? 0 : 1 /* Backwards because setting is for disabling */);
        }
        if (preference == mCameraDoubleTapPowerGesturePreference) {
            boolean value = (Boolean) objValue;
            Settings.Secure.putInt(getContentResolver(), CAMERA_DOUBLE_TAP_POWER_GESTURE_DISABLED,
                    value ? 0 : 1 /* Backwards because setting is for disabling */);
        }
        if (preference == mNightModePreference) {
            try {
                final int value = Integer.parseInt((String) objValue);
                final UiModeManager uiManager = (UiModeManager) getSystemService(
                        Context.UI_MODE_SERVICE);
                uiManager.setNightMode(value);
            } catch (NumberFormatException e) {
                Log.e(TAG, "could not persist night mode setting", e);
            }
        }
		/* add by allwinner */
		if (KEY_HDMI_OUTPUT_MODE.equals(key) || KEY_HDMI_OUTPUT_MODE_720P.equals(key)) {
            final int value = Integer.parseInt((String) objValue);
            try {
				Settings.System.putInt(getContentResolver(), Settings.System.HDMI_OUTPUT_MODE, value);
            } catch (NumberFormatException e) {
                Log.e(TAG, "could not persist hdmi output mode setting", e);
            }
        }
        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        if (preference == mFontSizePref) {
            if (Utils.hasMultipleUsers(getActivity())) {
                showDialog(DLG_GLOBAL_CHANGE_WARNING);
                return true;
            } else {
                mFontSizePref.click();
            }
        }else
        if(preference == mScreenRecordPref){
            Intent i=new Intent(Intent.ACTION_MAIN);
			i.addCategory(Intent.CATEGORY_HOME);
		    startActivity(i);
        	takeScreenrecord();
        	return true;
        	
        }
        return false;
    }
    
    final Object mScreenrecordLock = new Object();
    // Assume this is called from the Handler thread.
    private void takeScreenrecord() {
        synchronized (mScreenrecordLock) {
            ComponentName cn = new ComponentName("com.android.systemui",
                    "com.android.systemui.screenrecord.TakeScreenrecordService");
            Intent intent = new Intent();
            intent.setComponent(cn);
            ServiceConnection conn = new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder service) {
                    synchronized (mScreenrecordLock) {
                        Messenger messenger = new Messenger(service);
                        Message msg = Message.obtain(null, 1);
                        try {
                            messenger.send(msg);
                        } catch (RemoteException e) {
                        }
                    }
                }
                @Override
                public void onServiceDisconnected(ComponentName name) {}
            };
            getActivity().bindServiceAsUser(intent, conn, Context.BIND_AUTO_CREATE, UserHandle.CURRENT);
        }
     }

    @Override
    protected int getHelpResource() {
        return R.string.help_uri_display;
    }

    public static final Indexable.SearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider() {
                @Override
                public List<SearchIndexableResource> getXmlResourcesToIndex(Context context,
                        boolean enabled) {
                    ArrayList<SearchIndexableResource> result =
                            new ArrayList<SearchIndexableResource>();

                    SearchIndexableResource sir = new SearchIndexableResource(context);
                    sir.xmlResId = R.xml.display_settings;
                    result.add(sir);

                    return result;
                }

                @Override
                public List<String> getNonIndexableKeys(Context context) {
                    ArrayList<String> result = new ArrayList<String>();
                    if (!context.getResources().getBoolean(
                            com.android.internal.R.bool.config_dreamsSupported)) {
                        result.add(KEY_SCREEN_SAVER);
                    }
                    if (!isAutomaticBrightnessAvailable(context.getResources())) {
                        result.add(KEY_AUTO_BRIGHTNESS);
                    }
                    if (!isLiftToWakeAvailable(context)) {
                        result.add(KEY_LIFT_TO_WAKE);
                    }
                    if (!isDozeAvailable(context)) {
                        result.add(KEY_DOZE);
                    }
                    if (!RotationPolicy.isRotationLockToggleVisible(context)) {
                        result.add(KEY_AUTO_ROTATE);
                    }
                    if (!isTapToWakeAvailable(context.getResources())) {
                        result.add(KEY_TAP_TO_WAKE);
                    }
                    if (!isCameraGestureAvailable(context.getResources())) {
                        result.add(KEY_CAMERA_GESTURE);
                    }
                    if (!isCameraDoubleTapPowerGestureAvailable(context.getResources())) {
                        result.add(KEY_CAMERA_DOUBLE_TAP_POWER_GESTURE);
                    }
                    return result;
                }
            };
}
